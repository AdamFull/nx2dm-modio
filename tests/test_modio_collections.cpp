#include "framework/nxtest.h"

#include "modio/modio_collections.h"

using nxm::modio::Service;

TEST_CASE("modio collections: calls fail synchronously before ready") {
  const Service service;
  bool called = false;

  nxm::modio::get_mod_collection_info(
      service, Modio::ModCollectionID(1),
      [&](const Modio::ErrorCode ec,
          const Modio::Optional<Modio::ModCollectionInfo> info) {
        called = true;
        CHECK(ec);
        CHECK_FALSE(info.has_value());
      });
  CHECK(called);

  called = false;
  nxm::modio::subscribe_to_mod_collection(service, Modio::ModCollectionID(1),
                                          [&](const Modio::ErrorCode ec) {
                                            called = true;
                                            CHECK(ec);
                                          });
  CHECK(called);
}
