# mod.io module

Wraps the official [mod.io C++ SDK](https://github.com/modio/modio-sdk) as an
`nxe::Module`, exposing user-generated content (browsing, subscribing,
authoring, monetization, collections) to C++ and Luau. The descriptor admits
every platform (`ModulePlatform::All`). It builds and runs on Windows and
Android; `NxModio.cmake` also selects the SDK's iOS, macOS and Linux
platforms, which have not been exercised.

## Enabling it

- CMake: `-DNX_MODULE_MODIO=ON` (off by default).
- Per project: `python nx.py modules <project dir> --add modio` adds
  `"modio"` to `project.json`'s `"modules"`. `projects/samples` enables it.
- `NxModio.cmake` fetches the SDK with `FetchContent` at the commit in
  `NX_MODIO_TAG` (modio-sdk has no tagged releases; `NX_MODIO_REPOSITORY`
  names the remote). `NX_MODIO_SOURCE_DIR` points at a local checkout, with
  submodules, instead.

## Configuration

A project gives its mod.io game id and API key in `/config/modio.ini`,
authored at `assets/config/modio.ini`. The cooker validates it and publishes
`modio.ini.nxb`, which `nx::ini::load` reads in place of the text; a Shipping
build reads nothing else. No `.meta` is needed.

```ini
[modio]
game_id = 3609
api_key = <the game's mod.io API key>
test_environment = off
session_id = my-game
```

`game_id` (an integer) and `api_key` are required. `test_environment`
(`on`/`off`/`true`/`false`/`1`/`0`) defaults to `off`, `session_id` to
`"nx2d"`. `ModioModule::on_attach` reads the file (`modio_config.h`) and calls
`Service::initialize()` when it is present. Without it, or to switch games or
users at run time, a script calls `modio_configure` instead; the module does
not auto-configure if `initialize()` already ran or the file is absent.

The file is the module's own because `EngineConfig` cannot depend on an
optional module.

## Architecture

- **`Service`** (`modio_service.h`) owns the lifecycle: `Phase`
  (`Idle -> Initializing -> Ready -> ShuttingDown -> Idle`, or `Failed`),
  `initialize()`, `shutdown()`, `pump()`, plus busy/error tracking for the
  common operations (auth, subscribe and unsubscribe, mod management). The
  SDK's state lives behind free functions in the global `Modio::` namespace;
  `Service` adds sequencing (no SDK calls outside the Ready window) and
  mod-management event bookkeeping.
- **One free-function file per SDK area**, each taking `const Service&` first
  and gating on `service.ready()`: `modio_user.h` (account, social, games),
  `modio_authoring.h` (create, edit and publish mods, media, dependencies),
  `modio_monetization.h` (purchases, wallet, entitlements),
  `modio_advanced.h` (progress and storage queries, temporary mod sets,
  metrics sessions), `modio_collections.h` (curated mod bundles). A refused
  call returns `GenericError::SDKNotInitialized`, never a default
  `ErrorCode`, which is falsy and would read as success.
- **`modio_server.h`** wraps dedicated-server hosting, which has its own init
  lifecycle and is not gated on `Service::ready()`, as the SDK documents.
- **The pump.** `on_attach` registers `modio.pump` in `Update`, which runs
  `Service::pump()` on the main thread through `run_on_main`; the SDK expects
  its pump on the thread it was set up on, and its callbacks fire only inside
  `Modio::RunPendingHandlers()`. Each pump costs a full millisecond, because
  the SDK's timer service re-posts itself every tick and its event loop spins
  to its time limit. So the system pumps every frame only while the SDK is
  starting or stopping, mod management is busy, or a callback wrapped by
  `Service::track()` is pending; otherwise every `IDLE_PUMP_INTERVAL`
  (500 ms), since the SDK's idle work waits on timers of a second or more. A
  C++ caller using `Modio::*` directly wraps its callbacks with
  `Service::track()` or accepts that latency.
- `on_detach` calls `shutdown()` as a best effort: SDK teardown needs further
  pumps, which stop with the module, so at engine exit it is abandoned.

## Luau scripting surface

`modio_scripting.cpp` exposes 62 host functions; `script-services.json` lists
their signatures. Only scalar-safe operations are exposed. Anything returning a
rich struct (search results, mod info, tag lists) stays C++: a game's UI
bridges those from C++ rather than through a flattened scalar API.
String returns, such as `modio_default_install_directory` and
`modio_op_error`, are supported by the host ABI. Every binding runs on the
main thread (`run_on_main`): scripts run on a job's fiber, the SDK calls Java
on Android, which fails on a fiber stack, and it expects the thread that pumps
it. `modio_configure(game_id, api_key, test_environment)` returns `false`
unless the service is `Idle`.

Asynchronous calls report through one shared slot. The core calls
(`modio_authenticate_email_code`, `modio_subscribe`, ...) use `modio_busy`;
the authoring, monetization, advanced, collections and user calls write a
single `AsyncOp` (busy, error, number, text) polled with `modio_op_busy`,
`modio_op_error`, `modio_op_result_id` and `modio_op_result_text`. Both hold
whichever operation ran last, so two concurrent calls race.

`modio_server.h` is never exposed to Luau: a server process drives hosting
from its own native bootstrap code, not game scripts.

`projects/samples/assets/scripts/modio_startup.luau` drives part of the
surface through the real player, gated by `/config/modio.ini`; it is the
worked example of calling the module from a script.

## Android

The SDK needs three things beyond its native library that it cannot arrange
itself:

- **A JNI bring-up before `Modio::InitializeAsync`.** `modio_android.cpp`
  passes the `JavaVM`, then the activity, then calls `InitializeAndroid()`,
  in that order, off a fiber. `Service::initialize()` runs it and fails the
  service if it does not succeed. Skipping it does not degrade: the SDK's
  file service calls a null JNI method ID and the process takes a SIGSEGV
  before the first frame.
- **`Modio.java` in the APK**, the class its JNI wrapper looks up by name. It
  is committed at `android/java/com/modio/modiosdk/`, which `:app`'s
  source-set loop compiles for any enabled module, and is `@Keep` so R8 does
  not strip it; the module's `android/build.gradle.kts` adds only the
  `androidx.annotation` dependency for that.
- **`modio.crt` at the APK asset root.** `Modio.java` opens it by plain name
  from the asset manager, so it lives in `android/assets/`, which the
  engine's generic `stageModuleRootAssets` task in `android/app/build.gradle.kts`
  stages unnamespaced at the APK root for every enabled module. It is
  packaged even when assets stream, because the asset manager sees only the
  APK.

Both files are committed copies of SDK files, because Gradle builds the APK
from committed directories while the SDK exists only in the CMake build tree.
On Android, `NxModio.cmake` compares their SHA-256 with the fetched SDK at
configure time and fails if `NX_MODIO_TAG` moved: a stale `Modio.java` would
otherwise surface as the same unexplained SIGSEGV.

`Modio::SetGlobalActivity` stores the `jobject` without a reference of its
own, so `modio_android.cpp` holds a global ref for the life of the process.
The bring-up runs once; `InitializeAndroid()` leaks a wrapper on every call.
With no `/config/modio.ini` and no `modio_configure` call, none of it runs.

## Testing

Run with `python nx.py test -k modio -- -DNX_MODULE_MODIO=ON`. The tests in
`tests/`:

- `test_modio_service.cpp`, `test_modio_user.cpp`, `test_modio_advanced.cpp`,
  `test_modio_collections.cpp`: every operation refuses before
  `Service::ready()`. The SDK's global state cannot be mocked, so a ready
  `Service` needs a real init.
- `test_modio_config.cpp`: `load_project_config()` on a VFS memory device
  (valid, missing file, missing field, non-numeric `game_id`, malformed INI,
  explicit path).
- `test_modio_scripting.cpp`: diffs the signatures `Host::services()` derives
  from the registered C++ callables against `script-services.json`, the only
  check that the hand-written list matches what is registered.
- `test_modio_smoke.cpp`: a live round trip against mod.io's servers in
  `Environment::Live` with the SDK's published demo credentials (game id
  `3609`), which are registered on Live, not Test. Timeouts are bounded, and
  it `SKIP`s on `ErrorConditionTypes::NetworkError`, so an offline machine
  does not fail it.

The authenticated lifecycle (email auth, a real download and install,
purchases) needs a real mod.io account and has no automated test.
