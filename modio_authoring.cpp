#include "modio/modio_authoring.h"

#include "modio/modio_errors.h"

namespace nxm::modio {

Modio::ModCreationHandle new_mod_handle() { return Modio::GetModCreationHandle(); }

void submit_new_mod(
    const Service &service, const Modio::ModCreationHandle handle,
    Modio::CreateModParams params,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModID>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::SubmitNewModAsync(handle, std::move(params), std::move(on_done));
}

void submit_mod_changes(
    const Service &service, const Modio::ModID id, Modio::EditModParams params,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::ModInfo>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::SubmitModChangesAsync(id, std::move(params), std::move(on_done));
}

bool submit_new_mod_file(const Service &service, const Modio::ModID id,
                         Modio::CreateModFileParams params) {
  if (!service.ready() || !service.mod_management_enabled())
    return false;
  Modio::SubmitNewModFileForMod(id, std::move(params));
  return true;
}

bool submit_new_mod_source_file(const Service &service, const Modio::ModID id,
                                Modio::CreateSourceFileParams params) {
  if (!service.ready() || !service.mod_management_enabled())
    return false;
  Modio::SubmitNewModSourceFile(id, std::move(params));
  return true;
}

void get_mod_logo(
    const Service &service, const Modio::ModID id, const Modio::LogoSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModMediaAsync(id, size, std::move(on_done));
}

void get_mod_gallery_image(
    const Service &service, const Modio::ModID id,
    const Modio::GallerySize size, const Modio::GalleryIndex index,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModMediaAsync(id, size, index, std::move(on_done));
}

void get_mod_creator_avatar(
    const Service &service, const Modio::ModID id,
    const Modio::AvatarSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModMediaAsync(id, size, std::move(on_done));
}

void add_or_update_mod_logo(const Service &service, const Modio::ModID id,
                            std::string logo_path,
                            std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::AddOrUpdateModLogoAsync(id, std::move(logo_path), std::move(on_done));
}

void add_or_update_mod_gallery_images(
    const Service &service, const Modio::ModID id,
    std::vector<std::string> image_paths, const bool sync_gallery,
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::AddOrUpdateModGalleryImagesAsync(id, std::move(image_paths),
                                          sync_gallery, std::move(on_done));
}

void submit_mod_rating(const Service &service, const Modio::ModID id,
                       const Modio::Rating rating,
                       std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::SubmitModRatingAsync(id, rating, std::move(on_done));
}

void get_mod_tag_options(
    const Service &service,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModTagOptions>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModTagOptionsAsync(std::move(on_done));
}

void get_mod_dependencies(
    const Service &service, const Modio::ModID id, const bool recursive,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModDependencyList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModDependenciesAsync(id, recursive, std::move(on_done));
}

void add_mod_dependencies(const Service &service, const Modio::ModID id,
                          std::vector<Modio::ModID> dependencies,
                          std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::AddModDependenciesAsync(id, std::move(dependencies),
                                 std::move(on_done));
}

void delete_mod_dependencies(const Service &service, const Modio::ModID id,
                             std::vector<Modio::ModID> dependencies,
                             std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::DeleteModDependenciesAsync(id, std::move(dependencies),
                                    std::move(on_done));
}

void report_content(const Service &service, Modio::ReportParams report,
                    std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::ReportContentAsync(std::move(report), std::move(on_done));
}

void archive_mod(const Service &service, const Modio::ModID id,
                 std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::ArchiveModAsync(id, std::move(on_done));
}

void list_user_created_mods(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModInfoList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::ListUserCreatedModsAsync(std::move(filter), std::move(on_done));
}

std::vector<Modio::FieldError> last_validation_error() {
  return Modio::GetLastValidationError();
}

}
