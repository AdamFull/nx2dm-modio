#include "modio/modio_user.h"

#include "modio/modio_errors.h"

namespace nxm::modio {

void authenticate_external(const Service &service,
                           Modio::AuthenticationParams params,
                           const Modio::AuthenticationProvider provider,
                           std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::AuthenticateUserExternalAsync(std::move(params), provider,
                                       std::move(on_done));
}

void authenticate_delegated_token(
    const Service &service, Modio::AuthenticationParams params,
    std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::AuthenticateUserDelegatedTokenAsync(std::move(params),
                                             std::move(on_done));
}

void get_terms_of_use(
    const Service &service,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::Terms>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetTermsOfUseAsync(std::move(on_done));
}

void clear_user_data(const Service &service,
                     std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::ClearUserDataAsync(std::move(on_done));
}

void refresh_user_data(const Service &service,
                       std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::RefreshUserDataAsync(std::move(on_done));
}

void get_user_delegation_token(
    const Service &service,
    std::function<void(Modio::ErrorCode, std::string)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), std::string());
  Modio::GetUserDelegationTokenAsync(std::move(on_done));
}

void get_user_media(
    const Service &service, const Modio::AvatarSize size,
    std::function<void(Modio::ErrorCode, Modio::Optional<std::string>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetUserMediaAsync(size, std::move(on_done));
}

void set_language(const Modio::Language locale) { Modio::SetLanguage(locale); }

Modio::Language get_language() { return Modio::GetLanguage(); }

void mute_user(const Service &service, const Modio::UserID id,
               std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::MuteUserAsync(id, std::move(on_done));
}

void unmute_user(const Service &service, const Modio::UserID id,
                 std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::UnmuteUserAsync(id, std::move(on_done));
}

void get_muted_users(
    const Service &service,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::UserList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetMutedUsersAsync(std::move(on_done));
}

void follow_user(const Service &service, const Modio::UserID id,
                 std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::FollowUserAsync(id, std::move(on_done));
}

void unfollow_user(const Service &service, const Modio::UserID id,
                   std::function<void(Modio::ErrorCode)> on_done) {
  if (!service.ready())
    return on_done(not_ready_error());
  Modio::UnfollowUserAsync(id, std::move(on_done));
}

void get_user_followers(
    const Service &service, const Modio::UserID id,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::UserList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetUserFollowersAsync(id, std::move(on_done));
}

void get_user_following(
    const Service &service, const Modio::UserID id,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::UserList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetUserFollowingAsync(id, std::move(on_done));
}

void get_user_ratings(
    const Service &service,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::UserRatingList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetUserRatingsAsync(std::move(on_done));
}

void get_game_info(
    const Service &service, const Modio::GameID id,
    std::function<void(Modio::ErrorCode, Modio::Optional<Modio::GameInfo>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::GetGameInfoAsync(id, std::move(on_done));
}

void list_user_games(
    const Service &service, Modio::FilterParams filter,
    std::function<void(Modio::ErrorCode,
                       Modio::Optional<Modio::GameInfoList>)>
        on_done) {
  if (!service.ready())
    return on_done(not_ready_error(), {});
  Modio::ListUserGamesAsync(std::move(filter), std::move(on_done));
}

}
