#include "modio/modio_config.h"

#include "core/foundation/diagnostics/log.h"
#include "core/foundation/serialization/ini.h"
#include "core/foundation/vfs/vfs.h"

namespace nxm::modio {
namespace {

const nx::log::Category log_modio = nx::log::category("modio");

[[nodiscard]] bool parse_i64(const nx::string_view text, i64 &out) noexcept {
  if (text.empty())
    return false;
  usize i = 0;
  const bool negative = text[0] == '-';
  if (negative)
    i = 1;
  if (i == text.size())
    return false;
  i64 value = 0;
  for (; i < text.size(); ++i) {
    const char c = text[i];
    if (c < '0' || c > '9')
      return false;
    value = value * 10 + (c - '0');
  }
  out = negative ? -value : value;
  return true;
}

[[nodiscard]] bool parse_toggle(const nx::string_view text, bool &out) noexcept {
  if (text == "on" || text == "true" || text == "1") {
    out = true;
    return true;
  }
  if (text == "off" || text == "false" || text == "0") {
    out = false;
    return true;
  }
  return false;
}

} // namespace

Modio::Optional<ServiceConfig> load_project_config(const nx::string_view path) {
  const auto text = nx::vfs::read_text(path);
  if (!text) {
    if (text.error().kind != nx::fs::io_error::NotFound)
      nx::logw(log_modio, "config: could not read '{}': {}", path,
               nx::fs::to_string(text.error().kind));
    return {};
  }

  const auto parsed = nx::ini::parse(text->view());
  if (!parsed) {
    const nx::ini::ParseError &error = parsed.error();
    nx::logw(log_modio, "config: {}:{}:{}: {}", path, error.line, error.column,
             error.message());
    return {};
  }

  const nx::string *const game_id_text = parsed->find("modio", "game_id");
  const nx::string *const api_key = parsed->find("modio", "api_key");
  if (game_id_text == nullptr || api_key == nullptr) {
    nx::logw(log_modio, "config: '{}' is missing [modio] game_id or api_key",
             path);
    return {};
  }

  ServiceConfig config;
  if (!parse_i64(game_id_text->view(), config.game_id)) {
    nx::logw(log_modio, "config: '{}' [modio] game_id '{}' is not an integer",
             path, *game_id_text);
    return {};
  }
  config.api_key = *api_key;

  if (const nx::string *const value = parsed->find("modio", "test_environment")) {
    if (!parse_toggle(value->view(), config.test_environment)) {
      nx::logw(log_modio,
               "config: '{}' [modio] test_environment '{}' is not on/off",
               path, *value);
      return {};
    }
  }
  if (const nx::string *const value = parsed->find("modio", "session_id"))
    config.session_id = *value;

  return config;
}

}
