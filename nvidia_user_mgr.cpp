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

// systemd D-Bus related
static constexpr const char* systemdService = "org.freedesktop.systemd1";
static constexpr const char* systemdObjPath = "/org/freedesktop/systemd1";
static constexpr const char* systemdMgrInterface =
    "org.freedesktop.systemd1.Manager";
static constexpr const char* systemdUnitInterface =
    "org.freedesktop.systemd1.Unit";
static constexpr const char* dbusPropertiesInterface =
    "org.freedesktop.DBus.Properties";

/** @brief Reads dropbearConfigFile and returns true if the -s flag
 *  (disable password authentication) is present in DROPBEAR_EXTRA_ARGS.
 */
static bool isDropbearPasswordAuthDisabled()
{
    std::ifstream dropbearFile(dropbearConfigFile);
    if (!dropbearFile.is_open())
    {
        lg2::warning(
            "Failed to open dropbear config {FILE}, assuming password auth enabled",
            "FILE", dropbearConfigFile);
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

std::vector<UserMgr::AuthenticationMethod> UserMgr::sshPreferredAuthentication(
    std::vector<AuthenticationMethod> value)
{
    auto current = NvidiaAccountPolicyIface::sshPreferredAuthentication();
    if (value.size() == current.size() &&
        std::is_permutation(value.begin(), value.end(), current.begin()))
    {
        return current;
    }

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
    std::string tmpFile = std::string(dropbearConfigFile) + "_tmp";
    std::ifstream fileToRead(dropbearConfigFile, std::ios::in);
    std::ofstream fileToWrite(tmpFile, std::ios::out);
    if (!fileToRead.is_open() || !fileToWrite.is_open())
    {
        lg2::error("Failed to open dropbear configuration file {FILE}", "FILE",
                   dropbearConfigFile);
        (void)std::remove(tmpFile.c_str());
        elog<InternalFailure>();
    }
    std::string line;
    while (std::getline(fileToRead, line))
    {
        constexpr std::string_view argKey = "DROPBEAR_EXTRA_ARGS=";
        if (!line.starts_with(argKey))
        {
            fileToWrite << line << '\n';
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
                    tokens.push_back(token);
            }
            if (disablePasswordAuth)
                tokens.insert(tokens.begin(), "-s");

            std::string newArgs;
            for (const auto& t : tokens)
            {
                if (!newArgs.empty())
                    newArgs += ' ';
                newArgs += t;
            }
            fileToWrite << argKey << '"' << newArgs << '"' << '\n';
        }
        else
        {
            fileToWrite << line << '\n';
        }
    }
    fileToWrite.flush();
    syncToDisk(tmpFile);
    fileToWrite.close();
    fileToRead.close();
    if (std::rename(tmpFile.c_str(), dropbearConfigFile) != 0)
    {
        (void)std::remove(tmpFile.c_str());
        lg2::error("Failed to rename dropbear configuration temp file");
        elog<InternalFailure>();
    }
    {
        std::string parentDir =
            std::filesystem::path(dropbearConfigFile).parent_path().string();
        int dirFd = ::open(parentDir.c_str(), O_RDONLY | O_DIRECTORY);
        if (dirFd >= 0)
        {
            if (::fsync(dirFd) != 0)
            {
                lg2::error(
                    "Failed to fsync parent directory of dropbear config");
            }
            ::close(dirFd);
        }
    }
    try
    {
        auto startCall = bus.new_method_call(
            systemdService, systemdObjPath, systemdMgrInterface, "RestartUnit");
        startCall.append("dropbear.socket", "replace");
        bus.call(startCall);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to start/restart dropbear.socket: {ERR}", "ERR", e);
        elog<InternalFailure>();
    }
    return NvidiaAccountPolicyIface::sshPreferredAuthentication(value);
}

void UserMgr::initializeSshPreferredAuthentication()
{
    std::vector<AuthenticationMethod> authMethods = {
        AuthenticationMethod::PublicKey};
    if (!isDropbearPasswordAuthDisabled())
    {
        authMethods.push_back(AuthenticationMethod::Password);
    }
    NvidiaAccountPolicyIface::sshPreferredAuthentication(authMethods);
}

#endif // ENABLE_SSH_PREFERRED_AUTHENTICATION

} // namespace user
} // namespace phosphor
