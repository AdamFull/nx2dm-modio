#include "modio/modio_advanced.h"

#include "modio/modio_errors.h"

namespace nxm::modio {

void force_uninstall_mod(const Service &service, const Modio::ModID id,
                         std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::ForceUninstallModAsync(id, std::move(on_done));
}

Modio::ErrorCode prioritize_transfer_for_mod(const Service &service,
                                             const Modio::ModID id_to_prioritize) {
  return service.ready() ? Modio::PrioritizeTransferForMod(id_to_prioritize)
                         : not_ready_error();
}

Modio::Optional<Modio::ModProgressInfo>
query_current_mod_update(const Service &service) {
  return service.ready() ? Modio::QueryCurrentModUpdate()
                         : Modio::Optional<Modio::ModProgressInfo>{};
}

std::map<Modio::ModID, Modio::ModCollectionEntry>
query_system_installations(const Service &service) {
  return service.ready() ? Modio::QuerySystemInstallations()
                         : std::map<Modio::ModID, Modio::ModCollectionEntry>{};
}

Modio::Optional<Modio::StorageInfo> query_storage_info(const Service &service) {
  if (!service.ready())
    return {};
  return Modio::QueryStorageInfo();
}

void preview_external_updates(
    const Service &service,
    std::function<void(Modio::ErrorCode,
                       std::map<Modio::ModID, Modio::UserSubscriptionListChangeType>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::PreviewExternalUpdatesAsync(std::move(on_done));
}

std::vector<std::string> base_mod_installation_directories(const Service &service) {
  return service.ready() ? Modio::GetBaseModInstallationDirectories()
                         : std::vector<std::string>{};
}

std::string default_mod_installation_directory(const Modio::GameID game_id) {
  return Modio::GetDefaultModInstallationDirectory(game_id);
}

Modio::ErrorCode init_temp_mod_set(const Service &service,
                                   std::vector<Modio::ModID> ids) {
  return service.ready() ? Modio::InitTempModSet(std::move(ids))
                         : not_ready_error();
}

Modio::ErrorCode add_to_temp_mod_set(const Service &service,
                                     std::vector<Modio::ModID> ids) {
  return service.ready() ? Modio::AddToTempModSet(std::move(ids))
                         : not_ready_error();
}

Modio::ErrorCode remove_from_temp_mod_set(const Service &service,
                                          std::vector<Modio::ModID> ids) {
  return service.ready() ? Modio::RemoveFromTempModSet(std::move(ids))
                         : not_ready_error();
}

Modio::ErrorCode close_temp_mod_set(const Service &service) {
  return service.ready() ? Modio::CloseTempModSet() : not_ready_error();
}

std::map<Modio::ModID, Modio::ModCollectionEntry>
query_temp_mod_set(const Service &service) {
  return service.ready() ? Modio::QueryTempModSet()
                         : std::map<Modio::ModID, Modio::ModCollectionEntry>{};
}

void metrics_session_start(const Service &service,
                           Modio::MetricsSessionParams params,
                           std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::MetricsSessionStartAsync(std::move(params), std::move(on_done));
}

void metrics_session_send_heartbeat_once(
    const Service &service, std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::MetricsSessionSendHeartbeatOnceAsync(std::move(on_done));
}

void metrics_session_send_heartbeat_at_interval(
    const Service &service, const std::uint32_t interval_seconds,
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::MetricsSessionSendHeartbeatAtIntervalAsync(interval_seconds,
                                                     std::move(on_done));
}

void metrics_session_end(const Service &service,
                         std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::MetricsSessionEndAsync(std::move(on_done));
}

}
