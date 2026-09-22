#include "modio/modio_scripting.h"

#include "modio/modio_advanced.h"
#include "modio/modio_authoring.h"
#include "modio/modio_collections.h"
#include "modio/modio_monetization.h"
#include "modio/modio_service.h"
#include "modio/modio_user.h"
// modio_server.h is deliberately not exposed here: it's a headless dedicated
// -server hosting API, meant to be driven by a server process's own native
// bootstrap code, not a game's Luau scripts.

#include "app/engine.h"
#include "script/script_host.h"

#include <memory>

namespace nxm::modio {
namespace {

[[nodiscard]] Service *service_of(nxe::ModuleContext &ctx) {
  return ctx.services().find<Service>();
}

/// Shared scratch state for the async wrappers below (modio_user.h,
/// modio_authoring.h, ...), which - unlike Service's own operations - don't
/// track busy/error themselves (they're plain C++ functions with no Luau
/// concerns baked in). This mirrors Service::last_operation_busy()/
/// last_operation_error(): one shared slot for "whichever of these ran most
/// recently", not a per-call result; a script that fires two of these
/// concurrently can race, same documented caveat as Service's own surface.
struct AsyncOp {
  bool busy = false;
  nx::string error;
  nx::string text;
  i64 numeric = 0;
};

void begin_op(AsyncOp &op) {
  op.busy = true;
  op.error.clear();
  op.text.clear();
  op.numeric = 0;
}

void end_op(AsyncOp &op, const Modio::ErrorCode ec) {
  op.busy = false;
  op.error = ec ? nx::string(ec.message()) : nx::string();
}

} // namespace

namespace {

/// One mod as a script reads it.
struct ModRecord {
  f64 id = 0.0;
  nx::string name;
};

/// One installed mod, with where it lives on disk.
struct InstalledModRecord {
  f64 id = 0.0;
  nx::string name;
  nx::string path;
};

[[nodiscard]] f64 id_of(const Modio::ModID id) {
  return static_cast<f64>(static_cast<Modio::ModID::UnderlyingType>(id));
}

}

void expose_modio_services(nxe::script::Host &host, nxe::ModuleContext &ctx) {
  host.expose_as("modio_configure", [&ctx](const f64 game_id,
                                           const nx::string_view api_key,
                                           const bool test_environment) {
    Service *const service = service_of(ctx);
    if (service == nullptr || service->phase() != Phase::Idle)
      return false;
    ServiceConfig config;
    config.game_id = nx::cast<i64>(game_id);
    config.api_key = nx::string(api_key);
    config.test_environment = test_environment;
    service->initialize(config);
    return true;
  });

  host.expose_as("modio_ready", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->ready();
  });

  host.expose_as("modio_authenticated", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->authenticated();
  });

  host.expose_as("modio_busy", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->last_operation_busy();
  });

  host.expose_as("modio_request_email_code", [&ctx](const nx::string_view email) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    service->request_email_code(email);
    return true;
  });

  host.expose_as("modio_authenticate_email_code",
                [&ctx](const nx::string_view code) {
                  Service *const service = service_of(ctx);
                  if (service == nullptr)
                    return false;
                  service->authenticate_email_code(code);
                  return true;
                });

  host.expose_as("modio_enable_mod_management", [&ctx]() {
    Service *const service = service_of(ctx);
    return service != nullptr && service->enable_mod_management();
  });

  host.expose_as("modio_subscribe", [&ctx](const f64 mod_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    service->subscribe(Modio::ModID(nx::cast<i64>(mod_id)), false);
    return true;
  });

  host.expose_as("modio_unsubscribe", [&ctx](const f64 mod_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    service->unsubscribe(Modio::ModID(nx::cast<i64>(mod_id)));
    return true;
  });

  host.expose_as("modio_is_subscribed", [&ctx](const f64 mod_id) {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->is_subscribed(Modio::ModID(
                                     nx::cast<i64>(mod_id)));
  });

  host.expose_as("modio_is_installed", [&ctx](const f64 mod_id) {
    const Service *const service = service_of(ctx);
    return service != nullptr && service->is_installed(Modio::ModID(
                                     nx::cast<i64>(mod_id)));
  });

  host.expose_as("modio_subscribed_mods", [&ctx] {
    nx::vector<ModRecord> out;
    if (const Service *const service = service_of(ctx))
      for (const auto &[id, entry] : service->subscriptions())
        out.push_back({id_of(id), nx::string(entry.GetModProfile().ProfileName)});
    return out;
  });

  host.expose_as("modio_subscribed_count", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service == nullptr
              ? 0.0
              : nx::cast<f64>(service->subscriptions().size());
  });

  host.expose_as("modio_installed_mods", [&ctx] {
    nx::vector<InstalledModRecord> out;
    if (const Service *const service = service_of(ctx))
      for (const auto &[id, entry] : service->installations(true))
        out.push_back({id_of(id), nx::string(entry.GetModProfile().ProfileName),
                       nx::string(entry.GetPath())});
    return out;
  });

  host.expose_as("modio_installed_count", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service == nullptr
              ? 0.0
              : nx::cast<f64>(service->installations(true).size());
  });

  // -- Shared result slot for everything below --------------------------
  // One shared instance, captured by every lambda that needs it; it lives
  // exactly as long as these registered bindings do.
  const auto op = std::make_shared<AsyncOp>();

  host.expose_as("modio_op_busy", [op]() { return op->busy; });
  host.expose_as("modio_op_error",
                 [op]() -> nx::string_view { return op->error.view(); });
  host.expose_as("modio_op_result_id",
                 [op]() { return static_cast<f64>(op->numeric); });
  host.expose_as("modio_op_result_text",
                 [op]() -> nx::string_view { return op->text.view(); });

  // -- User: account + social (modio_user.h) -----------------------------

  host.expose_as("modio_set_language", [](const f64 locale) {
    set_language(static_cast<Modio::Language>(static_cast<int>(locale)));
    return true;
  });

  host.expose_as("modio_get_language", []() {
    return static_cast<f64>(static_cast<int>(get_language()));
  });

  host.expose_as("modio_clear_user_data", [&ctx, op]() {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    clear_user_data(*service, [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_refresh_user_data", [&ctx, op]() {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    refresh_user_data(*service,
                      [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_get_user_media", [&ctx, op](const f64 size) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    get_user_media(*service, static_cast<Modio::AvatarSize>(static_cast<int>(size)),
                   [op](const Modio::ErrorCode ec,
                        const Modio::Optional<std::string> path) {
                     end_op(*op, ec);
                     if (path.has_value())
                       op->text = nx::string(*path);
                   });
    return true;
  });

  host.expose_as("modio_mute_user", [&ctx, op](const f64 user_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    mute_user(*service, Modio::UserID(nx::cast<i64>(user_id)),
             [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_unmute_user", [&ctx, op](const f64 user_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    unmute_user(*service, Modio::UserID(nx::cast<i64>(user_id)),
               [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_follow_user", [&ctx, op](const f64 user_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    follow_user(*service, Modio::UserID(nx::cast<i64>(user_id)),
               [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_unfollow_user", [&ctx, op](const f64 user_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    unfollow_user(*service, Modio::UserID(nx::cast<i64>(user_id)),
                  [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  // -- Authoring: creating/publishing mods (modio_authoring.h) -----------

  host.expose_as("modio_new_mod_handle", []() {
    return static_cast<f64>(
        static_cast<Modio::ModCreationHandle::UnderlyingType>(new_mod_handle()));
  });

  host.expose_as(
      "modio_submit_new_mod",
      [&ctx, op](const f64 handle, const nx::string_view name,
                const nx::string_view summary, const nx::string_view logo_path) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        Modio::CreateModParams params;
        params.Name = std::string(name);
        params.Summary = std::string(summary);
        params.PathToLogoFile = std::string(logo_path);
        begin_op(*op);
        submit_new_mod(
            *service, Modio::ModCreationHandle(nx::cast<i64>(handle)),
            std::move(params),
            [op](const Modio::ErrorCode ec,
                 const Modio::Optional<Modio::ModID> id) {
              end_op(*op, ec);
              if (id.has_value())
                op->numeric = static_cast<Modio::ModID::UnderlyingType>(*id);
            });
        return true;
      });

  host.expose_as(
      "modio_submit_mod_changes",
      [&ctx, op](const f64 mod_id, const nx::string_view name,
                const nx::string_view summary, const nx::string_view description,
                const nx::string_view homepage_url) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        Modio::EditModParams params;
        if (!name.empty())
          params.Name = std::string(name);
        if (!summary.empty())
          params.Summary = std::string(summary);
        if (!description.empty())
          params.Description = std::string(description);
        if (!homepage_url.empty())
          params.HomepageURL = std::string(homepage_url);
        begin_op(*op);
        submit_mod_changes(
            *service, Modio::ModID(nx::cast<i64>(mod_id)), std::move(params),
            [op](const Modio::ErrorCode ec, const Modio::Optional<Modio::ModInfo>) {
              end_op(*op, ec);
            });
        return true;
      });

  host.expose_as(
      "modio_submit_new_mod_file",
      [&ctx](const f64 mod_id, const nx::string_view root_directory,
            const nx::string_view version, const nx::string_view changelog) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        Modio::CreateModFileParams params;
        params.RootDirectory = std::string(root_directory);
        if (!version.empty())
          params.Version = std::string(version);
        if (!changelog.empty())
          params.Changelog = std::string(changelog);
        return submit_new_mod_file(*service, Modio::ModID(nx::cast<i64>(mod_id)),
                                   std::move(params));
      });

  host.expose_as(
      "modio_submit_new_mod_source_file",
      [&ctx](const f64 mod_id, const nx::string_view root_directory,
            const nx::string_view version, const nx::string_view changelog) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        Modio::CreateSourceFileParams params;
        params.RootDirectory = std::string(root_directory);
        if (!version.empty())
          params.Version = std::string(version);
        if (!changelog.empty())
          params.Changelog = std::string(changelog);
        return submit_new_mod_source_file(
            *service, Modio::ModID(nx::cast<i64>(mod_id)), std::move(params));
      });

  host.expose_as("modio_get_mod_logo", [&ctx, op](const f64 mod_id, const f64 size) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    get_mod_logo(*service, Modio::ModID(nx::cast<i64>(mod_id)),
                static_cast<Modio::LogoSize>(static_cast<int>(size)),
                [op](const Modio::ErrorCode ec,
                     const Modio::Optional<std::string> path) {
                  end_op(*op, ec);
                  if (path.has_value())
                    op->text = nx::string(*path);
                });
    return true;
  });

  host.expose_as(
      "modio_get_mod_gallery_image",
      [&ctx, op](const f64 mod_id, const f64 size, const f64 index) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        get_mod_gallery_image(
            *service, Modio::ModID(nx::cast<i64>(mod_id)),
            static_cast<Modio::GallerySize>(static_cast<int>(size)),
            static_cast<Modio::GalleryIndex>(nx::cast<i64>(index)),
            [op](const Modio::ErrorCode ec,
                 const Modio::Optional<std::string> path) {
              end_op(*op, ec);
              if (path.has_value())
                op->text = nx::string(*path);
            });
        return true;
      });

  host.expose_as(
      "modio_get_mod_creator_avatar", [&ctx, op](const f64 mod_id, const f64 size) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        get_mod_creator_avatar(
            *service, Modio::ModID(nx::cast<i64>(mod_id)),
            static_cast<Modio::AvatarSize>(static_cast<int>(size)),
            [op](const Modio::ErrorCode ec,
                 const Modio::Optional<std::string> path) {
              end_op(*op, ec);
              if (path.has_value())
                op->text = nx::string(*path);
            });
        return true;
      });

  host.expose_as(
      "modio_add_or_update_mod_logo",
      [&ctx, op](const f64 mod_id, const nx::string_view logo_path) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        add_or_update_mod_logo(
            *service, Modio::ModID(nx::cast<i64>(mod_id)), std::string(logo_path),
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as(
      "modio_submit_mod_rating", [&ctx, op](const f64 mod_id, const f64 rating) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        submit_mod_rating(*service, Modio::ModID(nx::cast<i64>(mod_id)),
                          static_cast<Modio::Rating>(static_cast<int>(rating)),
                          [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as(
      "modio_add_mod_dependency",
      [&ctx, op](const f64 mod_id, const f64 dependency_id) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        add_mod_dependencies(
            *service, Modio::ModID(nx::cast<i64>(mod_id)),
            {Modio::ModID(nx::cast<i64>(dependency_id))},
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as(
      "modio_delete_mod_dependency",
      [&ctx, op](const f64 mod_id, const f64 dependency_id) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        delete_mod_dependencies(
            *service, Modio::ModID(nx::cast<i64>(mod_id)),
            {Modio::ModID(nx::cast<i64>(dependency_id))},
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as("modio_archive_mod", [&ctx, op](const f64 mod_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    archive_mod(*service, Modio::ModID(nx::cast<i64>(mod_id)),
               [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_has_validation_error",
                 []() { return !last_validation_error().empty(); });

  // -- Monetization (modio_monetization.h) --------------------------------

  host.expose_as(
      "modio_purchase_mod", [&ctx, op](const f64 mod_id, const f64 expected_price) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        const Modio::Optional<std::uint64_t> price =
            expected_price > 0.0
                ? Modio::Optional<std::uint64_t>(
                      static_cast<std::uint64_t>(expected_price))
                : Modio::Optional<std::uint64_t>{};
        begin_op(*op);
        purchase_mod(
            *service, Modio::ModID(nx::cast<i64>(mod_id)), price,
            [op](const Modio::ErrorCode ec,
                 const Modio::Optional<Modio::TransactionRecord>) {
              end_op(*op, ec);
            });
        return true;
      });

  host.expose_as("modio_fetch_wallet_balance", [&ctx, op]() {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    get_user_wallet_balance(
        *service, [op](const Modio::ErrorCode ec,
                       const Modio::Optional<std::uint64_t> balance) {
          end_op(*op, ec);
          if (balance.has_value())
            op->numeric = static_cast<i64>(*balance);
        });
    return true;
  });

  host.expose_as("modio_fetch_user_purchases", [&ctx, op]() {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    fetch_user_purchases(*service,
                         [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_purchased_mods", [&ctx] {
    nx::vector<ModRecord> out;
    if (const Service *const service = service_of(ctx))
      for (const auto &[id, info] : query_user_purchased_mods(*service))
        out.push_back({id_of(id), nx::string(info.ProfileName)});
    return out;
  });

  host.expose_as("modio_purchased_mods_count", [&ctx]() {
    const Service *const service = service_of(ctx);
    return service == nullptr
              ? 0.0
              : nx::cast<f64>(query_user_purchased_mods(*service).size());
  });

  // -- Advanced: progress/storage queries (modio_advanced.h) --------------

  host.expose_as("modio_force_uninstall_mod", [&ctx, op](const f64 mod_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    force_uninstall_mod(*service, Modio::ModID(nx::cast<i64>(mod_id)),
                        [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
    return true;
  });

  host.expose_as("modio_prioritize_transfer_for_mod", [&ctx](const f64 mod_id) {
    Service *const service = service_of(ctx);
    return service != nullptr &&
          !prioritize_transfer_for_mod(*service, Modio::ModID(nx::cast<i64>(mod_id)));
  });

  host.expose_as("modio_current_update_mod_id", [&ctx]() {
    const Service *const service = service_of(ctx);
    if (service == nullptr)
      return 0.0;
    const Modio::Optional<Modio::ModProgressInfo> info =
        query_current_mod_update(*service);
    return info.has_value()
              ? static_cast<f64>(static_cast<Modio::ModID::UnderlyingType>(info->ID))
              : 0.0;
  });

  host.expose_as("modio_current_update_state", [&ctx]() {
    const Service *const service = service_of(ctx);
    if (service == nullptr)
      return -1.0;
    const Modio::Optional<Modio::ModProgressInfo> info =
        query_current_mod_update(*service);
    return info.has_value()
              ? static_cast<f64>(static_cast<int32_t>(info->GetCurrentState()))
              : -1.0;
  });

  host.expose_as("modio_current_update_progress", [&ctx]() {
    const Service *const service = service_of(ctx);
    if (service == nullptr)
      return 0.0;
    const Modio::Optional<Modio::ModProgressInfo> info =
        query_current_mod_update(*service);
    if (!info.has_value())
      return 0.0;
    const Modio::ModProgressInfo::EModProgressState state = info->GetCurrentState();
    const auto total =
        static_cast<f64>(static_cast<Modio::FileSize::UnderlyingType>(
            info->GetTotalProgress(state)));
    if (total <= 0.0)
      return 0.0;
    const auto current =
        static_cast<f64>(static_cast<Modio::FileSize::UnderlyingType>(
            info->GetCurrentProgress(state)));
    return current / total;
  });

  host.expose_as("modio_storage_consumed_bytes", [&ctx]() {
    const Service *const service = service_of(ctx);
    if (service == nullptr)
      return 0.0;
    const Modio::Optional<Modio::StorageInfo> info = query_storage_info(*service);
    if (!info.has_value())
      return 0.0;
    return static_cast<f64>(static_cast<Modio::FileSize::UnderlyingType>(
        info->GetSpace(Modio::StorageLocation::Local, Modio::StorageUsage::Consumed)));
  });

  host.expose_as("modio_default_install_directory",
                 [op](const f64 game_id) -> nx::string_view {
                   op->text = nx::string(default_mod_installation_directory(
                       Modio::GameID(nx::cast<i64>(game_id))));
                   return op->text.view();
                 });

  // -- Collections: curated mod bundles (modio_collections.h) -------------

  host.expose_as("modio_get_mod_collection_info", [&ctx, op](const f64 collection_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    get_mod_collection_info(
        *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
        [op](const Modio::ErrorCode ec,
             const Modio::Optional<Modio::ModCollectionInfo>) { end_op(*op, ec); });
    return true;
  });

  host.expose_as(
      "modio_subscribe_to_mod_collection", [&ctx, op](const f64 collection_id) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        subscribe_to_mod_collection(
            *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as(
      "modio_unsubscribe_from_mod_collection", [&ctx, op](const f64 collection_id) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        unsubscribe_from_mod_collection(
            *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as("modio_follow_mod_collection", [&ctx, op](const f64 collection_id) {
    Service *const service = service_of(ctx);
    if (service == nullptr)
      return false;
    begin_op(*op);
    follow_mod_collection(
        *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
        [op](const Modio::ErrorCode ec,
             const Modio::Optional<Modio::ModCollectionInfo>) { end_op(*op, ec); });
    return true;
  });

  host.expose_as(
      "modio_unfollow_mod_collection", [&ctx, op](const f64 collection_id) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        unfollow_mod_collection(
            *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as(
      "modio_submit_mod_collection_rating",
      [&ctx, op](const f64 collection_id, const f64 rating) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        submit_mod_collection_rating(
            *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
            static_cast<Modio::Rating>(static_cast<int>(rating)),
            [op](const Modio::ErrorCode ec) { end_op(*op, ec); });
        return true;
      });

  host.expose_as(
      "modio_get_mod_collection_logo",
      [&ctx, op](const f64 collection_id, const f64 size) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        get_mod_collection_logo(
            *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
            static_cast<Modio::LogoSize>(static_cast<int>(size)),
            [op](const Modio::ErrorCode ec,
                 const Modio::Optional<std::string> path) {
              end_op(*op, ec);
              if (path.has_value())
                op->text = nx::string(*path);
            });
        return true;
      });

  host.expose_as(
      "modio_get_mod_collection_creator_avatar",
      [&ctx, op](const f64 collection_id, const f64 size) {
        Service *const service = service_of(ctx);
        if (service == nullptr)
          return false;
        begin_op(*op);
        get_mod_collection_creator_avatar(
            *service, Modio::ModCollectionID(nx::cast<i64>(collection_id)),
            static_cast<Modio::AvatarSize>(static_cast<int>(size)),
            [op](const Modio::ErrorCode ec,
                 const Modio::Optional<std::string> path) {
              end_op(*op, ec);
              if (path.has_value())
                op->text = nx::string(*path);
            });
        return true;
      });
}

}
