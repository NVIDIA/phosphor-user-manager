/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include "json_serializer.hpp"
#include "users.hpp"

#include <sys/wait.h>
#include <unistd.h>

#ifdef ENABLE_SSH_PREFERRED_AUTHENTICATION
#include <com/nvidia/User/AccountPolicy/server.hpp>
#endif
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/AccountPolicy/server.hpp>
#include <xyz/openbmc_project/User/Manager/server.hpp>
#include <xyz/openbmc_project/User/MultiFactorAuthConfiguration/server.hpp>
#include <xyz/openbmc_project/User/TOTPState/server.hpp>

#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace phosphor
{
namespace user
{
#ifdef ENABLE_IPMI
inline constexpr size_t ipmiMaxUsers = 15;
#else
inline constexpr size_t ipmiMaxUsers = 0;
#endif
inline constexpr size_t redfishHostInterfaceUsers = 15;
inline constexpr size_t maxSystemUsers =
    15 + ipmiMaxUsers + redfishHostInterfaceUsers;
extern uint8_t minPasswdLength; // MIN_PASSWORD_LENGTH;
extern uint8_t maxPasswdLength; // MAX_PASSWORD_LENGTH;
inline constexpr size_t maxSystemGroupNameLength = 32;
inline constexpr size_t maxSystemGroupCount = 64;

inline constexpr int policyAdoptionTypeDefault = 1;
inline constexpr int policyAdoptionTypeConditional = 2;
inline constexpr int policyAdoptionTypeUniversal = 3;

using UserMgrIface = sdbusplus::xyz::openbmc_project::User::server::Manager;
using UserSSHLists =
    std::pair<std::vector<std::string>, std::vector<std::string>>;
using AccountPolicyIface =
    sdbusplus::xyz::openbmc_project::User::server::AccountPolicy;

#ifdef ENABLE_SSH_PREFERRED_AUTHENTICATION
using NvidiaAccountPolicyIface =
    sdbusplus::com::nvidia::User::server::AccountPolicy;
#endif

using MultiFactorAuthConfigurationIface =
    sdbusplus::xyz::openbmc_project::User::server::MultiFactorAuthConfiguration;

using TOTPStateIface = sdbusplus::xyz::openbmc_project::User::server::TOTPState;

#ifdef ENABLE_SSH_PREFERRED_AUTHENTICATION
using Ifaces = sdbusplus::server::object_t<
    UserMgrIface, AccountPolicyIface, NvidiaAccountPolicyIface,
    MultiFactorAuthConfigurationIface, TOTPStateIface>;
#else
using Ifaces = sdbusplus::server::object_t<UserMgrIface, AccountPolicyIface,
                                           MultiFactorAuthConfigurationIface,
                                           TOTPStateIface>;
#endif

using Privilege = std::string;
using GroupList = std::vector<std::string>;
using UserEnabled = bool;
using PropertyName = std::string;
using ServiceEnabled = bool;

using UserInfo = std::variant<Privilege, GroupList, UserEnabled>;
using UserInfoMap = std::map<PropertyName, UserInfo>;

using DbusUserObjPath = sdbusplus::message::object_path;

using DbusUserPropVariant = std::variant<Privilege, ServiceEnabled>;

using DbusUserObjProperties = std::map<PropertyName, DbusUserPropVariant>;

using Interface = std::string;

using DbusUserObjValue = std::map<Interface, DbusUserObjProperties>;

using DbusUserObj = std::map<DbusUserObjPath, DbusUserObjValue>;

using MultiFactorAuthType = sdbusplus::common::xyz::openbmc_project::user::
    MultiFactorAuthConfiguration::Type;
std::string getCSVFromVector(std::span<const std::string> vec);

bool removeStringFromCSV(std::string& csvStr, const std::string& delStr);

template <typename... ArgTypes>
std::vector<std::string> executeCmd(const char* path, ArgTypes&&... tArgs)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        lg2::error("Failed to create pipe: {ERROR}", "ERROR", strerror(errno));
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
        return {};
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        lg2::error("Failed to fork process: {ERROR}", "ERROR", strerror(errno));
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
        close(pipefd[0]); // Close read end of pipe
        close(pipefd[1]); // Close write end of pipe
        return {};
    }

    if (pid == 0)         // Child process
    {
        close(pipefd[0]); // Close read end of pipe

        // Redirect write end of the pipe to stdout.
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            lg2::error("Failed to redirect stdout: {ERROR}", "ERROR",
                       strerror(errno));
            _exit(EXIT_FAILURE);
        }
        close(pipefd[1]); // Close write end of pipe

        std::vector<const char*> args = {path};
        (args.emplace_back(const_cast<const char*>(tArgs)), ...);
        args.emplace_back(nullptr);

        execv(path, const_cast<char* const*>(args.data()));

        // If exec returns, an error occurred
        lg2::error("Failed to execute command '{PATH}': {ERROR}", "PATH", path,
                   "ERROR", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    // Parent process.

    close(pipefd[1]); // Close write end of pipe

    FILE* fp = fdopen(pipefd[0], "r");
    if (fp == nullptr)
    {
        lg2::error("Failed to open pipe for reading: {ERROR}", "ERROR",
                   strerror(errno));
        close(pipefd[0]);
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
        return {};
    }

    std::vector<std::string> results;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != nullptr)
    {
        std::string line = buffer;
        if (!line.empty() && line.back() == '\n')
        {
            line.pop_back(); // Remove newline character
        }
        results.emplace_back(line);
    }

    fclose(fp);
    close(pipefd[0]);

    int status;
    while (waitpid(pid, &status, 0) == -1)
    {
        // Need to retry on EINTR.
        if (errno == EINTR)
        {
            continue;
        }

        lg2::error("Failed to wait for child process: {ERROR}", "ERROR",
                   strerror(errno));
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
        return {};
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        lg2::error("Command {PATH} execution failed, return code {RETCODE}",
                   "PATH", path, "RETCODE", WEXITSTATUS(status));
        phosphor::logging::elog<
            sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure>();
    }

    return results;
}

/** @class UserMgr
 *  @brief Responsible for managing user accounts over the D-Bus interface.
 */
class UserMgr : public Ifaces
{
  public:
    UserMgr() = delete;
    ~UserMgr() = default;
    UserMgr(const UserMgr&) = delete;
    UserMgr& operator=(const UserMgr&) = delete;
    UserMgr(UserMgr&&) = delete;
    UserMgr& operator=(UserMgr&&) = delete;

    /** @brief Constructs UserMgr object.
     *
     * @param[in] bus  - Bus to attach to.
     * @param[in] path - Path to attach to.
     */
    UserMgr(sdbusplus::bus_t& bus, const char* path);

    /** @brief Sets the policy adoption type
     *
     * @param[in] policyType - Policy adoption type (1=default, 2=conditional,
     * 3=universal)
     */
    void setPolicyAdoptionType(uint8_t policyType);

    /** @brief Set password expiry policy
     *
     * @param[in] updatePasswordExpiry - Enable/Disable password expiry
     * @param[in] exclusionList - Array of users to exclude from password expiry
     * @param[in] listSize - Size of the exclusion list array
     */
    void setPasswordExpirePolicy(
        bool updatePasswordExpiry,
        const std::initializer_list<const char*>& exclusionList);

    /** @brief Initializes the UserMgr object
     *
     * This method performs the initialization steps that were previously in the
     * constructor:
     * - Checks and updates password policy files
     * - Sets up privileges and groups
     * - Initializes account policy
     * - Sets up user objects
     * - Emits the object added signal
     */
    void initialize();

    /** @brief create user method.
     *  This method creates a new user as requested
     *
     *  @param[in] userName - Name of the user which has to be created
     *  @param[in] groupNames - Group names list, to which user has to be added.
     *  @param[in] priv - Privilege of the user.
     *  @param[in] enabled - State of the user enabled / disabled.
     */
    void createUser(std::string userName, std::vector<std::string> groupNames,
                    std::string priv, bool enabled) override;

    /** @brief rename user method.
     *  This method renames the user as requested
     *
     *  @param[in] userName - current name of the user
     *  @param[in] newUserName - new user name to which it has to be renamed.
     */
    void renameUser(std::string userName, std::string newUserName) override;

    /** @brief delete user method.
     *  This method deletes the user as requested
     *
     *  @param[in] userName - Name of the user which has to be deleted
     */
    void deleteUser(std::string userName);

    /** @brief Update user groups & privilege.
     *  This method updates user groups & privilege
     *
     *  @param[in] userName - user name, for which update is requested
     *  @param[in] groupName - Group to be updated..
     *  @param[in] priv - Privilege to be updated.
     */
    void updateGroupsAndPriv(const std::string& userName,
                             std::vector<std::string> groups,
                             const std::string& priv);

    /** @brief Update user enabled state.
     *  This method enables / disables user
     *
     *  @param[in] userName - user name, for which update is requested
     *  @param[in] enabled - enable / disable the user
     */
    void userEnable(const std::string& userName, bool enabled);

    /** @brief get user enabled state
     *  method to get user enabled state.
     *
     *  @param[in] userName - name of the user
     *  @return - user enabled status (true/false)
     */
    virtual bool isUserEnabled(const std::string& userName);

    /** @brief update minimum password length requirement
     *
     *  @param[in] val - minimum password length
     *  @return - minimum password length
     */
    uint8_t minPasswordLength(uint8_t val) override;

    /** @brief update old password history count
     *
     *  @param[in] val - number of times old passwords has to be avoided
     *  @return - number of times old password has to be avoided
     */
    uint8_t rememberOldPasswordTimes(uint8_t val) override;

    /** @brief update maximum number of failed login attempt before locked
     *  out.
     *
     *  @param[in] val - number of allowed attempt
     *  @return - number of allowed attempt
     */
    uint16_t maxLoginAttemptBeforeLockout(uint16_t val) override;

#ifdef ENABLE_SSH_PREFERRED_AUTHENTICATION
    std::vector<AuthenticationMethod> sshPreferredAuthentication(
        std::vector<AuthenticationMethod> value) override;
#endif

    /** @brief update timeout to unlock the account
     *
     *  @param[in] val - value in seconds
     *  @return - value in seconds
     */
    uint32_t accountUnlockTimeout(uint32_t val) override;

    /** @brief parses the faillock output for locked user status
     *
     * @param[in] - output from faillock for the user
     * @return - true / false indicating user locked / un-locked
     **/
    bool parseFaillockForLockout(
        const std::vector<std::string>& faillockOutput);

    /** @brief lists user locked state for failed attempt
     *
     * @param[in] - user name
     * @return - true / false indicating user locked / un-locked
     **/
    virtual bool userLockedForFailedAttempt(const std::string& userName);

    /** @brief lists user locked state for failed attempt
     *
     * @param[in]: user name
     * @param[in]: value - false -unlock user account, true - no action taken
     **/
    bool userLockedForFailedAttempt(const std::string& userName,
                                    const bool& value);

    /** @brief shows if the user's password is expired
     *
     * @param[in]: user name
     * @return - true / false indicating user password expired
     **/
    virtual bool userPasswordExpired(const std::string& userName);

    /** @brief returns user info
     * Checks if user is local user, then returns map of properties of user.
     * like user privilege, list of user groups, user enabled state and user
     * locked state. If its not local user, then it checks if its a ldap user,
     * then it gets the privilege mapping of the LDAP group.
     *
     * @param[in] - user name
     * @return -  map of user properties
     **/
    UserInfoMap getUserInfo(std::string userName) override;

    /** @brief get IPMI user count
     *  method to get IPMI user count
     *
     * @return - returns user count
     */
    virtual size_t getIpmiUsersCount(void);

    /** @brief get redfish-hostiface user count
     *  method to get redfish-hostiface user count
     *
     * @return - returns user count
     */
    size_t getRedfishHostInterfaceUsersCount(void);

    void createGroup(std::string groupName) override;

    void deleteGroup(std::string groupName) override;
    MultiFactorAuthType enabled() const override
    {
        return MultiFactorAuthConfigurationIface::enabled();
    }
    MultiFactorAuthType enabled(MultiFactorAuthType value,
                                bool skipSignal) override;
    bool secretKeyRequired(std::string userName) override;
    static std::vector<std::string> readAllGroupsOnSystem();
    void load();
    JsonSerializer& getSerializer()
    {
        return serializer;
    }

  protected:
    /** @brief get pam argument value
     *  method to get argument value from pam configuration
     *
     *  @param[in] confFile - path of the module config file from where arg has
     * to be read
     *  @param[in] argName - argument name
     *  @param[out] argValue - argument value
     *
     *  @return 0 - success state of the function
     */
    int getPamModuleConfValue(const std::string& confFile,
                              const std::string& argName,
                              std::string& argValue);

    /** @brief set pam argument value
     *  method to set argument value in pam configuration
     *
     *  @param[in] confFile - path of the module config file in which argument
     * value has to be set
     *  @param[in] argName - argument name
     *  @param[out] argValue - argument value
     *
     *  @return 0 - success state of the function
     */
    int setPamModuleConfValue(const std::string& confFile,
                              const std::string& argName,
                              const std::string& argValue);

    /** @brief check for user presence
     *  method to check for user existence
     *
     *  @param[in] userName - name of the user
     *  @return -true if user exists and false if not.
     */
    bool isUserExist(const std::string& userName);

    /** @brief check for user presence at the system level
     *  method to check for user existence
     *
     *  @param[in] userName - name of the user
     *  @return -true if user exists and false if not.
     */
    virtual bool isUserExistSystem(const std::string& userName);

    size_t getNonIpmiUsersCount();

    /** @brief check user exists
     *  method to check whether user exist, and throw if not.
     *
     *  @param[in] userName - name of the user
     */
    void throwForUserDoesNotExist(const std::string& userName);

    /** @brief check if user is belongs to "service" group
     *  method to check whether user is in "service" group and throw if yes.
     *
     *  @param[in] userName - name of the user
     */
    void throwForDeleteUserInServiceGroup(const std::string& userName);

    /** @brief check user does not exist
     *  method to check whether does not exist, and throw if exists.
     *
     *  @param[in] userName - name of the user
     */
    void throwForUserExists(const std::string& userName);

    /** @brief check user name constraints
     *  method to check user name constraints and throw if failed.
     *
     *  @param[in] userName - name of the user
     *  @param[in] groupNames - user groups
     */
    void throwForUserNameConstraints(
        const std::string& userName,
        const std::vector<std::string>& groupNames);

    /** @brief check group user count
     *  method to check max group user count, and throw if limit reached
     *
     *  @param[in] groupNames - group name
     */
    void throwForMaxGrpUserCount(const std::vector<std::string>& groupNames);

    /** @brief Executes the addition of a user
     *
     *  @param[in] userName - Name of the user to be added
     *  @param[in] groups - Groups to which the user should be added
     *  @param[in] sshRequested - Whether SSH access is requested
     *  @param[in] enabled - Whether the user should be enabled
     */
    virtual void executeUserAdd(const char* userName, const char* groups,
                                bool sshRequested, bool enabled);

    /** @brief Executes the deletion of a user
     *
     *  @param[in] userName - Name of the user to be deleted
     */
    virtual void executeUserDelete(const char* userName);

    /** @brief clear user's failure records
     *  method to clear user fail records and throw if failed.
     *
     *  @param[in] userName - name of the user
     */
    virtual void executeUserClearFailRecords(const char* userName);

    /** @brief Executes the renaming of a user
     *
     *  This method changes the username of an existing user to a new username.
     *
     *  @param[in] userName - Current name of the user
     *  @param[in] newUserName - New name for the user
     */
    virtual void executeUserRename(const char* userName,
                                   const char* newUserName);

    /** @brief Modifies user groups and SSH access
     *
     *  @param[in] userName - Name of the user to be modified
     *  @param[in] newGroups - New groups for the user
     *  @param[in] sshRequested - Whether SSH access is requested
     */
    virtual void executeUserModify(const char* userName, const char* newGroups,
                                   bool sshRequested);

    /** @brief Modifies the enabled state of a user
     *
     *  @param[in] userName - Name of the user to be modified
     *  @param[in] enabled - New enabled state for the user
     */
    virtual void executeUserModifyUserEnable(const char* userName,
                                             bool enabled);

    /** @brief Sets password expiry for a user
     *
     *  @param[in] userName - Name of the user whose password should expire
     */
    virtual void executeUserPasswordExpiry(const char* userName);

    /** @brief Executes the creation of a group
     *
     *  @param[in] groupName - Name of the group to be created
     */
    virtual void executeGroupCreation(const char* groupName);

    /** @brief Executes the deletion of a group
     *
     *  @param[in] groupName - Name of the group to be deleted
     */
    virtual void executeGroupDeletion(const char* groupName);

    /** @brief Retrieves failed login attempts for a user
     *
     *  @param[in] userName - Name of the user
     *  @return - Vector of strings representing failed attempts
     */
    virtual std::vector<std::string> getFailedAttempt(const char* userName);

    /** @brief check for valid privielge
     *  method to check valid privilege, and throw if invalid
     *
     *  @param[in] priv - privilege of the user
     */
    void throwForInvalidPrivilege(const std::string& priv);

    /** @brief check for valid groups
     *  method to check valid groups, and throw if invalid
     *
     *  @param[in] groupNames - user groups
     */
    void throwForInvalidGroups(const std::vector<std::string>& groupName);

    void initializeAccountPolicy();

    /** @brief ensure all predefined groups exist on the system
     *  Iterates over |predefinedGroups| and creates any that are missing
     *  from /etc/group. This recovers the case where a firmware update adds
     *  a new predefined group to the read-only rootfs but the persistent
     *  /etc/group overlay still shadows the old file (so the group only
     *  appeared after a factory reset previously).
     */
    void ensurePredefinedGroupsExist();

    /** @brief checks if the group creation meets all constraints
     * @param groupName - group to check
     */
    void checkCreateGroupConstraints(const std::string& groupName);

    /** @brief checks if the group deletion meets all constraints
     * @param groupName - group to check
     */
    void checkDeleteGroupConstraints(const std::string& groupName);

    /** @brief checks if the group name is legal and whether it's allowed to
     * change. The daemon doesn't allow arbitrary group to be created
     * @param groupName - group to check
     */
    void checkAndThrowForDisallowedGroupCreation(const std::string& groupName);

    /** @brief returns groups info
     * Checks for the available user groups, by which new user can be created.
     *
     * @param[in] - none
     * @return -  vector of strings of user groups
     **/
    std::vector<std::string> allGroups() const override;

    /**
     * @brief Check if password policy configuration file needs to be updated
     *
     * Determines if the password policy configuration file should be updated
     * based on the configured POLICY_UPDATE_ADOPTION_TYPE:
     * - POLICY_TYPE_DEFAULT: Updates only on first boot
     * - POLICY_TYPE_UNIVERSAL: Updates if default config has newer version
     * - POLICY_TYPE_CONDITIONAL: Updates if current config matches a previous
     * version
     *
     * @param workingConfigPath Path to current working pwquality config file
     * @param defaultConfigPath Path to default pwquality config file
     * @param previousConfigDirPath Path to directory containing previous config
     * files
     * @return true if policy file should be updated, false otherwise
     */
    bool shouldUpdatePolicyFile(const std::string& workingConfigPath,
                                const std::string& defaultConfigPath,
                                const std::string& previousConfigDirPath);

    /**
     * @brief Manages password policy configuration file updates
     *
     * Handles the update process for password quality configuration files by:
     * 1. Checking if an update is needed via shouldUpdatePolicyFile()
     * 2. If update is needed, copies the default configuration
     * 3. Updates password expiration settings if configured
     *
     * @param firstBootPath Path to file used to determine first boot status
     * @param workingConfigPath Path to current working pwquality config file
     * @param defaultConfigPath Path to default pwquality config file
     * @param previousConfigDirPath Path to directory containing previous config
     * files
     */
    void passwordPolicyFileCheck(const std::string& firstBootPath,
                                 const std::string& workingConfigPath,
                                 const std::string& defaultConfigPath,
                                 const std::string& previousConfigDirPath);

    /**
     * @brief Updates password expiration settings for a specific user
     *
     * Applies password expiration policy settings to the specified user
     * account. This includes:
     * 1. Setting maximum password age
     * 2. Setting minimum password age
     * 3. Setting password expiration warning period
     * 4. Enforcing password change at next login if required
     *
     * The method uses system commands (chage) to modify the user's
     * password aging and expiration parameters according to the
     * system-wide password policy settings.
     *
     * @param user The username for which to update password expiration settings
     * @return bool Returns true if password was successfully expired,
     *              false if user is in exclusion list or operation failed
     */
    bool passwordPolicyUpdateUserPasswordExpiration(const std::string& user);

    /**
     * @brief Compares the contents of two files byte by byte
     *
     * This method performs a binary comparison between two files to determine
     * if they are identical. It is used primarily for checking if configuration
     * files need to be updated.
     *
     * The comparison:
     * - Reads both files in binary mode
     * - Compares files byte by byte
     * - Handles files of different sizes
     * - Stops at the first difference found
     *
     * @param file1 Path to the first file to compare
     * @param file2 Path to the second file to compare
     * @return bool Returns true if files are identical, false if they differ
     *              or if either file cannot be opened
     */
    bool compareFiles(const std::string& file1, const std::string& file2);

    /**
     * @brief Compares version numbers between two configuration files
     *
     * Checks if the defaults file has a higher version number than the working
     * file. Version numbers are expected to be in the format "#version=N" where
     * N is an integer. Whitespace between # and version= is allowed.
     *
     * @param defaults - Path to the defaults configuration file
     * @param working - Path to the working configuration file
     * @return bool - Returns:
     *                true if:
     *                - defaults file has no version
     *                - working file has no version
     *                - defaults version > working version
     *                false if:
     *                - files cannot be opened
     *                - working version >= defaults version
     */
    bool checkVersion(const std::string& defaults, const std::string& working);

    /**
     * @brief Gets the version number from a configuration file
     *
     * Looks for a line starting with # and containing version=N
     * where N is an integer. Handles various whitespace formats:
     * #version=1
     * # version=1
     * #  version=1
     * #\tversion=1
     *
     * @param file - Open input file stream to read from
     * @return std::optional<int> - The version number if found and valid,
     *                             std::nullopt otherwise
     */
    std::optional<int> getFileVersion(std::ifstream& file);

  private:
    /** @brief sdbusplus handler */
    sdbusplus::bus_t& bus;

    /** @brief object path */
    const std::string path;

    /** @brief serializer for mfa */
    JsonSerializer serializer;
    /** @brief privilege manager container */
    const std::vector<std::string> privMgr = {"priv-admin", "priv-operator",
                                              "priv-user"};

    /** @brief groups manager container */
    std::vector<std::string> groupsMgr;

    /** @brief map container to hold users object */

    std::unordered_map<std::string, std::unique_ptr<phosphor::user::Users>>
        usersList;

    /** @brief Flag indicating if password expiration will be examined */
    bool policyUpdatePasswordExpiry{false};

    /** @brief List of users excluded from password expiration policy */
    std::vector<std::string> expiryExclusionList;

    /** @brief Flag indicating if password expiration is required */
    bool needPasswordExpiry{false};

    /**
     * @brief Flag indicating type of new NIST standard policy adoption
     */
    uint8_t policyAdoptionType = policyAdoptionTypeDefault;

    /** @brief get users in group
     *  method to get group user list
     *
     *  @param[in] groupName - group name
     *
     *  @return userList  - list of users in the group.
     */
    std::vector<std::string> getUsersInGroup(const std::string& groupName);

    /** @brief get user & SSH users list
     *  method to get the users and ssh users list.
     *
     *@return - vector of User & SSH user lists
     */
    UserSSHLists getUserAndSshGrpList(void);

    /** @brief initialize the user manager objects
     *  method to initialize the user manager objects accordingly
     *
     */
    void initUserObjects(void);

#ifdef ENABLE_SSH_PREFERRED_AUTHENTICATION
    /** @brief initialize SSH preferred authentication from dropbear config
     */
    void initializeSshPreferredAuthentication();
#endif

    /** @brief get service name
     *  method to get dbus service name
     *
     *  @param[in] path - object path
     *  @param[in] intf - interface
     *  @return - service name
     */
    std::string getServiceName(std::string&& path, std::string&& intf);

    /** @brief get primary group ID of specified user
     *
     * @param[in] - userName
     * @return - primary group ID
     */
    virtual gid_t getPrimaryGroup(const std::string& userName) const;

    /** @brief check whether if the user is a member of the group
     *
     * @param[in] - userName
     * @param[in] - ID of the user's primary group
     * @param[in] - groupName
     * @return - true if the user is a member of the group
     */
    virtual bool isGroupMember(const std::string& userName, gid_t primaryGid,
                               const std::string& groupName) const;

    /** @brief get privilege mapper object
     *  method to get dbus privilege mapper object
     *
     *  @return - map of user object
     */
    virtual DbusUserObj getPrivilegeMapperObject(void);

    /** @brief check whether if the user is a root privilege user
     *
     * @param[in] - user
     * @param[in] - userList
     * @return - true if the user is a root privilege user
     */
    bool isRootPrivilegeUser(
        const std::string& user,
        const std::initializer_list<const char*>& userList);
    friend class TestUserMgr;

  protected:
    /** @brief Path to the faillock configuration file
     *  This file controls the account locking settings after failed login
     * attempts. Default path is "/etc/security/faillock.conf". Used to
     * store/retrieve settings like maximum failed login attempts and account
     * unlock timeout.
     */
    std::string faillockConfigFile;

    /** @brief Path to the password history configuration file
     *  This file controls how many previous passwords are remembered to prevent
     * reuse. Default path is "/etc/security/pwhistory.conf". Used to
     * store/retrieve the number of old passwords that should be remembered.
     */
    std::string pwHistoryConfigFile;

    /** @brief Path to the password quality configuration file
     *  This file controls password complexity requirements and related
     * settings. Default path is "/etc/security/pwquality.conf". Used to
     * store/retrieve settings like minimum password length and other password
     * quality requirements.
     */
    std::string pwQualityConfigFile;
};

} // namespace user
} // namespace phosphor
