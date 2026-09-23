#include "modio/modio_config.h"
#include "modio/modio_scripting.h"
#include "modio/modio_service.h"

#include "app/engine.h"
#include "app/module_system/module.h"

#include "core/foundation/diagnostics/log.h"

namespace nxm::modio {
namespace {

constexpr nx::string_view PUMP_SYSTEM = "modio.pump";

class ModioModule final : public nxe::Module {
public:
  [[nodiscard]] nxe::ModuleDescriptor descriptor() const noexcept override {
    nxe::ModuleDescriptor out{};
    out.id = "modio";
    out.version = {1, 0, 0};
    out.platforms = nxe::ModulePlatform::All;
    return out;
  }

  bool on_register(nxe::ModuleContext &ctx) override {
    return ctx.service_registrar().provide(m_service);
  }

  void on_expose_scripts(nxe::script::Host &host,
                         nxe::ModuleContext &ctx) override {
    expose_modio_services(host, ctx);
  }

  bool on_attach(nxe::ModuleContext &ctx) override {
    if (!ctx.schedule().try_define(
            PUMP_SYSTEM, nxe::sys::SystemFn([this](const nxe::sys::Context &) {
              m_service.pump();
            }))) {
      nx::logw("modio: system '{}' is already owned by another module",
               PUMP_SYSTEM);
      return false;
    }
    ctx.schedule().add(nxe::sys::Stage::Update, PUMP_SYSTEM);
    // The SDK expects its pump on one thread, the one it was set up on.
    ctx.schedule().pin_to_main_thread(PUMP_SYSTEM);

    if (const Modio::Optional<ServiceConfig> config = load_project_config();
        config.has_value()) {
      nx::logi("modio: found {}, auto-configuring", kDefaultConfigPath);
      m_service.initialize(*config);
    } else {
      nx::logi("modio: no {} found; waiting for a script to call "
               "modio_configure",
               kDefaultConfigPath);
    }

    nx::logi("modio: attached");
    return true;
  }

  void on_detach(nxe::ModuleContext &) override {
    // Best effort: on_detach is synchronous but SDK teardown is async and
    // needs continued pump() calls to finish, which stop the moment this
    // module's system is torn down alongside it. This only runs at engine
    // shutdown, so the abandoned teardown work is harmless - the process is
    // about to exit anyway.
    m_service.shutdown();
  }

private:
  Service m_service;
};

} // namespace
} // namespace nxm::modio

NX_DECLARE_MODULE(modio, nxm::modio::ModioModule)
