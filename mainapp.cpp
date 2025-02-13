#include "config.h"

#include "user_mgr.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

#include <string>

// D-Bus root for user manager
constexpr auto userManagerRoot = "/xyz/openbmc_project/user";
using namespace phosphor::logging;

int main(int /*argc*/, char** /*argv*/)
{
    auto bus = sdbusplus::bus::new_default();
    sdbusplus::server::manager_t objManager(bus, userManagerRoot);

    try
    {
        phosphor::user::UserMgr userMgr(bus, userManagerRoot);

        // Claim the bus now
        bus.request_name(USER_MANAGER_BUSNAME);

        // Wait for client request
        bus.process_loop();
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Exception occurred during User Manager initialization: {ERR}",
            "ERR", e);
        return -1;
    }
    catch (...)
    {
        lg2::error("Exception occurred during User Manager initialization");
        return -1;
    }
    return 0;
}
