#include "config.h"

#include "ldap_mapper_entry.hpp"

#include "ldap_config.hpp"
#include "ldap_mapper_serialize.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/redfish_event_log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/Common/error.hpp>

#include <filesystem>

namespace phosphor
{
namespace ldap
{

using namespace phosphor::logging;
using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
using Argument = xyz::openbmc_project::Common::InvalidArgument;

LDAPMapperEntry::LDAPMapperEntry(sdbusplus::bus::bus& bus, const char* path,
                                 const char* filePath,
                                 const std::string& groupName,
                                 const std::string& privilege, Config& parent) :
    Interfaces(bus, path, Interfaces::action::defer_emit),
    id(std::stol(std::filesystem::path(path).filename())), manager(parent),
    persistPath(filePath)
{
    dbusObjpath = path;
    Interfaces::privilege(privilege, true);
    Interfaces::groupName(groupName, true);
    Interfaces::emit_object_added();
}

LDAPMapperEntry::LDAPMapperEntry(sdbusplus::bus::bus& bus, const char* path,
                                 const char* filePath, Config& parent) :
    Interfaces(bus, path, Interfaces::action::defer_emit),
    id(std::stol(std::filesystem::path(path).filename())), manager(parent),
    persistPath(filePath)
{
    dbusObjpath = path;
}
void LDAPMapperEntry::delete_(void)
{
    manager.deletePrivilegeMapper(id);
}

std::string LDAPMapperEntry::groupName(std::string value)
{
    if (value == Interfaces::groupName())
    {
        return value;
    }

    manager.checkPrivilegeMapper(value);
    auto val = Interfaces::groupName(value);
    serialize(*this, persistPath);
    if (value == Interfaces::groupName())
    {
        // send a redfish event
        std::vector<std::string> messageArgs = {"GroupName", value};
        sendEvent(MESSAGE_TYPE::PROPERTY_VALUE_MODIFIED,
                  sdbusplus::xyz::openbmc_project::Logging::server::Entry::
                      Level::Informational,
                  messageArgs, dbusObjpath);
    }
    return val;
}

std::string LDAPMapperEntry::privilege(std::string value)
{
    if (value == Interfaces::privilege())
    {
        return value;
    }

    manager.checkPrivilegeLevel(value);
    auto val = Interfaces::privilege(value);
    serialize(*this, persistPath);
    if (value == Interfaces::privilege())
    {
        // send a redfish event
        std::vector<std::string> messageArgs = {"Privilege", value};
        sendEvent(MESSAGE_TYPE::PROPERTY_VALUE_MODIFIED,
                  sdbusplus::xyz::openbmc_project::Logging::server::Entry::
                      Level::Informational,
                  messageArgs, dbusObjpath);
    }
    return val;
}

} // namespace ldap
} // namespace phosphor