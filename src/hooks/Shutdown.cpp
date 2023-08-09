/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <hacks/hacklist.hpp>
#include <settings/Bool.hpp>
#include "HookedMethods.hpp"
#include "MiscTemporary.hpp"
#include "votelogger.hpp"

static settings::Boolean auto_abandon{ "misc.auto-abandon", "false" };
static settings::String custom_disconnect_reason{ "misc.disconnect-reason", "" };
settings::Boolean random_name{ "misc.random-name", "false" };
extern std::string name_forced;

namespace hooked_methods
{

static TextFile randomnames_file;

DEFINE_HOOKED_METHOD(Shutdown, void, INetChannel *this_, const char *reason)
{
    g_Settings.bInvalid = true;
    logging::Info("Disconnect: %s", reason);
#if ENABLE_IPC
    ipc::UpdateServerAddress(true);
#endif
    if (isHackActive() && (*custom_disconnect_reason).length() > 3 && strstr(reason, "user"))
        original::Shutdown(this_, (*custom_disconnect_reason).c_str());
    else
        original::Shutdown(this_, reason);
    if (*auto_abandon && !ignoredc)
        tfmm::DisconnectAndAbandon();
    ignoredc = false;
    hacks::autojoin::OnShutdown();
    std::string message = reason;
    votelogger::onShutdown(message);
}

} // namespace hooked_methods