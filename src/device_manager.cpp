/**
 * @brief Implementation of DeviceMan
 * @file device_manager.cpp
 */
#include "device_manager.h"
#include <cstdlib> // system()
#include <exception>
#include <sstream>
#include <stdio.h>

#include <nlohmann/json.hpp>
#include <sstream>
using json = nlohmann::json;

DeviceMan::DeviceMan(std::string swaymsg_path) : m_swaymsg(swaymsg_path) {
  parseSwaymsg();
  m_backupDevices = m_Devices;
}

DeviceMan::~DeviceMan() {}

// Read swaymsg outout into nlohmann::json structure.
void read_swaymsg(json& out, FILE* swaymsg_fp) {
  std::stringstream sstr;
  char line[256];
  while (fgets(line, sizeof(line), swaymsg_fp) != NULL) {
    std::string line_s(line);
    sstr << line_s;
  }
  sstr >> out;
}

// Read swaymsg -t get_inputs --raw output into json.
void get_inputs_json(json& out, std::string swaymsg_path) {
  FILE* fp = popen((swaymsg_path + " -t get_inputs --raw").c_str(), "r");
  if (!fp)
    throw std::runtime_error("Failed to call swaymsg.");
  read_swaymsg(out, fp);
  pclose(fp);
}

// Read swaymsg -t get_outputs --raw output into json.
void get_outputs_json(json& out, std::string swaymsg_path) {
  FILE* fp = popen((swaymsg_path + " -t get_outputs --raw").c_str(), "r");
  if (!fp)
    throw std::runtime_error("Failed to call swaymsg.");
  read_swaymsg(out, fp);
  pclose(fp);
}

// String to boolean
inline bool stb(const std::string& s) { return s == "enabled" ? true : false; }
// Boolean to swaymsg string
inline std::string bts(bool b) { return b ? "enabled" : "disabled"; }

// Convert json block to Device.
void from_json(const json& j, Opt<Device>& device) {
  // Parse type and if it should be skipped then skip it.
  std::string type;
  j.at("type").get_to(type);
  if (std::find(DeviceMan::SKIP_CAP.begin(), DeviceMan::SKIP_CAP.end(),
                GetType(type).value_or(DevType::unknown)) !=
      DeviceMan::SKIP_CAP.end()) {
    device = {};
    return;
  }

  /* Parse json values according to to sway source code (ipc-json.c) */
  Device d;
  d.type = GetType(type).value_or(DevType::unknown);
  j.at("name").get_to(d.name);
  j.at("identifier").get_to(d.sway_id);

  if (d.type == DevType::touchpad || d.type == DevType::pointer) {
    if (j.contains(GetSettingName(SwaySetting::scroll_factor)))
      d.scroll_factor =
          j[GetSettingName(SwaySetting::scroll_factor)].get<float>();
  } else if (d.type == DevType::keyboard) {
    if (j.contains(GetSettingName(SwaySetting::repeat_delay)))
      d.repeat_delay = j[GetSettingName(SwaySetting::repeat_delay)].get<int>();
    if (j.contains(GetSettingName(SwaySetting::repeat_rate)))
      d.repeat_rate = j[GetSettingName(SwaySetting::repeat_rate)].get<int>();
  } else if (d.type == DevType::tablet_tool || d.type == DevType::tablet_pad) {
    SEnum tool({"pen", "eraser", "brush", "pencil", "airbrush", "*"});
    SEnum mode({"absolute", "relative"});
    /// FIXME: Somehow recieve this from swaymsg.
    tool.select("*");
    mode.select("asolute");
    d.tool_mode = std::make_pair(tool, mode);
  }

  const json& libinput = j.at("libinput");
  d.send_events = stb(libinput[GetSettingName(SwaySetting::send_events)]);

  if (libinput.contains(GetSettingName(SwaySetting::tap_to_click)))
    d.tap_to_click = stb(libinput[GetSettingName(SwaySetting::tap_to_click)]);
  if (libinput.contains(GetSettingName(SwaySetting::tap_button_map))) {
    d.tap_button_map = SEnum({"lrm", "lmr"});
    d.tap_button_map->select(
        libinput[GetSettingName(SwaySetting::tap_button_map)]);
  }
  if (libinput.contains(GetSettingName(SwaySetting::tap_and_drag)))
    d.tap_and_drag = stb(libinput[GetSettingName(SwaySetting::tap_and_drag)]);
  if (libinput.contains(GetSettingName(SwaySetting::tap_drag_lock)))
    d.tap_drag_lock = stb(libinput[GetSettingName(SwaySetting::tap_drag_lock)]);

  if (libinput.contains(GetSettingName(SwaySetting::accel_speed)))
    d.accel_speed =
        libinput[GetSettingName(SwaySetting::accel_speed)].get<float>();
  if (libinput.contains(GetSettingName(SwaySetting::accel_profile))) {
    d.accel_profiles = SEnum({"adaptive", "flat"});
    d.accel_profiles->select(
        libinput[GetSettingName(SwaySetting::accel_profile)]);
  }

  if (libinput.contains(GetSettingName(SwaySetting::natural_scroll)))
    d.nat_scroll = stb(libinput[GetSettingName(SwaySetting::natural_scroll)]);

  if (libinput.contains(GetSettingName(SwaySetting::left_handed)))
    d.left_handed = stb(libinput[GetSettingName(SwaySetting::left_handed)]);

  if (libinput.contains(GetSettingName(SwaySetting::click_method))) {
    d.click_methods = SEnum({"none", "button_areas", "clickfinger"});
    d.click_methods->select(
        libinput[GetSettingName(SwaySetting::click_method)]);
  }

  if (libinput.contains(GetSettingName(SwaySetting::middle_emulation)))
    d.mid_emu = stb(libinput[GetSettingName(SwaySetting::middle_emulation)]);

  if (libinput.contains(GetSettingName(SwaySetting::scroll_method))) {
    d.scroll_methods = SEnum({"none", "two_finger", "edge", "on_button_down"});
    d.scroll_methods->select(
        libinput[GetSettingName(SwaySetting::scroll_method)]);
  }
  if (libinput.contains(GetSettingName(SwaySetting::scroll_button)))
    d.scroll_button =
        libinput[GetSettingName(SwaySetting::scroll_button)].get<int>();

  if (libinput.contains(GetSettingName(SwaySetting::dwt)))
    d.dwt = stb(libinput[GetSettingName(SwaySetting::dwt)]);
  if (libinput.contains(GetSettingName(SwaySetting::dwtp)))
    d.dwtp = stb(libinput[GetSettingName(SwaySetting::dwtp)]);

  if (libinput.contains(GetSettingName(SwaySetting::cal_mat))) {
    CalArr arr;
    int i = 0;
    for (auto& num : libinput[GetSettingName(SwaySetting::cal_mat)])
      arr[i++] = num.get<float>();
    d.cal_mat = arr;
  }

  device = d;
}

// Get info about devices from swaymsg calls.
void DeviceMan::parseSwaymsg() {
  // Parse swaymsg inputs
  json j_inputs;
  get_inputs_json(j_inputs, m_swaymsg);
  for (auto& json_dev : j_inputs) {
    auto device = json_dev.get<Opt<Device>>();
    if (device)
      m_Devices.push_back(device.value());
  }

  // Parse swaymsg output names
  json j_outputs;
  get_outputs_json(j_outputs, m_swaymsg);
  std::vector<std::string> outputs;
  for (auto& out : j_outputs)
    outputs.push_back(out.at("name"));

  // Save output names to map_to_output SEnum for devices that support it.
  // FIXME: For now we always set the same values initially, beacuse they cannot
  // be retrieved from swaymsg calls.
  //        We could workaround this by creating our own configs and loading
  //        them at start.
  SEnum e(outputs);
  e.options.push_back("*"); // Wildcard matching whole desktop layout.
  e.select("*");
  for (auto& device : m_Devices) {
    switch (device.type) {
    case DevType::pointer:
    case DevType::touchpad:
    case DevType::tablet_pad:
    case DevType::tablet_tool:
      device.map_to_output = e;
      device.map_to_region = std::array<int, 4>{0, 0, 0, 0};
      break;
    case DevType::keyboard:
      device.xkb_capslock = false;
      device.xkb_numlock = false;
      break;
    default:
      break;
    }
  }
}

// This function convert a value to string that works in sway config file or
// swaymsg calls.
template <typename T> std::string to_string(const T& val) {
  return std::to_string(val);
}

template <> std::string to_string(const SEnum& e) { return (std::string)e; }
template <> std::string to_string(const bool& b) { return bts(b); }
template <>
std::string to_string(const CalArr& arr) // Calibration array
{
  std::string out = "";
  for (int i = 0; i < (int)arr.size(); i++)
    out += std::to_string(arr[i]) + (i + 1 == (int)arr.size() ? "" : " ");
  return out;
}
template <> std::string to_string(const std::pair<SEnum, SEnum>& pair_e) {
  return (std::string)pair_e.first + (std::string)pair_e.second;
}
template <>
std::string
to_string(const std::array<int, 4>& region) // Map to region value (maybe should
                                            // be merged with CalArr above..)
{
  std::string out = "";
  for (int i = 0; i < 4; i++)
    out += std::to_string(region[i]) + (i == 3 ? "" : " ");
  return out;
}

// Return the input parameter in format '<setting name> <setting values...>' for
// use in swaymsg call
inline std::string get_input_param(SwaySetting setting,
                                   const std::string& value) {
  return GetSettingName(setting, false) + " " + value;
}

// Call `swaymsg input` command.
void sway_write(const std::string& swaymsg, const std::string& sway_id,
                SwaySetting setting, const std::string& value) {
  std::string cmd = swaymsg + " input '" + sway_id + "' '" +
                    get_input_param(setting, value) + "'";
  std::system(cmd.c_str());
}

// Wrapper write function checking if option should be written or not.
template <typename T, bool B>
void opt_write(std::string swaymsg, std::string sway_id, SwaySetting setting,
               Opt<T, B>& value) {
  if (value && value.m_Enabled)
    sway_write(swaymsg, sway_id, setting, to_string(value.value()));
}

// Write parameter as a part of config into 'out' string.
template <typename T, bool B>
void opt_conf(std::string& out, SwaySetting setting, Opt<T, B>& opt) {
  if (opt && opt.m_Enabled)
    out += "    " + get_input_param(setting, to_string(opt.value())) + "\n";
}

// Call opt_write or opt_conf depending on the is_write parameter with given
// arguments.
template <bool is_write, typename... Args, typename Str>
inline void opt_call(const std::string& swaymsg, const std::string& sway_id,
                     Str&& out, Args&&... args) {
  if constexpr (is_write)
    opt_write(swaymsg, sway_id, std::forward<Args>(args)...);
  else
    opt_conf(std::forward<Str>(out), std::forward<Args>(args)...);
}

// Write changes or generate config parameters into 'out' depending on the
// 'is_write' template parameter. NOTE: Why? Because I want to avoid having to
// type all those calls below multiple times for config generation and writes.
// NOTE: `out` is templated so that we can pass both lvalues and rvalues (when
// we dont use it) as parameter.
template <bool is_write, typename Str>
void opt_calls(const std::string& swaymsg, Device& dev, Str&& out) {
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::scroll_factor, dev.scroll_factor);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::repeat_delay, dev.repeat_delay);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::repeat_rate, dev.repeat_rate);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::tool_mode, dev.tool_mode);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::map_to_output, dev.map_to_output);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::map_to_region, dev.map_to_region);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::tap_to_click, dev.tap_to_click);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::tap_and_drag, dev.tap_and_drag);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::tap_drag_lock, dev.tap_drag_lock);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::tap_button_map, dev.tap_button_map);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::left_handed, dev.left_handed);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::natural_scroll, dev.nat_scroll);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::middle_emulation, dev.mid_emu);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::cal_mat, dev.cal_mat);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::scroll_method, dev.scroll_methods);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::scroll_button, dev.scroll_button);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::dwt, dev.dwt);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::dwtp, dev.dwtp);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::click_method, dev.click_methods);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::accel_profile, dev.accel_profiles);
  opt_call<is_write>(swaymsg, dev.sway_id, out, SwaySetting::accel_speed, dev.accel_speed);
}

void DeviceMan::ApplyChanges(int device_index, bool backup) {
  Device& dev = backup ? m_backupDevices[device_index] : m_Devices[device_index];
  sway_write(m_swaymsg, dev.sway_id, SwaySetting::send_events, bts(dev.send_events));
  opt_calls<true>(m_swaymsg, dev, "");
}

std::string DeviceMan::GetSwayConfig(int device_index, bool match_type) {
  Device& dev = m_Devices[device_index];
  std::string conf;
  if (match_type)
    conf = "input type:" + GetTypeName(dev.type) + " {\n";
  else
    conf = "input " + dev.sway_id + " {\n";

  conf +=
      "    " +
      get_input_param(SwaySetting::send_events, to_string(dev.send_events)) +
      "\n";
  opt_calls<false>(m_swaymsg, dev, conf);

  /* Config only options */
  if (dev.xkb_capslock && dev.xkb_capslock.m_Enabled)
    conf += "    xkb_capslock " + bts(dev.xkb_capslock.value()) + "\n";
  if (dev.xkb_numlock && dev.xkb_numlock.m_Enabled)
    conf += "    xkb_numlock " + bts(dev.xkb_numlock.value()) + "\n";

  conf += "}";
  return conf;
}
