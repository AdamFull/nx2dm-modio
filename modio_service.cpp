#include "modio/modio_service.h"

#include "core/foundation/diagnostics/log.h"

namespace nxm::modio {
namespace {

const nx::log::Category log_modio = nx::log::category("modio");

[[nodiscard]] Modio::ErrorCode not_ready_error() noexcept {
  return Modio::make_error_code(Modio::GenericError::SDKNotInitialized);
}

[[nodiscard]] nx::string_view event_name(
    const Modio::ModManagementEvent::EventType type) noexcept {
  switch (type) {
  case Modio::ModManagementEvent::EventType::Installed:
    return "installed";
  case Modio::ModManagementEvent::EventType::Updated:
    return "updated";
  case Modio::ModManagementEvent::EventType::Uninstalled:
    return "uninstalled";
  case Modio::ModManagementEvent::EventType::Uploaded:
    return "uploaded";
  }
  return "unknown";
}

}

Service::~Service() {
  if (m_phase == Phase::Ready || m_phase == Phase::Initializing ||
      m_phase == Phase::ShuttingDown)
    nx::logw(log_modio,
             "service destroyed without shutdown() completing first");
}

void Service::initialize(const ServiceConfig &config) {
  if (m_phase != Phase::Idle && m_phase != Phase::Failed) {
    nx::logw(log_modio, "initialize() called while already active");
    return;
  }
  m_phase = Phase::Initializing;
  m_mod_management_enabled = false;
  const Modio::Environment environment = config.test_environment
                                             ? Modio::Environment::Test
                                             : Modio::Environment::Live;
  Modio::InitializeAsync(
      Modio::InitializeOptions(
          Modio::GameID(config.game_id),
          Modio::ApiKey(std::string(config.api_key.view())), environment,
          Modio::Portal::None, std::string(config.session_id.view())),
      [this](const Modio::ErrorCode ec) {
        m_last_error = ec;
        m_phase = ec ? Phase::Failed : Phase::Ready;
        if (ec)
          nx::loge(log_modio, "initialize failed: {}", ec.message());
        else
          nx::logi(log_modio, "initialized");
      });
}

void Service::shutdown() {
  if (m_phase == Phase::Idle) {
    return;
  }
  if (m_phase == Phase::Failed) {
    m_phase = Phase::Idle;
    return;
  }
  if (m_phase != Phase::Ready) {
    nx::logw(log_modio, "shutdown() called while not ready ({}); ignored",
             m_phase == Phase::Initializing ? "still initializing"
                                            : "already shutting down");
    return;
  }
  if (m_mod_management_enabled)
    disable_mod_management();
  m_phase = Phase::ShuttingDown;
  Modio::ShutdownAsync([this](const Modio::ErrorCode ec) {
    m_last_error = ec;
    if (ec)
      nx::loge(log_modio, "shutdown failed: {}", ec.message());
    m_phase = Phase::Idle;
  });
}

void Service::pump() {
  if (m_phase == Phase::Idle)
    return;
  Modio::RunPendingHandlers();
}

bool Service::authenticated() const {
  return ready() && Modio::QueryUserProfile().has_value();
}

bool Service::enable_mod_management() {
  if (!ready())
    return false;
  if (m_mod_management_enabled)
    return true;
  const Modio::ErrorCode ec =
      Modio::EnableModManagement([this](const Modio::ModManagementEvent event) {
        on_mod_management_event(event);
      });
  if (ec) {
    nx::loge(log_modio, "cannot enable mod management: {}", ec.message());
    return false;
  }
  m_mod_management_enabled = true;
  return true;
}

void Service::disable_mod_management() {
  if (!m_mod_management_enabled)
    return;
  Modio::DisableModManagement();
  m_mod_management_enabled = false;
}

bool Service::mod_management_busy() const {
  return ready() && Modio::IsModManagementBusy();
}

void Service::on_mod_management_event(const Modio::ModManagementEvent event) {
  const auto mod_id = static_cast<Modio::ModID::UnderlyingType>(event.ID);
  if (event.Status)
    nx::logw(log_modio, "mod {} {} failed: {}", mod_id, event_name(event.Event),
             event.Status.message());
  else
    nx::logi(log_modio, "mod {} {}", mod_id, event_name(event.Event));
}

std::function<void(Modio::ErrorCode)>
Service::tracked(std::function<void(Modio::ErrorCode)> on_done) {
  m_last_op_busy = true;
  m_last_op_error.clear();
  return [this, on_done = std::move(on_done)](const Modio::ErrorCode ec) {
    m_last_op_busy = false;
    m_last_op_error = ec ? nx::string(ec.message()) : nx::string();
    if (on_done)
      on_done(ec);
  };
}

void Service::fail_not_ready(
    const std::function<void(Modio::ErrorCode)> &on_done) {
  const Modio::ErrorCode ec = not_ready_error();
  m_last_op_busy = false;
  m_last_op_error = nx::string(ec.message());
  if (on_done)
    on_done(ec);
}

void Service::request_email_code(
    const nx::string_view email,
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!ready())
    return fail_not_ready(on_done);
  Modio::RequestEmailAuthCodeAsync(Modio::EmailAddress(std::string(email)),
                                   tracked(std::move(on_done)));
}

void Service::authenticate_email_code(
    const nx::string_view code, std::function<void(Modio::ErrorCode)> on_done) {
  if (!ready())
    return fail_not_ready(on_done);
  Modio::AuthenticateUserEmailAsync(Modio::EmailAuthCode(std::string(code)),
                                    tracked(std::move(on_done)));
}

void Service::verify_authentication(
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!ready())
    return fail_not_ready(on_done);
  Modio::VerifyUserAuthenticationAsync(tracked(std::move(on_done)));
}

void Service::subscribe(const Modio::ModID id, const bool include_dependencies,
                        std::function<void(Modio::ErrorCode)> on_done) {
  if (!ready() || !m_mod_management_enabled)
    return fail_not_ready(on_done);
  Modio::SubscribeToModAsync(id, include_dependencies, tracked(std::move(on_done)));
}

void Service::unsubscribe(const Modio::ModID id,
                          std::function<void(Modio::ErrorCode)> on_done) {
  if (!ready() || !m_mod_management_enabled)
    return fail_not_ready(on_done);
  Modio::UnsubscribeFromModAsync(id, tracked(std::move(on_done)));
}

void Service::fetch_external_updates(
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!ready())
    return fail_not_ready(on_done);
  Modio::FetchExternalUpdatesAsync(tracked(std::move(on_done)));
}

std::map<Modio::ModID, Modio::ModCollectionEntry> Service::subscriptions() const {
  return ready() ? Modio::QueryUserSubscriptions()
                 : std::map<Modio::ModID, Modio::ModCollectionEntry>{};
}

std::map<Modio::ModID, Modio::ModCollectionEntry>
Service::installations(const bool include_outdated) const {
  return ready() ? Modio::QueryUserInstallations(include_outdated)
                 : std::map<Modio::ModID, Modio::ModCollectionEntry>{};
}

}
