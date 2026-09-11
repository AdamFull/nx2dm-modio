#pragma once

#include "modio/ModioSDK.h"
#include "modio/modio_service.h"

#include <functional>

namespace nxm::modio {

/// Thin wrappers around the mod.io SDK's user/account/social API. Each
/// function checks @p service.ready() the same way Service's own operations
/// do, and fails synchronously with a not-ready ErrorCode otherwise; there is
/// no separate busy/error tracking here (that convenience exists on Service
/// for its Luau-facing surface only; these are C++-only).

// -- Authentication --------------------------------------------------------

void authenticate_external(const Service &service,
                           Modio::AuthenticationParams params,
                           Modio::AuthenticationProvider provider,
                           std::function<void(Modio::ErrorCode)> on_done);
void authenticate_delegated_token(
    const Service &service, Modio::AuthenticationParams params,
    std::function<void(Modio::ErrorCode)> on_done);
void get_terms_of_use(
    const Service &service,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::Terms>)>
        on_done);
void clear_user_data(const Service &service,
                     std::function<void(Modio::ErrorCode)> on_done);
void refresh_user_data(const Service &service,
                       std::function<void(Modio::ErrorCode)> on_done);
void get_user_delegation_token(
    const Service &service,
    std::function<void(Modio::ErrorCode, std::string)> on_done);
void get_user_media(
    const Service &service, Modio::AvatarSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done);

/// Not gated on ready() - the SDK documents no such requirement, since this
/// only sets a preference consulted by later calls.
void set_language(Modio::Language locale);
[[nodiscard]] Modio::Language get_language();

// -- Social -----------------------------------------------------------------

void mute_user(const Service &service, Modio::UserID id,
               std::function<void(Modio::ErrorCode)> on_done);
void unmute_user(const Service &service, Modio::UserID id,
                 std::function<void(Modio::ErrorCode)> on_done);
void get_muted_users(
    const Service &service,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::UserList>)>
        on_done);
void follow_user(const Service &service, Modio::UserID id,
                 std::function<void(Modio::ErrorCode)> on_done);
void unfollow_user(const Service &service, Modio::UserID id,
                   std::function<void(Modio::ErrorCode)> on_done);
void get_user_followers(
    const Service &service, Modio::UserID id,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::UserList>)>
        on_done);
void get_user_following(
    const Service &service, Modio::UserID id,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::UserList>)>
        on_done);
void get_user_ratings(
    const Service &service,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::UserRatingList>)>
        on_done);

// -- Games --------------------------------------------------------------

void get_game_info(
    const Service &service, Modio::GameID id,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::GameInfo>)>
        on_done);
void list_user_games(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::GameInfoList>)>
        on_done);

}
