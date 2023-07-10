/**
 * @file config.cpp
 */
#include "config.h"
#include <fstream>
#include <sstream>
#include <cstdlib> // std::getenv
#include <filesystem>

namespace ImWrap {
  void to_json(json_t& json, const ImWrap::ContextDefinition& def) {
    json["window_width"] = def.window_width;
    json["window_height"] = def.window_height;
    json["window_title"] = def.window_title;
    json["window_hints"] = def.window_hints;
    json["swap_interval"] = def.swap_interval;
    json["exit_key"] = def.exit_key;
    json["imgui_theme"] = int(def.imgui_theme);
    json["imgui_config_flags"] = def.imgui_config_flags;
    json["imgui_multiviewport"] = def.imgui_multiviewport;
  }

  void from_json(const json_t& json, ImWrap::ContextDefinition& def) {
    json.at("window_width").get_to(def.window_width);
    json.at("window_height").get_to(def.window_height);
    json.at("window_title").get_to(def.window_title);
    json.at("window_hints").get_to(def.window_hints);
    json.at("swap_interval").get_to(def.swap_interval);
    json.at("exit_key").get_to(def.exit_key);
    json.at("imgui_theme").get_to(def.imgui_theme);
    json.at("imgui_config_flags").get_to(def.imgui_config_flags);
    json.at("imgui_multiviewport").get_to(def.imgui_multiviewport);
  }
}

void to_json(json_t& json, const AppConfiguration& config) {
  json["safe_mode"] = config.safe_mode;
  json["swaymsg_path"] = config.swaymsg_path;
  json["revert_timeout"] = config.revert_timeout;
}
void from_json(const json_t& json, AppConfiguration& config) {
  json["safe_mode"].get_to(config.safe_mode);
  json["swaymsg_path"].get_to(config.swaymsg_path);
  json["revert_timeout"].get_to(config.revert_timeout);
}

void to_json(json_t& json, const Configuration& config) {
  json["imwrap"] = config.imwrap;
  json["app"] = config.app;
}
void from_json(const json_t& json, Configuration& config) {
  json["imwrap"].get_to(config.imwrap);
  json["app"].get_to(config.app);
}

static auto g_env_config = std::getenv("XDG_CONFIG_HOME");

inline auto get_config_path() {
  return std::filesystem::path(g_env_config ? g_env_config : "~") / CONFIG_DIR / CONFIG_FILE;
}
inline auto get_config_dir() {
  return std::filesystem::path(g_env_config ? g_env_config : "~") / CONFIG_DIR;
}

bool save_config(const Configuration& config) {
  json_t json = config;

  // Create all direcotires until config if needed.
  auto config_dir = get_config_dir();
  if (!std::filesystem::exists(config_dir)) {
    std::filesystem::create_directories(config_dir);
  }

  std::ofstream stream(get_config_path());
  if (!stream.is_open()) {
    return false;
  }
  stream << json.dump(2);

  return true;
}

std::optional<Configuration> load_config() {
  std::ifstream stream(get_config_path());
  if (!stream.is_open())
    return {};

  json_t json;
  stream >> json;

  return json.get<Configuration>();
}

