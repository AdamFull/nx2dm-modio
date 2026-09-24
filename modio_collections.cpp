#include "modio/modio_collections.h"

#include "modio/modio_errors.h"

namespace nxm::modio {

void list_mod_collections(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfoList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::ListModCollectionsAsync(std::move(filter),
                                 service.track(std::move(on_done)));
}

void get_mod_collection_info(
    const Service &service, const Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfo>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModCollectionInfoAsync(id, service.track(std::move(on_done)));
}

void get_mod_collection_mods(
    const Service &service, const Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModInfoList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModCollectionModsAsync(id, service.track(std::move(on_done)));
}

void submit_mod_collection_rating(
    const Service &service, const Modio::ModCollectionID id,
    const Modio::Rating rating, std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::SubmitModCollectionRatingAsync(id, rating,
                                        service.track(std::move(on_done)));
}

void subscribe_to_mod_collection(const Service &service,
                                 const Modio::ModCollectionID id,
                                 std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::SubscribeToModCollectionAsync(id, service.track(std::move(on_done)));
}

void unsubscribe_from_mod_collection(
    const Service &service, const Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::UnsubscribeFromModCollectionAsync(id,
                                           service.track(std::move(on_done)));
}

void follow_mod_collection(
    const Service &service, const Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfo>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::FollowModCollectionAsync(id, service.track(std::move(on_done)));
}

void unfollow_mod_collection(const Service &service,
                             const Modio::ModCollectionID id,
                             std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::UnfollowModCollectionAsync(id, service.track(std::move(on_done)));
}

void list_user_followed_mod_collections(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfoList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::ListUserFollowedModCollectionsAsync(std::move(filter),
                                             service.track(std::move(on_done)));
}

void get_mod_collection_logo(
    const Service &service, const Modio::ModCollectionID id,
    const Modio::LogoSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModCollectionMediaAsync(id, size,
                                    service.track(std::move(on_done)));
}

void get_mod_collection_creator_avatar(
    const Service &service, const Modio::ModCollectionID id,
    const Modio::AvatarSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetModCollectionMediaAsync(id, size,
                                    service.track(std::move(on_done)));
}

}
