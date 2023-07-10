/**
 * @brief Provides ability to load and save configuration options.
 * @file config.h
 */
#pragma once
#include <nlohmann/json.hpp>
#include <imguiwrapper.hpp>
#include <optional>

/// Name of the directory in which the configuration is stored.
#define CONFIG_DIR "swic"
#define CONFIG_FILE "config.json"

using json_t = nlohmann::json;

/// Runtime configuration of the main application.
struct AppConfiguration {
  bool safe_mode = true;    ///< Revert changes after a while if not confirmed
  std::string swaymsg_path = "swaymsg";   ///< Path to swaymsg executable
  float revert_timeout = 10.0f;   ///< Number of seconds to revert changes after (in safe mode)
};

/// All configuration data.
struct Configuration {
  ImWrap::ContextDefinition imwrap;
  AppConfiguration app;
};

/**
 * @brief Save configuration data to disk.
 *
 * Save configuration to disk. It will be located in `XDG_CONFIG_HOME`
 * or `~` if not defined.
 *
 * @param def ImWrap context configuration
 * @return TRUE on success FALSE on failure.
 */
bool save_config(const Configuration& def);
/**
 * @brief Load configuration from disk.
 * @param out_def Where to save the loaded ImWrap configuration.
 * @return Deserialized configuration on success.
 */
std::optional<Configuration> load_config();

