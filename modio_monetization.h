#pragma once

#include "modio/ModioSDK.h"
#include "modio/modio_service.h"

#include <cstdint>
#include <functional>

namespace nxm::modio {

/// Thin wrappers around the mod.io SDK's monetization API (portal-driven
/// virtual currency/entitlement purchases). Each checks service.ready() the
/// same way modio_user.h's wrappers do.

void purchase_mod(
    const Service &service, Modio::ModID id,
    Modio::Optional<std::uint64_t> expected_virtual_currency_price,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::TransactionRecord>)>
        on_done);
void purchase_mod_with_entitlement(
    const Service &service, Modio::ModID id, Modio::EntitlementParams params,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::TransactionRecord>)>
        on_done);
void refresh_user_entitlements(
    const Service &service, Modio::EntitlementParams params,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::EntitlementConsumptionStatusList>)>
        on_done);
void get_available_user_entitlements(
    const Service &service, Modio::EntitlementParams params,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::EntitlementList>)>
        on_done);
void get_user_wallet_balance(
    const Service &service,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::uint64_t>)>
        on_done);
void fetch_user_purchases(const Service &service,
                          std::function<void(Modio::ErrorCode)> on_done);
/// Local cache populated by fetch_user_purchases(); empty before ready() or
/// before that has ever succeeded.
[[nodiscard]] std::map<Modio::ModID, Modio::ModInfo> query_user_purchased_mods(
    const Service &service);

}
