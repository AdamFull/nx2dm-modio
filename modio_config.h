#pragma once

#include "modio/modio_service.h"

namespace nxm::modio {

/// VFS path ModioModule::on_attach() checks automatically. A project enables
/// the module by cooking a file to this path (author it at
/// assets/config/modio.ini) with its own game ID and API key from
/// https://mod.io/apikey - e.g.:
///
///   [modio]
///   game_id = 3609
///   api_key = ca842a1f60c40bc8fb2044bc9932d763
///   test_environment = off
///   session_id = my-game
///
/// game_id and api_key are required; test_environment (on/off/true/false/1/0,
/// default off) and session_id (default "nx2d") are optional. A project with
/// no need for this (no file present, or one that wants to authenticate a
/// different game per launch) can skip it entirely and call the Luau
/// "modio_configure" host function instead - the two are independent, and
/// ModioModule only auto-initializes when a file is actually found.
inline constexpr nx::string_view kDefaultConfigPath = "/config/modio.ini";

/// Reads and validates an INI file at @p path. Empty if the file does not
/// exist; also empty (with a logged warning) if it exists but fails to parse
/// or is missing a required field - callers should treat both the same way,
/// as "nothing to auto-configure with".
[[nodiscard]] Modio::Optional<ServiceConfig>
load_project_config(nx::string_view path = kDefaultConfigPath);

}
