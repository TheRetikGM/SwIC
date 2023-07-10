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
#include "gui/gui.h"
#include <imgui_internal.h>
#include <imguiwrapper.hpp>

class App {
  DeviceMan m_devMan;
  Configuration m_config;
  gui::DeviceEditor m_deviceEditor;
  gui::MenuBar m_menuBar;
  // gui::Settings m_settings;

public:
  App(Configuration& config)
    : m_devMan(config.app.swaymsg_path)
    , m_config(config)
    , m_deviceEditor(m_devMan, m_config)
  {
    if (m_devMan.m_Devices.size() == 0)
      throw std::runtime_error("No devices found.");
  }

  void OnUpdate(float dt) {
    m_menuBar.OnUpdate(dt);
    m_deviceEditor.OnUpdate(dt);
    // m_settings.OnUpdate(dt);
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
  app.swaymsg_path = "swaymsg";
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

    App app(config);
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
