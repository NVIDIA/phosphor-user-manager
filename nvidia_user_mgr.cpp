/*
 * NVIDIA-specific extensions to phosphor-user-manager.
 *
 * This file contains downstream-only additions that are kept separate from
 * user_mgr.cpp to reduce merge conflicts when syncing with the upstream
 * phosphor-user-manager repository.
 */

#include "config.h"

#include "user_mgr.hpp"

#include <fcntl.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace phosphor
{
namespace user
{

#ifdef ENABLE_SSH_PREFERRED_AUTHENTICATION

static void syncToDisk(const std::string& filePath)
{
    int fd = ::open(filePath.c_str(), O_WRONLY);
    if (fd >= 0)
    {
        if (::fsync(fd) != 0)
        {
            lg2::error("Failed to fsync dropbear configuration temp file");
        }
        ::close(fd);
    }
}
using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
using Argument = xyz::openbmc_project::Common::InvalidArgument;

static constexpr const char* dropbearConfigFile = "/etc/default/dropbear";
static constexpr const char* consoleDropbearEnvFile =
    "/etc/obmc-console/dropbear.env";

/** @brief Reads configFile and returns true if the -s flag
 *  (disable password authentication) is present in DROPBEAR_EXTRA_ARGS.
 */
static bool isDropbearPasswordAuthDisabled(const char* configFile)
{
    std::ifstream dropbearFile(configFile);
    if (!dropbearFile.is_open())
    {
        lg2::warning(
            "Failed to open dropbear config {FILE}, assuming password auth enabled",
            "FILE", configFile);
        return false;
    }
    std::string line;
    while (std::getline(dropbearFile, line))
    {
        constexpr std::string_view argKey = "DROPBEAR_EXTRA_ARGS=";
        if (!line.starts_with(argKey))
        {
            continue;
        }
        auto startQuote = line.find('"');
        auto endQuote = line.rfind('"');
        if (startQuote != std::string::npos && endQuote != startQuote)
        {
            std::string args =
                line.substr(startQuote + 1, endQuote - startQuote - 1);
            std::istringstream iss(args);
            std::string token;
            while (iss >> token)
            {
                if (token == "-s")
                {
                    return true;
                }
            }
        }
        break;
    }
    return false;
}

/** @brief Adds (disablePasswordAuth == true) or removes the -s flag in
 *  DROPBEAR_EXTRA_ARGS of configFile. The file is rewritten atomically.
 *
 *  @return true on success, false on any I/O failure.
 */
static bool setDropbearPasswordAuthFlag(const char* configFile,
                                        bool disablePasswordAuth)
{
    std::string tmpFile = std::string(configFile) + "_tmp";
    std::ifstream fileToRead(configFile, std::ios::in);
    std::ofstream fileToWrite(tmpFile, std::ios::out);
    if (!fileToRead.is_open() || !fileToWrite.is_open())
    {
        lg2::error("Failed to open dropbear configuration file {FILE}", "FILE",
                   configFile);
        (void)std::remove(tmpFile.c_str());
        return false;
    }
    auto writeFailed = [&tmpFile]() {
        lg2::error("Failed to write dropbear configuration temp file {FILE}",
                   "FILE", tmpFile);
        (void)std::remove(tmpFile.c_str());
        return false;
    };
    std::string line;
    while (std::getline(fileToRead, line))
    {
        constexpr std::string_view argKey = "DROPBEAR_EXTRA_ARGS=";
        if (!line.starts_with(argKey))
        {
            fileToWrite << line << '\n';
            if (!fileToWrite)
            {
                return writeFailed();
            }
            continue;
        }
        auto startQuote = line.find('"');
        auto endQuote = line.rfind('"');
        if (startQuote != std::string::npos && endQuote != startQuote)
        {
            std::string args =
                line.substr(startQuote + 1, endQuote - startQuote - 1);

            // Tokenize, strip any existing -s, optionally re-add
            std::istringstream iss(args);
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token)
            {
                if (token != "-s")
                {
                    tokens.push_back(token);
                }
            }
            if (disablePasswordAuth)
            {
                tokens.insert(tokens.begin(), "-s");
            }

            std::string newArgs;
            for (const auto& t : tokens)
            {
                if (!newArgs.empty())
                {
                    newArgs += ' ';
                }
                newArgs += t;
            }
            fileToWrite << argKey << '"' << newArgs << '"' << '\n';
        }
        else
        {
            fileToWrite << line << '\n';
        }
        if (!fileToWrite)
        {
            return writeFailed();
        }
    }
    fileToWrite.flush();
    if (!fileToWrite)
    {
        return writeFailed();
    }
    syncToDisk(tmpFile);
    fileToWrite.close();
    if (!fileToWrite)
    {
        return writeFailed();
    }
    fileToRead.close();
    if (std::rename(tmpFile.c_str(), configFile) != 0)
    {
        (void)std::remove(tmpFile.c_str());
        lg2::error("Failed to rename dropbear configuration temp file {FILE}",
                   "FILE", tmpFile);
        return false;
    }
    {
        std::string parentDir =
            std::filesystem::path(configFile).parent_path().string();
        int dirFd = ::open(parentDir.c_str(), O_RDONLY | O_DIRECTORY);
        if (dirFd >= 0)
        {
            if (::fsync(dirFd) != 0)
            {
                lg2::error("Failed to fsync parent directory of {FILE}", "FILE",
                           configFile);
            }
            ::close(dirFd);
        }
    }
    return true;
}

std::vector<UserMgr::AuthenticationMethod> UserMgr::sshPreferredAuthentication(
    std::vector<AuthenticationMethod> value)
{
    const bool disablePasswordAuth =
        std::find(value.begin(), value.end(), AuthenticationMethod::Password) ==
        value.end();

    // Only limited combination of authentication methods are supported.
    // Validate input for supported method
    bool validRequest = false;
    if ((std::find(value.begin(), value.end(),
                   AuthenticationMethod::PublicKey) != value.end()) &&
        value.size() == 1)
    {
        validRequest = true;
    }
    else if ((std::find(value.begin(), value.end(),
                        AuthenticationMethod::PublicKey) != value.end()) &&
             (std::find(value.begin(), value.end(),
                        AuthenticationMethod::Password) != value.end()) &&
             value.size() == 2)
    {
        validRequest = true;
    }
    if (!validRequest)
    {
        lg2::error(
            "PreferredSSHAuthentication method combination is not supported");
        elog<InvalidArgument>(
            Argument::ARGUMENT_NAME("SshPreferredAuthentication"),
            Argument::ARGUMENT_VALUE("Combination not supported"));
    }
    // Compare against the files, not the cached property: the property
    // reports Password if either file allows it, so the files can disagree.
    if (isDropbearPasswordAuthDisabled(dropbearConfigFile) !=
            disablePasswordAuth &&
        !setDropbearPasswordAuthFlag(dropbearConfigFile, disablePasswordAuth))
    {
        elog<InternalFailure>();
    }
    if (std::filesystem::exists(consoleDropbearEnvFile) &&
        isDropbearPasswordAuthDisabled(consoleDropbearEnvFile) !=
            disablePasswordAuth &&
        !setDropbearPasswordAuthFlag(consoleDropbearEnvFile,
                                     disablePasswordAuth))
    {
        elog<InternalFailure>();
    }
    return NvidiaAccountPolicyIface::sshPreferredAuthentication(value);
}

void UserMgr::initializeSshPreferredAuthentication()
{
    bool passwordAuthDisabled =
        isDropbearPasswordAuthDisabled(dropbearConfigFile);
    if (std::filesystem::exists(consoleDropbearEnvFile))
    {
        passwordAuthDisabled =
            passwordAuthDisabled &&
            isDropbearPasswordAuthDisabled(consoleDropbearEnvFile);
    }
    std::vector<AuthenticationMethod> authMethods = {
        AuthenticationMethod::PublicKey};
    if (!passwordAuthDisabled)
    {
        authMethods.push_back(AuthenticationMethod::Password);
    }
    NvidiaAccountPolicyIface::sshPreferredAuthentication(authMethods);
}

#endif // ENABLE_SSH_PREFERRED_AUTHENTICATION

} // namespace user
} // namespace phosphor
