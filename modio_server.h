#pragma once

#include "modio/ModioSDK.h"

#include <functional>

namespace nxm::modio {

/// Thin wrappers around the mod.io SDK's dedicated-server API. This is a
/// separate initialization path (Modio::InitializeModioServerAsync with
/// Modio::ServerInitializeOptions, not Service::initialize's regular client
/// flow) meant for a headless server process hosting mods for clients to
/// register against - it is not gated on Service::ready(), matching the
/// SDK's own docs, which document no such requirement for these calls.

void initialize_server(Modio::ServerInitializeOptions options,
                       std::function<void(Modio::ErrorCode)> on_init_complete);
void install_or_update_server_mods(
    std::vector<Modio::ModID> mods,
    std::function<void(Modio::ErrorCode)> on_done);
void register_client_mods_with_server(
    std::vector<Modio::ModID> ids,
    std::function<void(Modio::ErrorCode, std::set<Modio::ModID>)> on_done);
void clear_registered_client_mods();
[[nodiscard]] std::set<Modio::ModID> registered_client_mods();

}
