#include "modio/modio_server.h"

namespace nxm::modio {

void initialize_server(Modio::ServerInitializeOptions options,
                       std::function<void(Modio::ErrorCode)> on_init_complete) {
  Modio::InitializeModioServerAsync(std::move(options),
                                    std::move(on_init_complete));
}

void install_or_update_server_mods(
    std::vector<Modio::ModID> mods,
    std::function<void(Modio::ErrorCode)> on_done) {
  Modio::InstallOrUpdateServerModsAsync(std::move(mods), std::move(on_done));
}

void register_client_mods_with_server(
    std::vector<Modio::ModID> ids,
    std::function<void(Modio::ErrorCode, std::set<Modio::ModID>)> on_done) {
  Modio::RegisterClientModsWithServerAsync(std::move(ids), std::move(on_done));
}

void clear_registered_client_mods() { Modio::ClearRegisteredClientMods(); }

std::set<Modio::ModID> registered_client_mods() {
  return Modio::GetRegisteredClientMods();
}

}
