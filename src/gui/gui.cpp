/**
 * @brief Definitions of free functions from gui.h
 * @file gui.cpp
 */
#include "gui.h"

using namespace gui;

void gui::help_marker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

bool gui::device_getter(void* data, int n, const char** str) {
  auto devices = (std::vector<Device>*)data;
  *str = (*devices)[n].name.c_str();
  return true;
}

bool gui::senum_getter(void* data, int n, const char** str) {
  auto e = (SEnum*)data;
  *str = e->get(n).c_str();
  return true;
}
