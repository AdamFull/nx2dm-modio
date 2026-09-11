#include "framework/nxtest.h"

#include "modio/modio_authoring.h"
#include "modio/modio_user.h"

using nxm::modio::Service;

// The SDK's global state is not injectable, so - like modio_service's own
// tests - these can only exercise the not-ready guard without a live
// connection. Each wrapper file follows the exact same shape (check
// service.ready(), forward or fail synchronously), so this samples a few
// representative functions from each rather than repeating the same check
// for all thirty-odd wrappers.

TEST_CASE("modio user: social/auth calls fail synchronously before ready") {
  const Service service;
  bool called = false;

  nxm::modio::mute_user(service, Modio::UserID(1),
                        [&](const Modio::ErrorCode ec) {
                          called = true;
                          CHECK(ec);
                        });
  CHECK(called);

  called = false;
  nxm::modio::follow_user(service, Modio::UserID(1),
                          [&](const Modio::ErrorCode ec) {
                            called = true;
                            CHECK(ec);
                          });
  CHECK(called);

  called = false;
  nxm::modio::get_user_ratings(
      service, [&](const Modio::ErrorCode ec,
                   const Modio::Optional<Modio::UserRatingList> list) {
        called = true;
        CHECK(ec);
        CHECK_FALSE(list.has_value());
      });
  CHECK(called);
}

TEST_CASE("modio user: set/get language does not require ready()") {
  // Must not crash or refuse just because the SDK is not initialized.
  nxm::modio::set_language(Modio::Language::English);
  CHECK(nxm::modio::get_language() == Modio::Language::English);
}

TEST_CASE("modio authoring: calls fail synchronously before ready") {
  const Service service;
  bool called = false;

  nxm::modio::submit_new_mod(
      service, nxm::modio::new_mod_handle(), Modio::CreateModParams{},
      [&](const Modio::ErrorCode ec, const Modio::Optional<Modio::ModID> id) {
        called = true;
        CHECK(ec);
        CHECK_FALSE(id.has_value());
      });
  CHECK(called);

  called = false;
  nxm::modio::submit_mod_rating(service, Modio::ModID(1), Modio::Rating::Positive,
                                [&](const Modio::ErrorCode ec) {
                                  called = true;
                                  CHECK(ec);
                                });
  CHECK(called);
}

TEST_CASE("modio authoring: file submission refuses without mod management") {
  const Service service;
  CHECK_FALSE(nxm::modio::submit_new_mod_file(service, Modio::ModID(1),
                                              Modio::CreateModFileParams{}));
  CHECK_FALSE(nxm::modio::submit_new_mod_source_file(
      service, Modio::ModID(1), Modio::CreateSourceFileParams{}));
}

TEST_CASE("modio authoring: last_validation_error is empty with nothing to report") {
  CHECK(nxm::modio::last_validation_error().empty());
}
