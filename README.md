# mod.io module

Wraps the official [mod.io C++ SDK](https://github.com/modio/modio-sdk) as an
`nxe::Module`, exposing user-generated-content (mods: browsing, subscribing,
authoring, monetization, collections) to both C++ and Luau. Cross-platform per
the module descriptor (`ModulePlatform::All`), but only actually built and
run-tested on Windows so far - see "Platform status" below.

## Enabling it

- CMake: `-DNX_MODULE_MODIO=ON` (off by default).
- Per project: `python nx.py modules --add modio <project dir>`, which adds
  `"modio"` to `project.json`'s `"modules"` array. `projects/samples` already
  has it enabled as a live example.
- The SDK itself is vendored via `FetchContent` (see `NxModio.cmake`), pinned
  to a specific commit. `NX_MODIO_SOURCE_DIR` overrides it with a local
  checkout for SDK development, mirroring the same pattern `modules/spine`
  uses for `NX_SPINE2D_SOURCE_DIR`.

## Configuration

A project supplies its mod.io game ID and API key through an INI file at VFS
path `/config/modio.ini` (author it at `assets/config/modio.ini` - it's an
unrecognized-by-the-cooker extension, so it passes through the asset pipeline
byte-for-byte, no `.meta` needed, except on a Shipping build which would need
a real cook rule registered first):

```ini
[modio]
game_id = 3609
api_key = ca842a1f60c40bc8fb2044bc9932d763
test_environment = off
session_id = my-game
```

`game_id` and `api_key` are required; `test_environment` (on/off/true/false/
1/0) and `session_id` default to `off` and `"nx2d"`. `ModioModule::on_attach`
reads this file automatically (see `modio_config.h`/`.cpp`) and calls
`Service::initialize()` if it's present - no Luau call needed. A project with
no file, or one that wants to swap games/users at runtime, can skip it and
call the Luau `modio_configure` host function instead; the two paths don't
interfere; the module just won't auto-configure if `initialize()` was already
called or the file's absent.

This is the first module in the tree to own a project-scoped settings file -
there was no prior convention for it. It reuses `nx::vfs::read_text` +
`nx::ini::parse`, the same idiom `Engine::load_settings()` uses for
`/data/engine.ini`, since nothing in `EngineConfig` can depend on an optional
module and there's no other config-loading precedent to build on.

## Architecture

- **`Service`** (`modio_service.h/.cpp`) is the lifecycle owner: `Phase`
  (`Idle -> Initializing -> Ready -> ShuttingDown -> Idle`, or `Failed`),
  `initialize()`/`shutdown()`/`pump()`, plus the handful of operations common
  enough to deserve tracked busy/error state on `Service` itself (auth,
  subscribe/unsubscribe, mod management). The SDK's own state lives behind
  free functions in the global `Modio::` namespace, not an instance this class
  owns - `Service` only adds sequencing (don't call SDK functions outside the
  Ready window) and mod-management-event bookkeeping.
- **One free-function file per SDK area**, each taking `const Service&` first
  and gating on `service.ready()` (falling back to
  `Modio::make_error_code(GenericError::SDKNotInitialized)`, *not* a
  default-constructed `ErrorCode` - that's falsy/success and would silently
  lie to callers): `modio_user.h` (account/social/games),
  `modio_authoring.h` (create/edit/publish mods, media, dependencies),
  `modio_monetization.h` (purchases, wallet, entitlements),
  `modio_advanced.h` (progress/storage queries, temp mod sets, metrics
  sessions), `modio_collections.h` (curated mod bundles), `modio_server.h`
  (dedicated-server hosting - a separate init lifecycle, not gated on
  `Service::ready()` at all, matching the SDK's own docs).
- All 94 `MODIOSDK_API` functions in the vendored SDK are wrapped somewhere in
  the files above; this was verified mechanically (`grep` every SDK function
  name against every `Modio::*(` call site) rather than by inspection alone.
- `ModioModule::on_attach` also registers a `"modio.pump"` system in
  `Stage::Update` that calls `Service::pump()` on the main thread, since the
  SDK's callbacks only fire from inside `Modio::RunPendingHandlers()`. Each
  call costs a full millisecond - the SDK polls its event loop that long even
  with nothing queued - so the system pumps every frame only while the SDK is
  starting or stopping, mod management is busy, or an operation's callback
  is still pending (`Service::track()`). Otherwise it pumps every
  `IDLE_PUMP_INTERVAL` (100 ms). A C++ caller using `Modio::*` directly
  should wrap its callbacks with `Service::track()`, or accept that
  latency.

## Luau scripting surface

`modio_scripting.cpp` exposes ~59 host functions (`script-services.json` has
the exact signature list). Only scalar-safe operations are exposed directly;
anything that would need a rich struct result (search results, mod info, tag
lists, ...) stays C++-only by design - flattening those into scalars wasn't
worth the API surface, and a game's UI code is expected to bridge them from
C++ instead.

Most of the newer wrappers (authoring/monetization/advanced/collections/user)
share one pattern: a single `AsyncOp` struct (busy/error/numeric/text) that
every trigger function writes into, polled via `modio_op_busy()`/
`modio_op_error()`/`modio_op_result_id()`/`modio_op_result_text()`. This
mirrors `Service::last_operation_busy()`/`last_operation_error()`'s own
"whichever ran most recently" convenience, with the same caveat: it can race
if a script fires two of these concurrently.

`modio_server.h` is deliberately never exposed to Luau - dedicated-server
hosting is driven by a server process's own native bootstrap code, not game
scripts.

String *returns* (e.g. `modio_default_install_directory`, `modio_op_error`)
are supported by the host ABI (`value_traits<nx::string_view>` is symmetric,
and the engine's own core host API already ships several) - this was
initially assumed unconfirmed and later verified both by a signature-matching
test and a live Luau call (see Testing).

## Testing

All in `tests/`, run via `python nx.py test -k modio -- -DNX_MODULE_MODIO=ON`:

- `test_modio_service.cpp`, `test_modio_user.cpp`, `test_modio_advanced.cpp`,
  `test_modio_collections.cpp` - guard-path unit tests (every operation
  refuses correctly before `Service::ready()`). The SDK's global singleton
  state isn't mockable, so these can't isolate a "ready" `Service` without a
  real init.
- `test_modio_config.cpp` - `load_project_config()` against a VFS memory
  device: valid file, missing file, missing required field, non-numeric
  `game_id`, malformed ini, explicit-path override.
- `test_modio_scripting.cpp` - asks the actual compiled `Host::services()`
  (derived from the real C++ callables, not from `script-services.json`) for
  every signature and diffs it against the JSON. This is the one place a
  hand-written declaration could silently drift from what's really
  registered; nothing else checks that.
- `test_modio_smoke.cpp` - a genuine live network round trip against mod.io's
  real servers (`Environment::Live`, not `Test`: the SDK's own published demo
  credentials, GameID `3609` reused verbatim from `modio-sdk/examples/`, are
  registered against Live, not Test - confirmed via `curl` against both
  `api.mod.io` and `api.test.mod.io`. mod.io's Test tier is a private
  per-developer sandbox with no public demo credentials, so it can't be
  exercised without a real developer account). Bounded timeouts throughout;
  `SKIP`s via `Modio::ErrorCodeMatches(ec, ErrorConditionTypes::NetworkError)`
  rather than failing when the network is genuinely unreachable, so it won't
  false-fail in an offline CI environment while still catching a real
  regression when the network is up.
- Beyond the test binary: `projects/samples/assets/scripts/modio_startup.luau`
  drives a chunk of the surface through the real Luau VM in the actual
  shipped `nx2d.exe`, gated entirely by `/config/modio.ini`'s auto-configure -
  no C++ harness involved. Useful as a live worked example of calling this
  module from a game script.

## Platform status

Windows and Android have been configured, built and run. The CMake platform
detection (`_nx_modio_platform_string()` in `NxModio.cmake`) has branches for
iOS/Apple/Linux mirroring `cmake/NxModules.cmake`'s `_nx_current_platform`,
but those branches have not been exercised.

### Android

The SDK needs three things beyond the native library, none of which it can
arrange for itself:

- **A JNI bring-up before `Modio::InitializeAsync`** - `modio_android.cpp`
  passes the `JavaVM`, then the activity, then calls `InitializeAndroid()`, in
  that order. `Service::initialize()` runs it and fails the service if it does
  not succeed. Skipping it does not degrade gracefully: the SDK's file service
  calls a null JNI method ID and the process takes a SIGSEGV before the first
  frame.
- **`Modio.java` inside the APK** - the class its JNI wrapper looks up by
  name. Committed at `android/java/com/modio/modiosdk/`, which `:app`'s
  source-set loop picks up for any enabled module.
- **`modio.crt` at the APK asset root** - `Modio.java` copies it out of the
  asset manager by a path with no directory part, so the module's
  `android/build.gradle.kts` stages it separately from the engine's usual
  per-module `modules/<name>/` asset tree.

The last two are committed copies of SDK files, because Gradle assembles the
APK from committed directories while the SDK only exists inside the CMake
build tree. `NxModio.cmake` hashes both against the fetched SDK at configure
time and fails the build if `NX_MODIO_TAG` has moved out from under them - a
stale `Modio.java` would otherwise reappear as that same unexplained SIGSEGV.

`Modio::SetGlobalActivity` stores the `jobject` without taking a reference of
its own, so `modio_android.cpp` holds the global ref for the life of the
process. Note also that the SDK only reaches this code when a project actually
configures it: with no `/config/modio.ini` the module attaches and waits, and
none of the above runs.

## Known gaps

- No live coverage of the full authenticated UGC lifecycle (email auth,
  subscribe + real file download/install, purchases) - all need a real
  mod.io user account, which isn't automatable here.
- No cross-platform verification beyond Windows and Android (see above).
- `NX_MODIO_SOURCE_DIR` (local SDK checkout override) is unexercised - only
  the `FetchContent` git path has actually been run.
