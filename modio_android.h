#pragma once

namespace nxm::modio {

/// Hands the mod.io SDK the JNI vm and activity it needs before
/// Modio::InitializeAsync can run. Without it the SDK dereferences a null
/// method ID inside its own file service and the process dies before the first
/// frame. Safe to call more than once; only the first call does anything.
///
/// Android only - nothing else needs it, and the translation unit is only
/// compiled there.
bool initialize_android_backend();

} // namespace nxm::modio
