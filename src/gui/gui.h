/**
 * @brief Provides GUI for editing device properties.
 * @file device_editor.hpp
 * @author TheRetikGM (theretikgm@gmail.com)
 */
#pragma once
#include "../device_manager.h"
#include "../config.h"
#include <imgui_internal.h>
#include <imgui.h>

/// Combo button created from SEnum (selectable string enum)
#define IMGUI_COMBO_SENUM(label, senum)                                        \
  ImGui::Combo(label, &senum.sel, &gui::senum_getter, &senum.options,          \
               senum.options.size())

/// Help marker on the same line or below
#define IMGUI_HINT(sameline, text)                                             \
  if constexpr (sameline)                                                      \
    ImGui::SameLine();                                                         \
  gui::help_marker(text)

namespace gui {
  /**
   * @brief Create imgui help marker
   * @param desc Text to show as help
   */
  void help_marker(const char* desc);
  /// ImGui::Combo getter callback for use with std::vector<Device>.
  bool device_getter(void* data, int n, const char** str);
  /// ImGui::Combo getter callback for use with SEnum.
  bool senum_getter(void* data, int n, const char** str);
  /**
   * @brief Option which can be enabled or disabled using arrow button.
   * @param id Id to identify arrow button with. Prefix with `##` for hidden id.
   * @param opt Option which is being enabled or disabled.
   * @param func Generator function for contents of this option. Doesn't take any arguments.
   */
  template <typename Func, typename T, bool E>
  void opt_toggle(const char* id, Opt<T, E>& opt, Func func) {
    if (ImGui::ArrowButton(id, opt.m_Enabled ? ImGuiDir_Down : ImGuiDir_Right))
      opt.m_Enabled = !opt.m_Enabled;
    ImGui::BeginDisabled(!opt.m_Enabled);
    func();
    ImGui::EndDisabled();
  }

  /// Base class for GUI elements.
  class Gui {
  public:
    virtual void OnUpdate(float dt) = 0;
  };

  /// Editor for device properties.
  class DeviceEditor : public Gui {
    const float MAX_SCROLL_FACTOR = 5.0f;
  public:
    /**
     * @brief Construct new instance of DeviceEditor
     * @param manager Device manager to use for editing device properties.
     * @param config Program configuration
     */
    DeviceEditor(DeviceMan& manager, Configuration& config);

    /// Construct and update all GUI components.
    void OnUpdate(float dt) override;

  private:
    DeviceMan& m_manager;
    Device* m_device{ nullptr };
    Configuration& m_config;

    void guiKeyboard();
    void guiTablet();
    void guiMapping();
    void guiLibInput();
    void guiOptions();
    void guiSwayConfig(int selected_device);
    void guiRevertPopup(float dt, int selected_device, float& current_timeout);
    bool callSlurp(int* out);
  };

  /// TODO: Application settings.
  // class Settings : public Gui {};

  /// TODO: The application main bar.
  class MenuBar : public Gui {
  public:
    void OnUpdate(float dt) override;
  };

}; // gui
