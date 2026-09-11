#pragma once

#include "modio/ModioSDK.h"

#include "core/foundation/strings/utf8_string.h"

#include <functional>
#include <map>

namespace nxm::modio {

struct ServiceConfig {
  i64 game_id = 0;
  nx::string api_key;
  bool test_environment = false;
  nx::string session_id = "nx2d";
};

enum class Phase : u8 { Idle, Initializing, Ready, ShuttingDown, Failed };

/// Thin lifecycle wrapper around the mod.io SDK's global Modio:: API. The SDK
/// itself owns all state behind free functions, not an instance this class
/// constructs; what this adds is sequencing (don't call SDK functions before
/// InitializeAsync's callback fires, or after ShutdownAsync's has been
/// requested) and mod-management-event bookkeeping. Any C++ caller may still
/// call Modio::* directly once ready() is true - richer queries this doesn't
/// wrap (ListAllModsAsync, GetModInfoAsync, ...) are meant to be called that
/// way.
class Service {
public:
  Service() = default;
  ~Service();
  Service(const Service &) = delete;
  Service &operator=(const Service &) = delete;

  void initialize(const ServiceConfig &config);
  /// Requests an async shutdown. Keep calling pump() until phase() is no
  /// longer ShuttingDown - the SDK needs its event loop pumped to unwind.
  void shutdown();
  void pump();

  [[nodiscard]] Phase phase() const noexcept { return m_phase; }
  [[nodiscard]] bool ready() const noexcept { return m_phase == Phase::Ready; }
  [[nodiscard]] Modio::ErrorCode last_error() const noexcept {
    return m_last_error;
  }

  [[nodiscard]] bool authenticated() const;

  /// Registers this service's mod management event handler. Required once,
  /// after ready(), before subscribe()/unsubscribe() can do anything - the
  /// SDK refuses those calls until mod management is enabled. Idempotent.
  bool enable_mod_management();
  void disable_mod_management();
  [[nodiscard]] bool mod_management_enabled() const noexcept {
    return m_mod_management_enabled;
  }
  [[nodiscard]] bool mod_management_busy() const;
  /// Cross-check against the SDK's own view, rather than the flag above
  /// (which only reflects calls made through this Service). ready() must be
  /// true; returns false otherwise.
  [[nodiscard]] bool sdk_mod_management_enabled() const;

  /// Adjusts the SDK's own log verbosity. Not gated on ready(); safe to call
  /// before initialize(). initialize() always wires the SDK's log callback
  /// into this module's "modio" log category regardless.
  static void set_log_level(Modio::LogLevel level);

  void request_email_code(nx::string_view email,
                          std::function<void(Modio::ErrorCode)> on_done = {});
  void authenticate_email_code(nx::string_view code,
                               std::function<void(Modio::ErrorCode)> on_done = {});
  void verify_authentication(std::function<void(Modio::ErrorCode)> on_done = {});

  void subscribe(Modio::ModID id, bool include_dependencies,
                std::function<void(Modio::ErrorCode)> on_done = {});
  void unsubscribe(Modio::ModID id,
                   std::function<void(Modio::ErrorCode)> on_done = {});
  void fetch_external_updates(std::function<void(Modio::ErrorCode)> on_done = {});

  /// name_contains empty means "no name filter". Results are paginated;
  /// start_index/count follow Modio::FilterParams::IndexedResults.
  void search_mods(
      nx::string_view name_contains, usize start_index, usize count,
      std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModInfoList>)>
          on_done);
  void get_mod_info(
      Modio::ModID id,
      std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModInfo>)>
          on_done);

  /// The mod's local install directory, once QueryUserInstallations reports
  /// it present - i.e. the path a game would pass to Engine::mount_overlay.
  /// Empty before ready() or if the mod is not currently installed.
  [[nodiscard]] Modio::Optional<std::string>
  installed_mod_path(Modio::ModID id) const;
  [[nodiscard]] bool is_subscribed(Modio::ModID id) const;
  [[nodiscard]] bool is_installed(Modio::ModID id) const;

  /// Convenience mirror of whichever of the above ran most recently, for a
  /// caller (the Luau surface) that polls instead of holding a callback.
  /// Callers that already have their own callback should just use its
  /// ErrorCode directly - this can race between two independent callers.
  [[nodiscard]] bool last_operation_busy() const noexcept {
    return m_last_op_busy;
  }
  [[nodiscard]] nx::string_view last_operation_error() const noexcept {
    return m_last_op_error.view();
  }

  [[nodiscard]] std::map<Modio::ModID, Modio::ModCollectionEntry>
  subscriptions() const;
  [[nodiscard]] std::map<Modio::ModID, Modio::ModCollectionEntry>
  installations(bool include_outdated) const;

private:
  void on_mod_management_event(Modio::ModManagementEvent event);
  [[nodiscard]] std::function<void(Modio::ErrorCode)>
  tracked(std::function<void(Modio::ErrorCode)> on_done);
  void fail_not_ready(const std::function<void(Modio::ErrorCode)> &on_done);

  Phase m_phase = Phase::Idle;
  Modio::ErrorCode m_last_error;
  bool m_mod_management_enabled = false;
  bool m_last_op_busy = false;
  nx::string m_last_op_error;
};

}
