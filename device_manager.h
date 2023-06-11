#pragma once
#include <algorithm>
#include <array>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <utility> // std::pair

using CalArr = std::array<float, 6>;

/**
 * Works like std::optional, but has some additional properties
 */
template <typename T, bool enabled = true> class Opt {
public:
  bool m_Enabled = enabled;

  Opt(const T &val) : m_val(val), m_hasVal(true) {}

  template <typename U = std::nullopt_t>
  Opt(std::initializer_list<U>) : m_val(), m_hasVal(false) {}
  Opt() = default;

  template <typename U = std::nullopt_t>
  Opt &operator=(std::initializer_list<U>) {
    m_hasVal = false;
    return *this;
  }

  Opt &operator=(const T &t) {
    m_hasVal = true;
    m_val = t;
    return *this;
  }
  operator bool() const { return m_hasVal; }
  T *operator->() { return &m_val; }

  inline T &value() { return m_val; }
  inline bool has_value() const { return m_hasVal; }
  T value_or(const T &val) { return m_hasVal ? m_val : val; }

private:
  T m_val;
  bool m_hasVal{false};
};

// Return name of the enum from enum_strings.
template <typename T>
std::string
GetEnumName(const std::array<std::string, (int)T::size> &enum_strings,
            T enum_elem) {
  return enum_strings[(int)enum_elem];
}

// Get enum type from name in enum_strings.
template <typename T>
Opt<T>
GetEnumFromName(const std::array<std::string, (int)T::size> &enum_strings,
                std::string name) {
  auto it = std::find(enum_strings.begin(), enum_strings.end(), name);
  if (it == enum_strings.end())
    return {};
  return T(it - enum_strings.begin());
}

// Device capabilities
enum class DevType : int {
  keyboard = 0,
  pointer,
  touchpad,
  tablet_tool,
  tablet_pad,
  gesture,
  sw,
  unknown,
  size
};
const std::array<std::string, (int)DevType::size> DEV_CAP_S = {
    "keyboard",   "pointer", "touchpad", "tablet_tool",
    "tablet_pad", "gesture", "switch",   "unknown"};
inline std::string GetTypeName(DevType c) { return GetEnumName(DEV_CAP_S, c); }
inline Opt<DevType> GetType(std::string name) {
  return GetEnumFromName<DevType>(DEV_CAP_S, name);
}

/**
 * Type holding multiple string options and information about which one is
 * selected.
 */
struct SEnum {
  std::vector<std::string> options{};
  int sel{-1}; // Selected option

  SEnum(const std::vector<std::string> &opts) : options(opts), sel(0) {}
  SEnum() = default;

  // Automatically index the options array.
  std::string &operator[](int index) {
    if (index >= (int)options.size())
      throw std::out_of_range("Index is out of range.");
    return options[index];
  }
  inline std::string &get(int index) { return (*this)[index]; }

  // Return currencly selected option when casting to string.
  operator std::string() const {
    if (sel < 0 || sel >= (int)options.size())
      throw std::out_of_range("No enum is selected or corrupted index (" +
                              std::to_string(sel) + ")");
    return options[sel];
  }

  // Set option matching name as selected.
  bool select(std::string name) {
    for (size_t i = 0; i < options.size(); i++)
      if (options[i] == name) {
        sel = i;
        return true;
      }
    return false;
  }

  inline int size() { return options.size(); }
  inline auto begin() { return options.begin(); }
  inline auto end() { return options.end(); }
};

/**
 * Holds parameters devices can have.
 * Taken from `man sway-input` and sway source code.
 * NOTE: Currently not all possible parameters are implemented.
 */
struct Device {
  std::string sway_id;
  std::string name;
  DevType type;
  Opt<float> scroll_factor; // Pointer, touch

  /* Keyboard */
  Opt<int> repeat_delay; // Milliseconds
  Opt<int> repeat_rate;  // Characters per second
  /* Keyboard - can be set in config only */
  Opt<bool, false> xkb_capslock; // Initially enable capslock
  Opt<bool, false> xkb_numlock;  // Initially enable numlock

  /* Tablet */
  Opt<std::pair<SEnum, SEnum>, false> tool_mode;

  /* Mapping - cannot GET from swaymsg */
  Opt<SEnum, false> map_to_output; // Pointer, touch, tablet
                                   // Wildcard *, can be used to match the whole
                                   // desktop layout.
  Opt<std::array<int, 4>, false>
      map_to_region; // Valid for ^. Format: <x> <y> <w> <h>

  /* Libinput */
  bool send_events = true;
  Opt<bool> tap_to_click;
  Opt<bool> tap_and_drag;
  Opt<bool> tap_drag_lock;
  Opt<SEnum> tap_button_map;
  Opt<bool> left_handed;
  Opt<bool> nat_scroll;
  Opt<bool> mid_emu;   // Middle emulation
  Opt<CalArr> cal_mat; // Calibration
  Opt<SEnum> scroll_methods;
  Opt<int> scroll_button;
  Opt<bool> dwt;  // Disable while typing
  Opt<bool> dwtp; // Disable while trackpointing
  Opt<SEnum> click_methods;
  Opt<SEnum> accel_profiles;
  Opt<float> accel_speed;
};

/**
 * Manages getting all devices and their parameters.
 */
class DeviceMan {
public:
  // Skip device with given capabilities.
  inline static const std::vector<DevType> SKIP_CAP = {
      DevType::unknown,
      DevType::sw, // Switch devices such as Lid-switch
      DevType::gesture};
  std::vector<Device> m_Devices;

  DeviceMan(std::string swaymsg_path);
  ~DeviceMan();

  /// Apply all changes to device settings.
  /// @param device Index of the device in m_Devices arr.
  void ApplyChanges(int device, bool backup = false);

  /// Revert changes to device to initial state (whel calling Init()).
  /// NOTE: This will not edit m_Devices
  /// @param device Index of the device in m_Devices arr.
  inline void RevertChanges(int device) { ApplyChanges(device, true); }

  /// Revert changes made to the device to initial state
  /// @param device Index of the device in m_Devices array
  inline void RestoreBackup(int device) {
    ApplyChanges(device, true);
    m_Devices[device] = m_backupDevices[device];
  }

  /// Generate and return configuration which can be used in sway config
  /// @param device Index of the device in m_Devices array
  std::string GetSwayConfig(int device, bool match_type = false);

  Device &operator[](const size_t &i) { return m_Devices.at(i); }
  Device &Get(const size_t &i_device) { return (*this)[i_device]; }
  inline auto begin() { return m_Devices.begin(); }
  inline auto end() { return m_Devices.end(); }

private:
  // Store initial configuration of devices.
  std::vector<Device> m_backupDevices;
  std::string m_swaymsg;

  void parseSwaymsg();
};

/// Define settings a device can have.
enum class SwaySetting : int {
  repeat_delay = 0,
  repeat_rate,
  scroll_factor,
  tool_mode,
  map_to_output,
  map_to_region,
  send_events,
  tap_to_click,
  tap_and_drag,
  tap_drag_lock,
  tap_button_map,
  left_handed,
  natural_scroll,
  middle_emulation,
  cal_mat,
  scroll_method,
  scroll_button,
  dwt,
  dwtp,
  click_method,
  accel_profile,
  accel_speed,
  size
};

/// Define name of the setting when parsing json from `swaymsg -t get_inputs
/// --raw`
const static std::array<std::string, (int)SwaySetting::size> SWAY_SETTING_GET =
    {"repeat_delay",
     "repeat_rate",
     "scroll_factor",
     "tool_mode",
     "map_to_output",
     "map_to_region",
     "send_events",
     "tap",
     "tap_drag",
     "tap_drag_lock",
     "tap_button_map",
     "left_handed",
     "natural_scroll",
     "middle_emulation",
     "calibration_matrix",
     "scroll_method",
     "scroll_button",
     "dwt",
     "dwtp",
     "click_method",
     "accel_profile",
     "accel_speed"};

/// Define names of the settings when using `swaymsg input <set setting> ...` or
/// generating sway config. NOTE: Does not contain config only options.
const static std::array<std::string, (int)SwaySetting::size> SWAY_SETTING_SET{
    SWAY_SETTING_GET[0],
    SWAY_SETTING_GET[1],
    SWAY_SETTING_GET[2],
    SWAY_SETTING_GET[3],
    SWAY_SETTING_GET[4],
    SWAY_SETTING_GET[5],
    "events",
    SWAY_SETTING_GET[7],
    "drag",
    "drag_lock",
    SWAY_SETTING_GET[10],
    SWAY_SETTING_GET[11],
    SWAY_SETTING_GET[12],
    SWAY_SETTING_GET[13],
    SWAY_SETTING_GET[14],
    SWAY_SETTING_GET[15],
    SWAY_SETTING_GET[16],
    SWAY_SETTING_GET[17],
    SWAY_SETTING_GET[18],
    SWAY_SETTING_GET[19],
    SWAY_SETTING_GET[20],
    "pointer_accel"};

inline std::string GetSettingName(SwaySetting s, bool get = true) {
  return GetEnumName(get ? SWAY_SETTING_GET : SWAY_SETTING_SET, s);
}
inline Opt<SwaySetting> GetSetting(std::string name, bool get = true) {
  return GetEnumFromName<SwaySetting>(get ? SWAY_SETTING_GET : SWAY_SETTING_SET,
                                      name);
}
