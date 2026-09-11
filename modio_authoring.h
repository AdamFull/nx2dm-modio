#pragma once

#include "modio/ModioSDK.h"
#include "modio/modio_service.h"

#include <functional>

namespace nxm::modio {

/// Thin wrappers around the mod.io SDK's mod authoring/publishing API:
/// creating and editing mods, uploading files/media, tags, dependencies,
/// ratings and reports. Each checks service.ready() (and, where the SDK
/// documents @requires management-enabled, mod_management_enabled() too) the
/// same way modio_user.h's wrappers do.

[[nodiscard]] Modio::ModCreationHandle new_mod_handle();

void submit_new_mod(
    const Service &service, Modio::ModCreationHandle handle,
    Modio::CreateModParams params,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModID>)>
        on_done);
void submit_mod_changes(
    const Service &service, Modio::ModID id, Modio::EditModParams params,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModInfo>)>
        on_done);

/// Fire-and-forget: results arrive as a Modio::ModManagementEvent through
/// Service's mod management callback, not a direct reply here. Requires mod
/// management to already be enabled (Service::enable_mod_management()).
[[nodiscard]] bool submit_new_mod_file(const Service &service, Modio::ModID id,
                                       Modio::CreateModFileParams params);
[[nodiscard]] bool submit_new_mod_source_file(
    const Service &service, Modio::ModID id,
    Modio::CreateSourceFileParams params);

void get_mod_logo(
    const Service &service, Modio::ModID id, Modio::LogoSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done);
void get_mod_gallery_image(
    const Service &service, Modio::ModID id, Modio::GallerySize size,
    Modio::GalleryIndex index,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done);
void get_mod_creator_avatar(
    const Service &service, Modio::ModID id, Modio::AvatarSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done);
void add_or_update_mod_logo(const Service &service, Modio::ModID id,
                            std::string logo_path,
                            std::function<void(Modio::ErrorCode)> on_done);
void add_or_update_mod_gallery_images(
    const Service &service, Modio::ModID id,
    std::vector<std::string> image_paths, bool sync_gallery,
    std::function<void(Modio::ErrorCode)> on_done);

void submit_mod_rating(const Service &service, Modio::ModID id,
                       Modio::Rating rating,
                       std::function<void(Modio::ErrorCode)> on_done);
void get_mod_tag_options(
    const Service &service,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModTagOptions>)>
        on_done);
void get_mod_dependencies(
    const Service &service, Modio::ModID id, bool recursive,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModDependencyList>)>
        on_done);
void add_mod_dependencies(const Service &service, Modio::ModID id,
                          std::vector<Modio::ModID> dependencies,
                          std::function<void(Modio::ErrorCode)> on_done);
void delete_mod_dependencies(const Service &service, Modio::ModID id,
                             std::vector<Modio::ModID> dependencies,
                             std::function<void(Modio::ErrorCode)> on_done);

void report_content(const Service &service, Modio::ReportParams report,
                    std::function<void(Modio::ErrorCode)> on_done);
void archive_mod(const Service &service, Modio::ModID id,
                 std::function<void(Modio::ErrorCode)> on_done);
void list_user_created_mods(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModInfoList>)>
        on_done);

/// Non-empty only right after a call above returned a validation failure.
[[nodiscard]] std::vector<Modio::FieldError> last_validation_error();

}
