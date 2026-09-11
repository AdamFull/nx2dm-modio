#pragma once

#include "modio/ModioSDK.h"

namespace nxm::modio {

/// The ErrorCode every wrapper in this module returns when called before the
/// SDK reports ready() - mirrors what the SDK's own calls would return in
/// that state (GenericError::SDKNotInitialized), so callers can treat it
/// identically either way.
[[nodiscard]] inline Modio::ErrorCode not_ready_error() noexcept {
  return Modio::make_error_code(Modio::GenericError::SDKNotInitialized);
}

}
