#include "framework/nxtest.h"

#include "modio/modio_service.h"

using nxm::modio::Phase;
using nxm::modio::Service;

TEST_CASE("modio service: set_log_level works before any initialize") {
  // Must not crash or require ready(); it's a direct SDK passthrough.
  Service::set_log_level(Modio::LogLevel::Warning);
}

TEST_CASE("modio service: starts idle and unauthenticated") {
  Service service;
  CHECK(service.phase() == Phase::Idle);
  CHECK_FALSE(service.ready());
  CHECK_FALSE(service.authenticated());
  CHECK_FALSE(service.mod_management_enabled());
  CHECK_FALSE(service.sdk_mod_management_enabled());
  CHECK_FALSE(service.last_operation_busy());
}

TEST_CASE("modio service: enable_mod_management refuses before ready") {
  Service service;
  CHECK_FALSE(service.enable_mod_management());
  CHECK_FALSE(service.mod_management_enabled());
}

TEST_CASE("modio service: subscribe/unsubscribe fail synchronously before ready") {
  Service service;
  bool called = false;
  bool had_error = false;
  service.subscribe(Modio::ModID(1), false,
                    [&](const Modio::ErrorCode ec) {
                      called = true;
                      had_error = static_cast<bool>(ec);
                    });
  CHECK(called);
  CHECK(had_error);
  CHECK_FALSE(service.last_operation_busy());

  called = false;
  had_error = false;
  service.unsubscribe(Modio::ModID(1), [&](const Modio::ErrorCode ec) {
    called = true;
    had_error = static_cast<bool>(ec);
  });
  CHECK(called);
  CHECK(had_error);
}

TEST_CASE("modio service: auth calls fail synchronously before ready") {
  Service service;
  bool called = false;
  service.request_email_code("player@example.com",
                             [&](const Modio::ErrorCode ec) {
                               called = true;
                               CHECK(ec);
                             });
  CHECK(called);

  called = false;
  service.authenticate_email_code("123456", [&](const Modio::ErrorCode ec) {
    called = true;
    CHECK(ec);
  });
  CHECK(called);

  called = false;
  service.verify_authentication([&](const Modio::ErrorCode ec) {
    called = true;
    CHECK(ec);
  });
  CHECK(called);
}

TEST_CASE("modio service: queries return empty before ready") {
  const Service service;
  CHECK(service.subscriptions().empty());
  CHECK(service.installations(true).empty());
  CHECK_FALSE(service.mod_management_busy());
  CHECK_FALSE(service.installed_mod_path(Modio::ModID(1)).has_value());
  CHECK_FALSE(service.is_subscribed(Modio::ModID(1)));
  CHECK_FALSE(service.is_installed(Modio::ModID(1)));
}

TEST_CASE("modio service: browsing calls fail synchronously before ready") {
  Service service;
  bool called = false;
  service.search_mods("", 0, 20,
                      [&](const Modio::ErrorCode ec,
                          const Modio::Optional<Modio::ModInfoList> list) {
                        called = true;
                        CHECK(ec);
                        CHECK_FALSE(list.has_value());
                      });
  CHECK(called);

  called = false;
  service.get_mod_info(
      Modio::ModID(1), [&](const Modio::ErrorCode ec,
                           const Modio::Optional<Modio::ModInfo> info) {
        called = true;
        CHECK(ec);
        CHECK_FALSE(info.has_value());
      });
  CHECK(called);
}

TEST_CASE("modio service: a callback-less call is safe") {
  Service service;
  // No on_done supplied; must not crash even though the operation fails
  // synchronously before ready().
  service.request_email_code("player@example.com");
  service.subscribe(Modio::ModID(1), false);
  CHECK_FALSE(service.last_operation_busy());
}

TEST_CASE("modio service: shutdown before any initialize is a no-op") {
  Service service;
  service.shutdown();
  CHECK(service.phase() == Phase::Idle);
}

TEST_CASE(
    "modio service: the pump runs every frame while starting or stopping") {
  using nxm::modio::pump_due;
  CHECK(pump_due(Phase::Initializing, false, std::chrono::milliseconds(0)));
  CHECK(pump_due(Phase::ShuttingDown, false, std::chrono::milliseconds(0)));
  CHECK_FALSE(pump_due(Phase::Idle, true, std::chrono::seconds(10)));
}

TEST_CASE("modio service: an idle service pumps only at the idle interval") {
  using nxm::modio::IDLE_PUMP_INTERVAL;
  using nxm::modio::pump_due;
  for (const Phase phase : {Phase::Ready, Phase::Failed}) {
    CHECK_FALSE(pump_due(phase, false, std::chrono::milliseconds(0)));
    CHECK_FALSE(pump_due(phase, false,
                         IDLE_PUMP_INTERVAL - std::chrono::milliseconds(1)));
    CHECK(pump_due(phase, false, IDLE_PUMP_INTERVAL));
    CHECK(pump_due(phase, true, std::chrono::milliseconds(0)));
  }
}

TEST_CASE("modio service: an idle service never asks for a pump") {
  Service service;
  CHECK_FALSE(service.pump_due(std::chrono::steady_clock::now()));
}

TEST_CASE("modio service: a tracked callback holds the pump until it runs") {
  Service service;
  int calls = 0;
  std::function<void(Modio::ErrorCode)> callback =
      service.track(std::function<void(Modio::ErrorCode)>(
          [&](Modio::ErrorCode) { ++calls; }));
  CHECK(service.pending_operations() == 1u);

  callback(Modio::ErrorCode{});
  CHECK(calls == 1);
  CHECK(service.pending_operations() == 0u);

  // An SDK that called back twice must not take another operation's count.
  callback(Modio::ErrorCode{});
  CHECK(calls == 2);
  CHECK(service.pending_operations() == 0u);
}

TEST_CASE(
    "modio service: a tracked callback dropped uncalled releases the pump") {
  Service service;
  {
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModInfo>)>
        callback = service.track(
            std::function<void(Modio::ErrorCode,
                               Modio::Optional<Modio::ModInfo>)>());
    const auto copy = callback;
    callback = nullptr;
    CHECK(service.pending_operations() == 1u);
  }
  CHECK(service.pending_operations() == 0u);
}
