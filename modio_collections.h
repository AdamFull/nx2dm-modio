#pragma once

#include "modio/ModioSDK.h"
#include "modio/modio_service.h"

#include <functional>

namespace nxm::modio {

/// Thin wrappers around the mod.io SDK's mod collections API - curated
/// bundles of mods a game or creator groups together (distinct from an
/// individual mod's own dependency list). Each checks service.ready() the
/// same way modio_user.h's wrappers do.

void list_mod_collections(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfoList>)>
        on_done);
void get_mod_collection_info(
    const Service &service, Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfo>)>
        on_done);
void get_mod_collection_mods(
    const Service &service, Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModInfoList>)>
        on_done);
void submit_mod_collection_rating(
    const Service &service, Modio::ModCollectionID id, Modio::Rating rating,
    std::function<void(Modio::ErrorCode)> on_done);

/// Unlike subscribe(), this does not itself trigger installation - call
/// Service::fetch_external_updates() afterward to start it.
void subscribe_to_mod_collection(const Service &service,
                                 Modio::ModCollectionID id,
                                 std::function<void(Modio::ErrorCode)> on_done);
void unsubscribe_from_mod_collection(
    const Service &service, Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode)> on_done);
void follow_mod_collection(
    const Service &service, Modio::ModCollectionID id,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfo>)>
        on_done);
void unfollow_mod_collection(const Service &service, Modio::ModCollectionID id,
                             std::function<void(Modio::ErrorCode)> on_done);
void list_user_followed_mod_collections(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::ModCollectionInfoList>)>
        on_done);
void get_mod_collection_logo(
    const Service &service, Modio::ModCollectionID id, Modio::LogoSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done);
void get_mod_collection_creator_avatar(
    const Service &service, Modio::ModCollectionID id, Modio::AvatarSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done);

}
