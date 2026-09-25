#include "modio/modio_android.h"

#include "core/foundation/diagnostics/diagnostic.h"
#include "core/foundation/diagnostics/log.h"
#include "core/foundation/fibers/scheduler.h"

#include <SDL3/SDL_system.h>

#include "ModioAndroid.h"

namespace nxm::modio {
namespace {

const nx::log::Category log_modio = nx::log::category("modio");

/// Held for the life of the process on purpose. Modio::SetGlobalActivity does
/// not make a reference of its own - AndroidContextService just assigns the
/// jobject - so this global ref is the only thing keeping the activity alive
/// for the SDK, and the SDK's teardown is asynchronous and abandoned at
/// shutdown (see Service::on_detach). Releasing it would hand mod.io a
/// dangling reference with nothing gained; the process is exiting anyway.
jobject g_activity = nullptr;

} // namespace

bool initialize_android_backend() {
  // InitializeAndroid() allocates a new JavaClassWrapperModio every call and
  // leaks the last one, and initialize() may run again after a failure.
  if (g_activity != nullptr)
    return true;
  NX_ASSERT(!nx::fiber::in_fiber(),
            "Java fails on a fiber stack; call it through run_on_main");

  JNIEnv *const env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
  if (env == nullptr) {
    nx::loge(log_modio, "no JNI environment; mod.io cannot start");
    return false;
  }
  JavaVM *vm = nullptr;
  if (env->GetJavaVM(&vm) != JNI_OK || vm == nullptr) {
    nx::loge(log_modio, "no JavaVM; mod.io cannot start");
    return false;
  }
  jobject const activity = static_cast<jobject>(SDL_GetAndroidActivity());
  if (activity == nullptr) {
    nx::loge(log_modio, "no Android activity; mod.io cannot start");
    return false;
  }
  g_activity = env->NewGlobalRef(activity);
  env->DeleteLocalRef(activity);
  if (g_activity == nullptr) {
    nx::loge(log_modio, "failed to hold a global ref on the activity");
    return false;
  }

  // Order is the SDK's: the vm first, then the activity, then the bindings
  // that read both. bUseExternalStorageForMods stays at the SDK default so
  // downloaded mods land in external storage.
  Modio::InitializeAndroidJNI(vm, nullptr, true);
  Modio::SetGlobalActivity(g_activity);
  Modio::InitializeAndroid();
  return true;
}

} // namespace nxm::modio
