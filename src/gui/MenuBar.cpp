/**
 * @breif Definition of gui::MenuBar methods
 * @file MenuBar.cpp
 */
#include "gui.h"

using namespace gui;

void MenuBar::OnUpdate(float) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New preset..")) {};
      if (ImGui::MenuItem("Select preset")) {};
      if (ImGui::MenuItem("Delete preset")) {};
      ImGui::Separator();
      if (ImGui::MenuItem("Load preset..")) {};
      if (ImGui::MenuItem("Export presets..")) {};
      ImGui::Separator();
      if (ImGui::MenuItem("Settings")) {};
      ImGui::Separator();
      if (ImGui::MenuItem("Quit", "Esc")) {};

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Apply chanages")) {};
      if (ImGui::MenuItem("Revert to inital")) {};
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

