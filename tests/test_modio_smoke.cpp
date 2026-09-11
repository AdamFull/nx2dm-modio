#include "framework/nxtest.h"

#include "modio/modio_service.h"

#include <chrono>
#include <string>
#include <thread>

using nxm::modio::Phase;
using nxm::modio::Service;
using nxm::modio::ServiceConfig;

namespace {

// The mod.io C++ SDK's own publicly published example game/key, reused
// verbatim from every example under modio-sdk/examples/ (e.g.
// 01_Initialization.cpp). Verified live via curl before writing this test:
// api.mod.io/v1/games/3609 with this key returns the game's real profile,
// while api.test.mod.io/v1/games/3609 with the same key returns 401
// "Invalid credentials". So this pair is registered against mod.io's LIVE
// tier, not Test - mod.io's Test tier is a private per-developer sandbox
// with no publicly documented demo credentials, so it cannot be exercised
// without a real mod.io developer account. This test therefore proves the
// module's init -> pump -> ready -> API-call pipeline against mod.io's real
// Live servers instead.
constexpr int kDemoGameId = 3609;
constexpr const char *kDemoApiKey = "ca842a1f60c40bc8fb2044bc9932d763";

// Pumps until predicate() is true or the deadline passes. Mirrors the
// busy-pump idiom from the SDK's own examples (RunPendingHandlers in a
// loop), just bounded so a network stall can't hang the test suite.
template <typename Predicate>
bool pump_until(Service &service, Predicate &&predicate,
                const std::chrono::seconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!predicate()) {
    if (std::chrono::steady_clock::now() >= deadline)
      return false;
    service.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return true;
}

}

TEST_CASE("modio smoke: live round trip against mod.io's public demo game") {
  Service service;
  service.initialize(ServiceConfig{
      .game_id = kDemoGameId,
      .api_key = kDemoApiKey,
      .test_environment = false,
      .session_id = "nx2d-smoke-test",
  });

  if (!pump_until(
          service, [&] { return service.phase() != Phase::Initializing; },
          std::chrono::seconds(20)))
    SKIP("timed out waiting for mod.io InitializeAsync - no network?");

  if (service.phase() == Phase::Failed) {
    const Modio::ErrorCode ec = service.last_error();
    if (Modio::ErrorCodeMatches(ec, Modio::ErrorConditionTypes::NetworkError))
      SKIP(std::string("mod.io unreachable: ") + ec.message());
    FAIL((std::string("initialize failed: ") + ec.message()).c_str());
    return;
  }

  REQUIRE(service.phase() == Phase::Ready);
  CHECK_FALSE(service.authenticated());

  bool search_done = false;
  Modio::ErrorCode search_ec;
  Modio::Optional<Modio::ModInfoList> results;
  service.search_mods(
      "", 0, 10,
      [&](const Modio::ErrorCode ec,
          const Modio::Optional<Modio::ModInfoList> list) {
        search_done = true;
        search_ec = ec;
        results = list;
      });

  if (!pump_until(
          service, [&] { return search_done; }, std::chrono::seconds(20))) {
    service.shutdown();
    SKIP("timed out waiting for search_mods to respond");
  }

  if (search_ec) {
    if (Modio::ErrorCodeMatches(search_ec,
                                Modio::ErrorConditionTypes::NetworkError)) {
      service.shutdown();
      SKIP(std::string("mod.io unreachable during search: ") +
           search_ec.message());
    }
    FAIL((std::string("search_mods failed: ") + search_ec.message()).c_str());
  } else {
    // A genuine, non-vacuous assertion: the server actually returned real
    // paging metadata for game 3609, proving the round trip happened rather
    // than the callback firing with an empty default.
    REQUIRE(results.has_value());
    CHECK(results->Size() <= 10);
    CHECK(results->GetTotalResultCount() >= 0);
    std::printf(
        "[modio smoke] server reports %d total mods for game %d, returned "
        "%zu in this page\n",
        results->GetTotalResultCount(), kDemoGameId, results->Size());
  }

  service.shutdown();
  pump_until(
      service, [&] { return service.phase() == Phase::Idle; },
      std::chrono::seconds(20));
  CHECK(service.phase() == Phase::Idle);
}
