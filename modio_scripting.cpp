#include "modio/modio_scripting.h"

#include "modio/modio_service.h"

#include "core/app/engine.h"
#include "core/script/script_host.h"

namespace nxm::modio {
namespace {

[[nodiscard]] Service *service_of(nxe::ModuleContext &ctx) {
  return ctx.services().find<Service>();
}

} // namespace

void expose_modio_services(nxe::script::Host &host, nxe::ModuleContext &ctx) {
  host.expose_as("modio_configure", [&ctx](const f64 game_id,
                                           const nx::string_view api_key,
                                           const bool test_environment) {
    Service *const service = service_of(ctx);
    if (service == nullptr || service->phase() != Phase::Idle)
      return false;
    ServiceConfig config;
    config.game_id = nx::cast<i64>(game_id);
    config.api_key = nx::string(api_key);
    config.test_environment = test_environment;
    service->initialize(config);
    return true;
  });

  host.expose_as("modio_ready", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->ready();
  });

  host.expose_as("modio_authenticated", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->authenticated();
  });

  host.expose_as("modio_busy", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->last_operation_busy();
  });

  host.expose_as("modio_request_email_code", [&ctx](const nx::string_view email) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    service->request_email_code(email);
    return true;
  });

  host.expose_as("modio_authenticate_email_code",
                [&ctx](const nx::string_view code) {
                  Service *const service = service_of(ctx);
                  if (service == nullptr)
                    return false;
                  service->authenticate_email_code(code);
                  return true;
                });

  host.expose_as("modio_enable_mod_management", [&ctx]() {
    Service *const service = service_of(ctx);
    return service != nullptr && service->enable_mod_management();
  });

  host.expose_as("modio_subscribe", [&ctx](const f64 mod_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    service->subscribe(Modio::ModID(nx::cast<i64>(mod_id)), false);
    return true;
  });

  host.expose_as("modio_unsubscribe", [&ctx](const f64 mod_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    service->unsubscribe(Modio::ModID(nx::cast<i64>(mod_id)));
    return true;
  });

  host.expose_as("modio_is_subscribed", [&ctx](const f64 mod_id) {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->is_subscribed(Modio::ModID(
                                     nx::cast<i64>(mod_id)));
  });

  host.expose_as("modio_is_installed", [&ctx](const f64 mod_id) {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->is_installed(Modio::ModID(
                                     nx::cast<i64>(mod_id)));
  });

  host.expose_as("modio_subscribed_count", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service == nullptr
              ? 0.0
              : nx::cast<f64>(service->subscriptions().size());
  });

  host.expose_as("modio_installed_count", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service == nullptr
              ? 0.0
              : nx::cast<f64>(service->installations(true).size());
  });
}

}
