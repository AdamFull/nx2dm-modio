#include "framework/nxtest.h"

#include "modio/modio_service.h"

using nxm::modio::Phase;
using nxm::modio::Service;

TEST_CASE("modio service: starts idle and unauthenticated") {
  Service service;
  CHECK(service.phase() == Phase::Idle);
  CHECK_FALSE(service.ready());
  CHECK_FALSE(service.authenticated());
  CHECK_FALSE(service.mod_management_enabled());
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
