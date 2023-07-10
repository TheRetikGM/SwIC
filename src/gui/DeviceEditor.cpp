/**
 * @brief Definition of gui::DeviceEditor class methods
 * @file DeviceEditor.cpp
 */
#include "gui.h"

using namespace gui;

DeviceEditor::DeviceEditor(DeviceMan& manager, Configuration& config)
  : m_manager(manager)
  , m_config(config)
{
  // NOTE: Assuming that the devices vector will not change during frame.
  m_device = &m_manager.m_Devices[0];
}

void DeviceEditor::OnUpdate(float dt) {
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
  ImGui::Combo("Device", &sel_dev, &device_getter, &m_manager.m_Devices,
               m_manager.m_Devices.size());
  ImGui::Separator();

  // Basic device information.
  m_device = &m_manager.m_Devices[sel_dev];
  ImGui::LabelText("ID", "%s", m_device->sway_id.c_str());
  ImGui::LabelText("Type", "%s", GetTypeName(m_device->type).c_str());

  // The Options and sway config tree nodes.
  ImGui::Separator();
  ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("Options")) {
    guiOptions();
    ImGui::TreePop();
  }
  if (ImGui::TreeNode("Sway config")) {
    guiSwayConfig(sel_dev);
    ImGui::TreePop();
  }

  // Apply button opening Revert popup if m_safeMode is enabled.
  ImGui::Separator();
  static float revert_time = 0.0f;
  if (ImGui::Button("Apply")) {
    m_manager.ApplyChanges(sel_dev);

    if (m_config.app.safe_mode) {
      revert_time = 0.0f;
      ImGui::OpenPopup("Revert?");
    }
  }
  guiRevertPopup(dt, sel_dev, revert_time);

  // Revert button
  ImGui::SameLine();
  if (ImGui::Button("Revert")) {
    m_manager.RestoreBackup(sel_dev);
  }

  ImGui::End(); // Fullscreen window
}

void DeviceEditor::guiKeyboard() {
  if (m_device->repeat_delay) {
    ImGui::InputInt("Repeat delay", &m_device->repeat_delay.value(), 25, 100);
    IMGUI_HINT(true,
               "Number of milliseconds before the key starts repeating");
  }
  if (m_device->repeat_rate) {
    ImGui::InputInt("Repeat rate", &m_device->repeat_rate.value(), 1, 5);
    IMGUI_HINT(true, "Number of characters to repeat per second");
  }
  const auto& imgui_xkb_capslock = [this]() {
    ImGui::SameLine();
    ImGui::Checkbox("xkb capslock", &m_device->xkb_capslock.value());
    IMGUI_HINT(true, "Enable capslock on startup");
  };
  if (m_device->xkb_capslock)
    opt_toggle("##xkb_capslock", m_device->xkb_capslock, imgui_xkb_capslock);

  const auto& imgui_xkb_numlock = [this]() {
    ImGui::SameLine();
    ImGui::Checkbox("xkb numlock", &m_device->xkb_numlock.value());
    IMGUI_HINT(true, "Enable numlock on startup");
  };
  if (m_device->xkb_numlock)
    opt_toggle("##xkb_numlock", m_device->xkb_numlock, imgui_xkb_numlock);
}

void DeviceEditor::guiTablet() {
  if (m_device->tool_mode) {
    opt_toggle("##tool_mode", m_device->tool_mode, [this]() {
      ImGui::SameLine();
      ImGui::Text("Tool mode");
      IMGUI_HINT(true, "Currently this is not recieved from \nthe swaymsg "
                       "and always have default values.");
      ImGui::Indent();
      IMGUI_COMBO_SENUM("Tool", m_device->tool_mode->first);
      IMGUI_COMBO_SENUM("Mode", m_device->tool_mode->second);
      ImGui::Unindent();
    });
  }
}

void DeviceEditor::guiMapping() {
  if (m_device->map_to_output) {
    opt_toggle("##map_to_output", m_device->map_to_output, [this]() {
      ImGui::SameLine();
      IMGUI_COMBO_SENUM("Map to output", m_device->map_to_output.value());
    });
  }
  if (m_device->map_to_region) {
    const auto& imgui_map_to_region = [this]() -> void {
      ImGui::SameLine();
      ImGui::Text("Map to region");
      ImGui::Indent();
      ImGui::InputInt4("Region", m_device->map_to_region.value().data());
      if (ImGui::Button("Select"))
        callSlurp(m_device->map_to_region.value().data());
      IMGUI_HINT(true, "Requires slurp to be installed");
      ImGui::Unindent();
    };
    opt_toggle("##map_to_region", m_device->map_to_region, imgui_map_to_region);
  }
}

void DeviceEditor::guiLibInput() {
  ImGui::Checkbox("Send events", &m_device->send_events);
  IMGUI_HINT(true, "Enable/Disable this device");

  if (m_device->tap_to_click)
    ImGui::Checkbox("Tap to click", &m_device->tap_to_click.value());
  if (m_device->tap_and_drag)
    ImGui::Checkbox("Tap and drag", &m_device->tap_and_drag.value());
  if (m_device->tap_drag_lock)
    ImGui::Checkbox("Tap drag lock", &m_device->tap_drag_lock.value());
  if (m_device->tap_button_map)
    IMGUI_COMBO_SENUM("Tap button map", m_device->tap_button_map.value());
  if (m_device->left_handed) {
    ImGui::Checkbox("Left handed", &m_device->left_handed.value());
    IMGUI_HINT(true, "Swap left and right buttons");
  }
  if (m_device->nat_scroll) {
    ImGui::Checkbox("Natural scroll", &m_device->nat_scroll.value());
    IMGUI_HINT(true, "Inverse scrolling");
  }
  if (m_device->mid_emu) {
    ImGui::Checkbox("Middle emulation", &m_device->mid_emu.value());
    IMGUI_HINT(true, "Middle click emulation");
  }
  if (m_device->cal_mat) {
    ImGui::Text("Calibration matrix");
    ImGui::Indent();
    ImGui::InputFloat3("##cal_mat_1", m_device->cal_mat->data());
    ImGui::InputFloat3("##cal_mat_2", m_device->cal_mat->data() + 3);
    ImGui::Unindent();
  }
  if (m_device->scroll_methods)
    IMGUI_COMBO_SENUM("Scroll method", m_device->scroll_methods.value());
  if (m_device->scroll_button) {
    ImGui::InputInt("Scroll button", &m_device->scroll_button.value());
    {}
    IMGUI_HINT(true,
               "Sets the button used for\nscroll_method on_button_down");
  }
  if (m_device->scroll_factor) {
    ImGui::SliderFloat("Scroll factor", &m_device->scroll_factor.value(), 0.0f,
                       MAX_SCROLL_FACTOR);
    IMGUI_HINT(true, "Scrolling speed is scaled by this value");
  }
  if (m_device->dwt) {
    ImGui::Checkbox("DWT", &m_device->dwt.value());
    IMGUI_HINT(true, "Disable while typing");
  }
  if (m_device->dwtp) {
    ImGui::Checkbox("DWTP", &m_device->dwtp.value());
    IMGUI_HINT(true, "Disable while trackpointing");
  }
  if (m_device->click_methods)
    IMGUI_COMBO_SENUM("Click method", m_device->click_methods.value());
  if (m_device->accel_speed) {
    ImGui::SliderFloat("Accel speed", &m_device->accel_speed.value(), -1.0f, 1.0f);
    IMGUI_HINT(true, "Basically pointer speed");
  }
  if (m_device->accel_profiles) {
    IMGUI_COMBO_SENUM("Accel profile", m_device->accel_profiles.value());
    IMGUI_HINT(
        true, "adaptive - Accelerative movement\n    flat - Linear movement");
  }
}

void DeviceEditor::guiOptions() {
  if (ImGui::BeginTabBar("TapBar options")) {
    if (m_device->type == DevType::keyboard) {
      if (ImGui::BeginTabItem("Keyboard")) {
        guiKeyboard();
        ImGui::EndTabItem();
      }
    }
    if (m_device->type == DevType::tablet_tool) {
      if (ImGui::BeginTabItem("Tablet")) {
        guiTablet();
        ImGui::EndTabItem();
      }
    }
    if (m_device->map_to_output || m_device->map_to_region) {
      if (ImGui::BeginTabItem("Mapping")) {
        guiMapping();
        ImGui::EndTabItem();
      }
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::BeginTabItem("Libinput")) {
      guiLibInput();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void DeviceEditor::guiSwayConfig(int selected_device) {
  static bool match_type = false;
  ImGui::Checkbox("Match type", &match_type);

  std::string config = m_manager.GetSwayConfig(selected_device, match_type);
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

void DeviceEditor::guiRevertPopup(float dt, int selected_device, float& current_timeout) {
  // Always center this window when appearing
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal("Revert?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Changes will be\nreverted after: %4.1fs\n\n",
                m_config.app.revert_timeout - current_timeout);

    current_timeout += dt;
    if (current_timeout >= m_config.app.revert_timeout) {
      m_manager.RevertChanges(selected_device);
      ImGui::CloseCurrentPopup();
    }
    ImGui::Separator();

    if (ImGui::Button("Keep"))
      ImGui::CloseCurrentPopup();
    ImGui::SameLine();
    ImGui::SetItemDefaultFocus();
    if (ImGui::Button("Revert")) {
      m_manager.RevertChanges(selected_device);
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

bool DeviceEditor::callSlurp(int* out) {
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

