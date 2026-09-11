#include "framework/nxtest.h"

#include "core/app/engine.h"
#include "core/script/script_host.h"
#include "modio/modio_scripting.h"

namespace {

using namespace nxm::modio;
namespace script = nxe::script;

struct Exposed {
  nxe::Engine engine{nxe::Game{}};
  nxe::ModuleContext ctx{engine};
  script::Host host;
  nx::vector<script::Host::ServiceInfo> services;

  Exposed() {
    expose_modio_services(host, ctx);
    services = host.services();
  }

  [[nodiscard]] const script::Host::ServiceInfo *
  find(const nx::string_view name) const {
    for (const script::Host::ServiceInfo &one : services)
      if (one.name == name)
        return &one;
    return nullptr;
  }
};

} // namespace

// This is the one place a mismatch between what modio_scripting.cpp actually
// registers and what modules/modio/script-services.json declares to Luau
// would show up: GenerateHostDeclarations.cmake types host_api.luau purely
// from the JSON, with no cross-check against the real C++ callables (see
// nxe::script::detail::spell_signature, which IS the ground truth used here
// via Host::services()). Keep this list in sync with script-services.json.
TEST_CASE("modio scripting: every service is exposed with the shape a script "
          "is told about") {
  const Exposed exposed;

  static constexpr struct {
    nx::string_view name;
    nx::string_view signature;
  } WANT[] = {
      {"modio_configure", "(number,string,boolean)->(boolean)"},
      {"modio_ready", "()->(boolean)"},
      {"modio_authenticated", "()->(boolean)"},
      {"modio_busy", "()->(boolean)"},
      {"modio_request_email_code", "(string)->(boolean)"},
      {"modio_authenticate_email_code", "(string)->(boolean)"},
      {"modio_enable_mod_management", "()->(boolean)"},
      {"modio_subscribe", "(number)->(boolean)"},
      {"modio_unsubscribe", "(number)->(boolean)"},
      {"modio_is_subscribed", "(number)->(boolean)"},
      {"modio_is_installed", "(number)->(boolean)"},
      {"modio_subscribed_count", "()->(number)"},
      {"modio_installed_count", "()->(number)"},
      {"modio_op_busy", "()->(boolean)"},
      {"modio_op_error", "()->(string)"},
      {"modio_op_result_id", "()->(number)"},
      {"modio_op_result_text", "()->(string)"},
      {"modio_set_language", "(number)->(boolean)"},
      {"modio_get_language", "()->(number)"},
      {"modio_clear_user_data", "()->(boolean)"},
      {"modio_refresh_user_data", "()->(boolean)"},
      {"modio_get_user_media", "(number)->(boolean)"},
      {"modio_mute_user", "(number)->(boolean)"},
      {"modio_unmute_user", "(number)->(boolean)"},
      {"modio_follow_user", "(number)->(boolean)"},
      {"modio_unfollow_user", "(number)->(boolean)"},
      {"modio_new_mod_handle", "()->(number)"},
      {"modio_submit_new_mod", "(number,string,string,string)->(boolean)"},
      {"modio_submit_mod_changes",
       "(number,string,string,string,string)->(boolean)"},
      {"modio_submit_new_mod_file", "(number,string,string,string)->(boolean)"},
      {"modio_submit_new_mod_source_file",
       "(number,string,string,string)->(boolean)"},
      {"modio_get_mod_logo", "(number,number)->(boolean)"},
      {"modio_get_mod_gallery_image", "(number,number,number)->(boolean)"},
      {"modio_get_mod_creator_avatar", "(number,number)->(boolean)"},
      {"modio_add_or_update_mod_logo", "(number,string)->(boolean)"},
      {"modio_submit_mod_rating", "(number,number)->(boolean)"},
      {"modio_add_mod_dependency", "(number,number)->(boolean)"},
      {"modio_delete_mod_dependency", "(number,number)->(boolean)"},
      {"modio_archive_mod", "(number)->(boolean)"},
      {"modio_has_validation_error", "()->(boolean)"},
      {"modio_purchase_mod", "(number,number)->(boolean)"},
      {"modio_fetch_wallet_balance", "()->(boolean)"},
      {"modio_fetch_user_purchases", "()->(boolean)"},
      {"modio_purchased_mods_count", "()->(number)"},
      {"modio_force_uninstall_mod", "(number)->(boolean)"},
      {"modio_prioritize_transfer_for_mod", "(number)->(boolean)"},
      {"modio_current_update_mod_id", "()->(number)"},
      {"modio_current_update_state", "()->(number)"},
      {"modio_current_update_progress", "()->(number)"},
      {"modio_storage_consumed_bytes", "()->(number)"},
      {"modio_default_install_directory", "(number)->(string)"},
      {"modio_get_mod_collection_info", "(number)->(boolean)"},
      {"modio_subscribe_to_mod_collection", "(number)->(boolean)"},
      {"modio_unsubscribe_from_mod_collection", "(number)->(boolean)"},
      {"modio_follow_mod_collection", "(number)->(boolean)"},
      {"modio_unfollow_mod_collection", "(number)->(boolean)"},
      {"modio_submit_mod_collection_rating", "(number,number)->(boolean)"},
      {"modio_get_mod_collection_logo", "(number,number)->(boolean)"},
      {"modio_get_mod_collection_creator_avatar",
       "(number,number)->(boolean)"},
  };

  CHECK(exposed.services.size() == nx::array_size(WANT));
  for (const auto &want : WANT) {
    const script::Host::ServiceInfo *const found = exposed.find(want.name);
    REQUIRE(found != nullptr);
    CHECK(found->signature == want.signature);
  }
}

TEST_CASE("modio scripting: the module hands them over on its own") {
  std::unique_ptr<nxe::Module> found;
  for (const nxe::ModuleFactory factory : nxe::enabled_module_factories()) {
    std::unique_ptr<nxe::Module> module = factory();
    if (module != nullptr && module->name() == "modio")
      found = std::move(module);
  }
  REQUIRE(found != nullptr);

  nxe::Engine engine{nxe::Game{}};
  nxe::ModuleContext ctx{engine};
  script::Host host;
  found->on_expose_scripts(host, ctx);

  script::Host direct;
  expose_modio_services(direct, ctx);
  CHECK(host.exposed_count() == direct.exposed_count());
  CHECK(host.exposed_count() > 0u);
}
