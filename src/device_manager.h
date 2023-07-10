/**
 * @brief Provides a DeviceManager class, which is used to manage libinput devices.
 * @file device_manager.h
 */
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

/// Datatype representing libinput calibration 2x3 matrix
using CalArr = std::array<float, 6>;

/**
 * @brief Works like std::optional, but can be enabled and disabled.
 *
 * This is made to be used to enable DeviceMan to have properties, which
 * have some value, but they can be explicitly enabled or disabled.
 * @tparam T Type to hold as data
 * @tparam enabled Initial enabled state
 */
template <typename T, bool enabled = true> class Opt {
public:
  bool m_Enabled = enabled;

  /**
   * @brief Create new instance of Opt with given value.
   * @param val Initial value of data.
   */
  Opt(const T& val) : m_val(val), m_hasVal(true) {}
  /**
   * @brief Create new instance of Opt with `{}` as value.
   */
  template <typename U = std::nullopt_t>
  Opt(std::initializer_list<U>) : m_val(), m_hasVal(false) {}
  Opt() = default;

  template <typename U = std::nullopt_t>
  Opt& operator=(std::initializer_list<U>) {
    m_hasVal = false;
    return *this;
  }

  Opt& operator=(const T& t) {
    m_hasVal = true;
    m_val = t;
    return *this;
  }
  operator bool() const { return m_hasVal; }
  T* operator->() { return &m_val; }

  inline T& value() { return m_val; }
  inline bool has_value() const { return m_hasVal; }
  T value_or(const T& val) { return m_hasVal ? m_val : val; }

private:
  T m_val;
  bool m_hasVal{false};
};

/// Get name of the enum from enum_strings.
template <typename T>
std::string
GetEnumName(const std::array<std::string, (int)T::size>& enum_strings,
            T enum_elem) {
  return enum_strings[(int)enum_elem];
}

/// Get enum type from name in enum_strings.
template <typename T>
Opt<T>
GetEnumFromName(const std::array<std::string, (int)T::size>& enum_strings,
                std::string name) {
  auto it = std::find(enum_strings.begin(), enum_strings.end(), name);
  if (it == enum_strings.end())
    return {};
  return T(it - enum_strings.begin());
}

/// Device capabilities
enum class DevType : int {
  keyboard = 0,   ///< Device is a keyboard
  pointer,        ///< Device is a pointer
  touchpad,       ///< Device is a touchpad
  tablet_tool,    ///< Device is a tablet tool
  tablet_pad,     ///< Device is a tabled pad
  gesture,        ///< Gesture device
  sw,             ///< Device is a switch
  unknown,        ///< Unknown device
  size
};
/// Strings of DevType enum
const std::array<std::string, (int)DevType::size> DEV_CAP_S = {
    "keyboard",   "pointer", "touchpad", "tablet_tool",
    "tablet_pad", "gesture", "switch",   "unknown"};
/// Get DevType string from enum.
inline std::string GetTypeName(DevType c) { return GetEnumName(DEV_CAP_S, c); }
/// Get DevType enum from string.
inline Opt<DevType> GetType(std::string name) { return GetEnumFromName<DevType>(DEV_CAP_S, name); }

/**
 * @brief Acts like a selectable enum of strings.
 *
 * This is used by DeviceMan to give user the list of available
 * options and let them select one.
 */
struct SEnum {
  /**
   * @brief Available string options
   * @todo Change this to const. To do this we need to ensure that the
   *       methods return types are valid and that the copy constructor
   *       is working as it should.
   */
  std::vector<std::string> options{};
  int sel{-1};  ///< Selected option

  /**
   * @brief Create new instance from given vector of strings (can be initializer list).
   * @param opts String options to contain as selectables.
   * @param sel Initial selection index to the opts string.
   */
  SEnum(const std::vector<std::string>& opts, int sel = 0) : options(opts), sel(sel) {}
  SEnum() = default;

  /**
   * @param Index to the SEnum::options vector.
   * @exception std::out_of_range On invalid index.
   */
  std::string& operator[](int index) {
    if (index >= (int)options.size())
      throw std::out_of_range("Index is out of range.");
    return options[index];
  }
  /// Same as operator[].
  inline std::string& get(int index) { return (*this)[index]; }

  /**
   * @brief When casted to string, then return the currently selected option
   * @exception std::out_of_range When no option is selected or invalid index.
   */
  operator std::string() const {
    if (sel < 0 || sel >= (int)options.size())
      throw std::out_of_range("No enum is selected or corrupted index (" +
                              std::to_string(sel) + ")");
    return options[sel];
  }

  /// Set option matching name as selected.
  bool select(std::string name) {
    for (size_t i = 0; i < options.size(); i++)
      if (options[i] == name) {
        sel = i;
        return true;
      }
    return false;
  }

  /// Return number of selectable options.
  inline int size() { return options.size(); }
  inline auto begin() { return options.begin(); }
  inline auto end() { return options.end(); }
};

/**
 * @brief Holds parameters devices can have.
 *
 * Taken from `man sway-input` and sway source code.
 * NOTE: Currently not all possible parameters are implemented.
 */
struct Device {
  std::string sway_id;    ///< ID of the device passed to the `swaymsg` call
  std::string name;       ///< Name of the device
  DevType type;           ///< Type of device
  Opt<float> scroll_factor; ///< Pointer, touch

  /* Keyboard */
  Opt<int> repeat_delay; ///< After how many milliseconds to start repeating.
  Opt<int> repeat_rate;  ///< How characters per second to repeat.
  /* Keyboard - can be set in config only */
  Opt<bool, false> xkb_capslock; ///< Initially enable capslock
  Opt<bool, false> xkb_numlock;  ///< Initially enable numlock

  /* Tablet */
  Opt<std::pair<SEnum, SEnum>, false> tool_mode;

  /* Mapping - cannot GET from swaymsg */
  Opt<SEnum, false> map_to_output; // Pointer, touch, tablet
                                   // Wildcard *, can be used to match the whole
                                   // desktop layout.
  Opt<std::array<int, 4>, false> map_to_region; // Valid for ^. Format: <x> <y> <w> <h>

  /* Libinput */
  bool send_events = true;
  Opt<bool> tap_to_click;
  Opt<bool> tap_and_drag;
  Opt<bool> tap_drag_lock;
  Opt<SEnum> tap_button_map;
  Opt<bool> left_handed;
  Opt<bool> nat_scroll;
  Opt<bool> mid_emu;   ///< Middle emulation
  Opt<CalArr> cal_mat; ///< Calibration
  Opt<SEnum> scroll_methods;
  Opt<int> scroll_button;
  Opt<bool> dwt;  ///< Disable while typing
  Opt<bool> dwtp; ///< Disable while trackpointing
  Opt<SEnum> click_methods;
  Opt<SEnum> accel_profiles;
  Opt<float> accel_speed;
};

/**
 * @brief Manages getting all devices and their parameters.
 */
class DeviceMan {
public:
  /// Do not manage devices with given capabilities.
  inline static const std::vector<DevType> SKIP_CAP = {
      DevType::unknown,
      DevType::sw, // Switch devices such as Lid-switch
      DevType::gesture};
  std::vector<Device> m_Devices;

  DeviceMan(std::string swaymsg_path);
  ~DeviceMan();

  /**
   * @brief Apply all changes to device settings.
   * @param device Index of the device in m_Devices arr.
   * @param backup Use stored initial backup of device configuration.
   */
  void ApplyChanges(int device, bool backup = false);

  /**
   * Revert changes to device to initial state (whel calling Init()).
   * @param device Index of the device in m_Devices arr.
   * @note This will not edit DeviceMan::m_Devices, meaning user edits are
   *       not deleted, but they are not applied.
   */
  inline void RevertChanges(int device) { ApplyChanges(device, true); }

  /**
   * @brief Revert changes made to the device to initial state
   * @param device Index of the device in m_Devices array
   */
  inline void RestoreBackup(int device) {
    ApplyChanges(device, true);
    m_Devices[device] = m_backupDevices[device];
  }

  /**
   * @brief Generate and return configuration which can be used in sway config
   * @param device Index of the device in m_Devices array
   */
  std::string GetSwayConfig(int device, bool match_type = false);

  Device& operator[](const size_t& i) { return m_Devices.at(i); }
  Device& Get(const size_t& i_device) { return (*this)[i_device]; }
  inline auto begin() { return m_Devices.begin(); }
  inline auto end() { return m_Devices.end(); }

private:
  // Holds initial configuration of devices.
  std::vector<Device> m_backupDevices;
  // Holds name of the swaymsg executable to call
  // when applying changes.
  std::string m_swaymsg;

  /// Parse information about libinput devices via swaymsg.
  void parseSwaymsg();
};

/// Define settings a device can have. Taken from `man sway-input`
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

// NOTE: Some settings have different names when we are getting
//       them and when we are setting them. To solve this, we
//       define `SWAY_SETTING_GET` for *get* names and we
//       define `SWAY_SETTING_SET` for *set* names.

/// Define name of the setting when parsing json from `swaymsg -t get_inputs --raw`
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
