#pragma once

#include "modio/ModioSDK.h"
#include "modio/modio_service.h"

#include <cstdint>
#include <functional>

namespace nxm::modio {

/// Thin wrappers around the mod.io SDK's less-common mod management queries,
/// temp mod sets, and metrics sessions. Each checks service.ready() the same
/// way modio_user.h's wrappers do; the synchronous queries return an empty/
/// default value instead of invoking a callback when not ready.

void force_uninstall_mod(const Service &service, Modio::ModID id,
                         std::function<void(Modio::ErrorCode)> on_done);
[[nodiscard]] Modio::ErrorCode prioritize_transfer_for_mod(
    const Service &service, Modio::ModID id_to_prioritize);
[[nodiscard]] Modio::Optional<Modio::ModProgressInfo>
query_current_mod_update(const Service &service);
[[nodiscard]] std::map<Modio::ModID, Modio::ModCollectionEntry>
query_system_installations(const Service &service);
[[nodiscard]] Modio::Optional<Modio::StorageInfo>
query_storage_info(const Service &service);
void preview_external_updates(
    const Service &service,
    std::function<void(Modio::ErrorCode,
                       std::map<Modio::ModID, Modio::UserSubscriptionListChangeType>)>
        on_done);
[[nodiscard]] std::vector<std::string>
base_mod_installation_directories(const Service &service);
/// Not gated on ready() - the SDK documents this as callable without
/// initializing the SDK first.
[[nodiscard]] std::string
default_mod_installation_directory(Modio::GameID game_id);

// -- Temp mod sets ----------------------------------------------------------
// A scratch subscription set for previewing mods (e.g. a level editor) that
// installs/uninstalls independently of the user's real subscriptions.

[[nodiscard]] Modio::ErrorCode init_temp_mod_set(const Service &service,
                                                 std::vector<Modio::ModID> ids);
[[nodiscard]] Modio::ErrorCode add_to_temp_mod_set(
    const Service &service, std::vector<Modio::ModID> ids);
[[nodiscard]] Modio::ErrorCode remove_from_temp_mod_set(
    const Service &service, std::vector<Modio::ModID> ids);
[[nodiscard]] Modio::ErrorCode close_temp_mod_set(const Service &service);
[[nodiscard]] std::map<Modio::ModID, Modio::ModCollectionEntry>
query_temp_mod_set(const Service &service);

// -- Metrics sessions ---------------------------------------------------

void metrics_session_start(const Service &service,
                           Modio::MetricsSessionParams params,
                           std::function<void(Modio::ErrorCode)> on_done);
void metrics_session_send_heartbeat_once(
    const Service &service, std::function<void(Modio::ErrorCode)> on_done);
void metrics_session_send_heartbeat_at_interval(
    const Service &service, std::uint32_t interval_seconds,
    std::function<void(Modio::ErrorCode)> on_done);
void metrics_session_end(const Service &service,
                         std::function<void(Modio::ErrorCode)> on_done);

}
