#include "framework/nxtest.h"

#include "app/engine.h"
#include "core/foundation/platform/filesystem.h"
#include "script/luau/luau_backend.h"
#include "script/luau/luau_bindings.h"
#include "script/script_host.h"
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
};

} // namespace

TEST_CASE("modio scripting: every service is exposed as script-services.json "
          "declares it") {
  const Exposed exposed;
  const auto manifest = nx::fs::file_read_text(
      nx::fs::path_view(NX_MODULE_SERVICES_MANIFEST));
  REQUIRE(manifest);

  nx::string error;
  if (!script::luau_manifest_agrees(manifest.value(), exposed.services, error))
    FAIL(error.c_str());
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

// No mod.io service is registered here, so every list is empty - but each
// arrives as a table a script can walk, agreeing with its count.
TEST_CASE("modio scripting: the mod lists come back as tables") {
  nxe::Engine engine{nxe::Game{}};
  nxe::ModuleContext ctx{engine};
  script::Host host;
  REQUIRE(host.set_backend(script::luau_backend()));
  expose_modio_services(host, ctx);
  REQUIRE(host.bind());
  const nx::string_view source = R"(
assert(#host.modio_subscribed_mods() == host.modio_subscribed_count(), "subscribed")
assert(#host.modio_installed_mods() == host.modio_installed_count(), "installed")
assert(#host.modio_purchased_mods() == host.modio_purchased_mods_count(), "purchased")
return {}
)";
  CHECK(host.load("modio_lists",
                  {reinterpret_cast<const std::byte *>(source.data()),
                   source.size()}));
}
