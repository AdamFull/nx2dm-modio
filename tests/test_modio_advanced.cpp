#include "framework/nxtest.h"

#include "modio/modio_advanced.h"
#include "modio/modio_monetization.h"

using nxm::modio::Service;

TEST_CASE("modio advanced: synchronous queries fail before ready") {
  const Service service;
  CHECK(nxm::modio::prioritize_transfer_for_mod(service, Modio::ModID(1)));
  CHECK_FALSE(nxm::modio::query_current_mod_update(service).has_value());
  CHECK(nxm::modio::query_system_installations(service).empty());
  CHECK_FALSE(nxm::modio::query_storage_info(service).has_value());
  CHECK(nxm::modio::base_mod_installation_directories(service).empty());
  CHECK(nxm::modio::init_temp_mod_set(service, {Modio::ModID(1)}));
  CHECK(nxm::modio::query_temp_mod_set(service).empty());
}

TEST_CASE("modio advanced: async calls fail synchronously before ready") {
  const Service service;
  bool called = false;
  nxm::modio::force_uninstall_mod(service, Modio::ModID(1),
                                  [&](const Modio::ErrorCode ec) {
                                    called = true;
                                    CHECK(ec);
                                  });
  CHECK(called);

  called = false;
  nxm::modio::metrics_session_start(
      service, Modio::MetricsSessionParams{}, [&](const Modio::ErrorCode ec) {
        called = true;
        CHECK(ec);
      });
  CHECK(called);
}

TEST_CASE("modio monetization: calls fail synchronously before ready") {
  const Service service;
  bool called = false;
  nxm::modio::purchase_mod(
      service, Modio::ModID(1), {},
      [&](const Modio::ErrorCode ec,
          const Modio::Optional<Modio::TransactionRecord> record) {
        called = true;
        CHECK(ec);
        CHECK_FALSE(record.has_value());
      });
  CHECK(called);

  CHECK(nxm::modio::query_user_purchased_mods(service).empty());
}
