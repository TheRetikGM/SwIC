/**
 * @file main.cpp
 */
#include <cstdio> // popen
#include <iostream>
#include <sstream> // std::istringstream
#include <string>
#include <vector>

#include "device_manager.h"
#include "config.h"
#include <imgui_internal.h>
#include <imguiwrapper.hpp>

#define MAX_SCROLL_FACTOR 5.0f
#define SWAYMSG_CMD "swaymsg"

// Easier creation of ImGui::Combo controls from SEnum type.
#define IMGUI_COMBO_SENUM(label, senum)                                        \
  ImGui::Combo(label, &senum.sel, &SEnumGetter, &senum.options,                \
               senum.options.size())

// ImGui hint on the same line as previous ImGui control.
#define IMGUI_HINT(sameline, text)                                             \
  if constexpr (sameline)                                                      \
    ImGui::SameLine();                                                         \
  HelpMarker(text)

class App {
  DeviceMan m_devMan;
  AppConfiguration m_config;

public:
  App(const AppConfiguration& config)
    : m_devMan(config.swaymsg_path)
    , m_config(config)
  {
    if (m_devMan.m_Devices.size() == 0)
      throw std::runtime_error("No devices found.");
  }

  // Display the help marker with given description.
  static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextUnformatted(desc);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }

  // Get data for selecting devices in ImGui::Combo
  static bool DevGetter(void* data, int n, const char** str) {
    auto devices = (std::vector<Device>*)data;
    *str = (*devices)[n].name.c_str();
    return true;
  }

  // Get data from SEnum for ImGui::Combo
  static bool SEnumGetter(void* data, int n, const char** str) {
    auto e = (SEnum*)data;
    *str = e->get(n).c_str();
    return true;
  }

  // Defaultable imgui block. Contents of this block is generated in `func`.
  template <typename Func, typename T, bool E>
  static void OptD(const char* id, Opt<T, E>& opt, Func func) {
    if (ImGui::ArrowButton(id, opt.m_Enabled ? ImGuiDir_Down : ImGuiDir_Right))
      opt.m_Enabled = !opt.m_Enabled;
    ImGui::BeginDisabled(!opt.m_Enabled);
    func();
    ImGui::EndDisabled();
  }

  static void GuiKeyboard(Device& dev) {
    if (dev.repeat_delay) {
      ImGui::InputInt("Repeat delay", &dev.repeat_delay.value(), 25, 100);
      IMGUI_HINT(true,
                 "Number of milliseconds before the key starts repeating");
    }
    if (dev.repeat_rate) {
      ImGui::InputInt("Repeat rate", &dev.repeat_rate.value(), 1, 5);
      IMGUI_HINT(true, "Number of characters to repeat per second");
    }

    const auto& imgui_xkb_capslock = [&dev]() {
      ImGui::SameLine();
      ImGui::Checkbox("xkb capslock", &dev.xkb_capslock.value());
      IMGUI_HINT(true, "Enable capslock on startup");
    };
    if (dev.xkb_capslock)
      OptD("##xkb_capslock", dev.xkb_capslock, imgui_xkb_capslock);

    const auto& imgui_xkb_numlock = [&dev]() {
      ImGui::SameLine();
      ImGui::Checkbox("xkb numlock", &dev.xkb_numlock.value());
      IMGUI_HINT(true, "Enable numlock on startup");
    };
    if (dev.xkb_numlock)
      OptD("##xkb_numlock", dev.xkb_numlock, imgui_xkb_numlock);
  }

  static void GuiTablet(Device& dev) {
    if (dev.tool_mode) {
      OptD("##tool_mode", dev.tool_mode, [&dev]() {
        ImGui::SameLine();
        ImGui::Text("Tool mode");
        IMGUI_HINT(true, "Currently this is not recieved from \nthe swaymsg "
                         "and always have default values.");
        ImGui::Indent();
        IMGUI_COMBO_SENUM("Tool", dev.tool_mode->first);
        IMGUI_COMBO_SENUM("Mode", dev.tool_mode->second);
        ImGui::Unindent();
      });
    }
  }

  // Call `slurp` for region select and save it to int[4] out.
  static bool CallSlurp(int* out) {
    FILE* slurp = popen("slurp -f '%x %y %w %h' 2>&1", "r");
    if (!slurp)
      return false;

    char out_line[100] = {};
    fgets(out_line, sizeof(out_line), slurp);
    std::string line(out_line);

    if (strstr(out_line, "cancelled")) {
      pclose(slurp);
      return false;
    }

    std::istringstream is(line);
    for (int i = 0; i < 4; i++)
      is >> out[i];

    pclose(slurp);
    return true;
  }

  static void GuiMapping(Device& dev) {
    if (dev.map_to_output) {
      OptD("##map_to_output", dev.map_to_output, [&dev]() {
        ImGui::SameLine();
        IMGUI_COMBO_SENUM("Map to output", dev.map_to_output.value());
      });
    }
    if (dev.map_to_region) {
      const auto& imgui_map_to_region = [&dev]() -> void {
        ImGui::SameLine();
        ImGui::Text("Map to region");
        ImGui::Indent();
        ImGui::InputInt4("Region", dev.map_to_region.value().data());
        if (ImGui::Button("Select"))
          App::CallSlurp(dev.map_to_region.value().data());
        IMGUI_HINT(true, "Requires slurp to be installed");
        ImGui::Unindent();
      };
      OptD("##map_to_region", dev.map_to_region, imgui_map_to_region);
    }
  }

  static void GuiLibinput(Device& dev) {
    ImGui::Checkbox("Send events", &dev.send_events);
    IMGUI_HINT(true, "Enable/Disable this device");

    if (dev.tap_to_click)
      ImGui::Checkbox("Tap to click", &dev.tap_to_click.value());
    if (dev.tap_and_drag)
      ImGui::Checkbox("Tap and drag", &dev.tap_and_drag.value());
    if (dev.tap_drag_lock)
      ImGui::Checkbox("Tap drag lock", &dev.tap_drag_lock.value());
    if (dev.tap_button_map)
      IMGUI_COMBO_SENUM("Tap button map", dev.tap_button_map.value());
    if (dev.left_handed) {
      ImGui::Checkbox("Left handed", &dev.left_handed.value());
      IMGUI_HINT(true, "Swap left and right buttons");
    }
    if (dev.nat_scroll) {
      ImGui::Checkbox("Natural scroll", &dev.nat_scroll.value());
      IMGUI_HINT(true, "Inverse scrolling");
    }
    if (dev.mid_emu) {
      ImGui::Checkbox("Middle emulation", &dev.mid_emu.value());
      IMGUI_HINT(true, "Middle click emulation");
    }
    if (dev.cal_mat) {
      ImGui::Text("Calibration matrix");
      ImGui::Indent();
      ImGui::InputFloat3("##cal_mat_1", dev.cal_mat->data());
      ImGui::InputFloat3("##cal_mat_2", dev.cal_mat->data() + 3);
      ImGui::Unindent();
    }
    if (dev.scroll_methods)
      IMGUI_COMBO_SENUM("Scroll method", dev.scroll_methods.value());
    if (dev.scroll_button) {
      ImGui::InputInt("Scroll button", &dev.scroll_button.value());
      {}
      IMGUI_HINT(true,
                 "Sets the button used for\nscroll_method on_button_down");
    }
    if (dev.scroll_factor) {
      ImGui::SliderFloat("Scroll factor", &dev.scroll_factor.value(), 0.0f,
                         MAX_SCROLL_FACTOR);
      IMGUI_HINT(true, "Scrolling speed is scaled by this value");
    }
    if (dev.dwt) {
      ImGui::Checkbox("DWT", &dev.dwt.value());
      IMGUI_HINT(true, "Disable while typing");
    }
    if (dev.dwtp) {
      ImGui::Checkbox("DWTP", &dev.dwtp.value());
      IMGUI_HINT(true, "Disable while trackpointing");
    }
    if (dev.click_methods)
      IMGUI_COMBO_SENUM("Click method", dev.click_methods.value());
    if (dev.accel_speed) {
      ImGui::SliderFloat("Accel speed", &dev.accel_speed.value(), -1.0f, 1.0f);
      IMGUI_HINT(true, "Basically pointer speed");
    }
    if (dev.accel_profiles) {
      IMGUI_COMBO_SENUM("Accel profile", dev.accel_profiles.value());
      IMGUI_HINT(
          true, "adaptive - Accelerative movement\n    flat - Linear movement");
    }
  }

  static void GuiOptions(Device& dev) {
    if (ImGui::BeginTabBar("TapBar options")) {
      if (dev.type == DevType::keyboard) {
        if (ImGui::BeginTabItem("Keyboard")) {
          GuiKeyboard(dev);
          ImGui::EndTabItem();
        }
      }
      if (dev.type == DevType::tablet_tool) {
        if (ImGui::BeginTabItem("Tablet")) {
          GuiTablet(dev);
          ImGui::EndTabItem();
        }
      }
      if (dev.map_to_output || dev.map_to_region) {
        if (ImGui::BeginTabItem("Mapping")) {
          GuiMapping(dev);
          ImGui::EndTabItem();
        }
      }
      ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
      if (ImGui::BeginTabItem("Libinput")) {
        GuiLibinput(dev);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }

  void GuiSwayConfig(int selected_device) {
    static bool match_type = false;
    ImGui::Checkbox("Match type", &match_type);

    std::string config = m_devMan.GetSwayConfig(selected_device, match_type);
    static bool copy = false;

    if (copy)
      ImGui::LogToClipboard();
    ImGui::TextWrapped("%s", config.c_str());
    if (copy) {
      ImGui::LogFinish();
      copy = false;
    }
    auto draw_list = ImGui::GetForegroundDrawList();
    draw_list->AddRect(
        ImGui::GetItemRectMin(),
        ImVec2(ImGui::GetContentRegionMax().x, ImGui::GetItemRectMax().y),
        ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Separator]));

    ImGui::Spacing();
    if (ImGui::Button("Copy"))
      copy = true;
    ImGui::Spacing();
  }

  void GuiRevertPopup(float dt, int selected_device,
                      float& current_rev_timeout) {
    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Revert?", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Changes will be\nreverted after: %4.1fs\n\n",
                  m_config.revert_timeout - current_rev_timeout);

      current_rev_timeout += dt;
      if (current_rev_timeout >= m_config.revert_timeout) {
        m_devMan.RevertChanges(selected_device);
        ImGui::CloseCurrentPopup();
      }
      ImGui::Separator();

      if (ImGui::Button("Keep"))
        ImGui::CloseCurrentPopup();
      ImGui::SameLine();
      ImGui::SetItemDefaultFocus();
      if (ImGui::Button("Revert")) {
        m_devMan.RevertChanges(selected_device);
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  void OnUpdate(float dt) {
    // Setup fullscreen imgui window.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("Fullscreen", NULL, flags);

    // Device selector combo.
    static int sel_dev = 0;
    ImGui::Combo("Device", &sel_dev, &DevGetter, &m_devMan.m_Devices,
                 m_devMan.m_Devices.size());
    ImGui::Separator();

    // Basic device information.
    Device& dev = m_devMan.m_Devices[sel_dev];
    ImGui::LabelText("ID", "%s", dev.sway_id.c_str());
    ImGui::LabelText("Type", "%s", GetTypeName(dev.type).c_str());

    // The Options and sway config tree nodes.
    ImGui::Separator();
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Options")) {
      GuiOptions(dev);
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Sway config")) {
      GuiSwayConfig(sel_dev);
      ImGui::TreePop();
    }

    // Apply button opening Revert popup if m_safeMode is enabled.
    ImGui::Separator();
    static float revert_time = 0.0f;
    if (ImGui::Button("Apply")) {
      m_devMan.ApplyChanges(sel_dev);

      if (m_config.safe_mode) {
        revert_time = 0.0f;
        ImGui::OpenPopup("Revert?");
      }
    }
    GuiRevertPopup(dt, sel_dev, revert_time);

    // Revert button
    ImGui::SameLine();
    if (ImGui::Button("Revert")) {
      m_devMan.RestoreBackup(sel_dev);
    }

    ImGui::End(); // Fullscreen window
  }
};

Configuration get_default_config() {
  ImWrap::ContextDefinition imwrap;
  imwrap.window_title = "Sway Input Configurator";
  imwrap.window_width = 500;
  imwrap.window_height = 700;
  imwrap.exit_key = GLFW_KEY_ESCAPE;
  imwrap.imgui_theme = ImWrap::ImGuiTheme::dark;
  imwrap.window_hints[GLFW_RESIZABLE] = GLFW_FALSE; // Disable resizing, so that
                                                 // the window is floating by default.
  imwrap.imgui_config_flags &= ~ImGuiConfigFlags_DockingEnable;

  AppConfiguration app;
  app.safe_mode = true;
  app.swaymsg_path = SWAYMSG_CMD;
  app.revert_timeout = 10.0f;

  return { imwrap, app };
}

int main() {
  Configuration config = load_config().value_or(get_default_config());

  try {
    auto context = ImWrap::Context::Create(config.imwrap);

    // Disable imgui.ini file.
    ImGuiIO& imgui_io = ImGui::GetIO();
    imgui_io.IniFilename = nullptr;
    imgui_io.LogFilename = nullptr;

    App app(config.app);
    ImWrap::run(context, app);
    ImWrap::Context::Destroy(context);
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    save_config(config);
    return 1;
  }

  if (!save_config(config))
    std::cerr << "Failed to save config." << std::endl;
  return 0;
}
