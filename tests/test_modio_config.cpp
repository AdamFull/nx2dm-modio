#include "framework/nxtest.h"

#include "modio/modio_config.h"

#include "core/foundation/vfs/vfs.h"

namespace {

using nxm::modio::load_project_config;

struct VfsScope {
  VfsScope() { initialized = nx::vfs::initialize(); }
  ~VfsScope() { nx::vfs::shutdown(); }
  bool initialized = false;
};

[[nodiscard]] nx::blob<u8> as_bytes(const nx::string_view text) {
  return nx::blob<u8>(
      {reinterpret_cast<const u8 *>(text.data()), text.size()});
}

} // namespace

TEST_CASE("modio config: no file present is not an error") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("modio config: a valid file is parsed in full") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/modio.ini", as_bytes(R"(
[modio]
game_id = 3609
api_key = ca842a1f60c40bc8fb2044bc9932d763
test_environment = on
session_id = my-game
)"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  const Modio::Optional<nxm::modio::ServiceConfig> config =
      load_project_config();
  REQUIRE(config.has_value());
  CHECK(config->game_id == 3609);
  CHECK(config->api_key.view() == "ca842a1f60c40bc8fb2044bc9932d763");
  CHECK(config->test_environment);
  CHECK(config->session_id.view() == "my-game");

  nx::vfs::unmount(mount);
}

TEST_CASE("modio config: optional fields fall back to ServiceConfig's own "
          "defaults") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/modio.ini", as_bytes(R"(
[modio]
game_id = 42
api_key = some-key
)"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  const Modio::Optional<nxm::modio::ServiceConfig> config =
      load_project_config();
  REQUIRE(config.has_value());
  CHECK(config->game_id == 42);
  CHECK(config->api_key.view() == "some-key");
  CHECK_FALSE(config->test_environment);
  CHECK(config->session_id.view() == "nx2d");

  nx::vfs::unmount(mount);
}

TEST_CASE("modio config: missing game_id or api_key is refused") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/modio.ini", as_bytes("[modio]\napi_key = only-a-key\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("modio config: a non-numeric game_id is refused") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/modio.ini",
             as_bytes("[modio]\ngame_id = not-a-number\napi_key = k\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("modio config: malformed ini does not crash") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/config/modio.ini", as_bytes("this is not [ini at all"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());

  nx::vfs::unmount(mount);
}

TEST_CASE("modio config: an explicit path overrides the default") {
  const VfsScope scope;
  REQUIRE(scope.initialized);
  nx::vfs::MemoryDevice *const memory = nx::vfs::make_memory_device();
  REQUIRE(memory != nullptr);
  memory->add("/somewhere/else.ini",
             as_bytes("[modio]\ngame_id = 7\napi_key = k\n"));
  const nx::vfs::MountId mount = nx::vfs::mount("/", memory);

  CHECK_FALSE(load_project_config().has_value());
  const Modio::Optional<nxm::modio::ServiceConfig> config =
      load_project_config("/somewhere/else.ini");
  REQUIRE(config.has_value());
  CHECK(config->game_id == 7);

  nx::vfs::unmount(mount);
}
