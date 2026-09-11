#include "modio/modio_monetization.h"

#include "modio/modio_errors.h"

namespace nxm::modio {

void purchase_mod(
    const Service &service, const Modio::ModID id,
    const Modio::Optional<std::uint64_t> expected_virtual_currency_price,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::TransactionRecord>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::PurchaseModAsync(id, expected_virtual_currency_price,
                          std::move(on_done));
}

void purchase_mod_with_entitlement(
    const Service &service, const Modio::ModID id,
    Modio::EntitlementParams params,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::TransactionRecord>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::PurchaseModWithEntitlementAsync(id, std::move(params),
                                         std::move(on_done));
}

void refresh_user_entitlements(
    const Service &service, Modio::EntitlementParams params,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::EntitlementConsumptionStatusList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::RefreshUserEntitlementsAsync(std::move(params), std::move(on_done));
}

void get_available_user_entitlements(
    const Service &service, Modio::EntitlementParams params,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::EntitlementList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetAvailableUserEntitlementsAsync(std::move(params),
                                           std::move(on_done));
}

void get_user_wallet_balance(
    const Service &service,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::uint64_t>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetUserWalletBalanceAsync(std::move(on_done));
}

void fetch_user_purchases(const Service &service,
                          std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::FetchUserPurchasesAsync(std::move(on_done));
}

std::map<Modio::ModID, Modio::ModInfo>
query_user_purchased_mods(const Service &service) {
  return service.ready() ? Modio::QueryUserPurchasedMods()
                         : std::map<Modio::ModID, Modio::ModInfo>{};
}

}
