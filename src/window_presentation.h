// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <algorithm>

inline void prepareNotebookWindow(bool& present) {
    const auto* viewport=ImGui::GetMainViewport();
    const ImVec2 available{std::max(1.f,viewport->WorkSize.x-24),std::max(1.f,viewport->WorkSize.y-24)};
    ImGui::SetNextWindowSizeConstraints(
        {std::min(720.f,available.x),std::min(420.f,available.y)},
        {std::min(1800.f,available.x),std::min(1400.f,available.y)});
    if(!present)return;
    const ImVec2 size{std::min(870.f,available.x),std::min(620.f,available.y)};
    ImGui::SetNextWindowSize(size,ImGuiCond_Always);
    ImGui::SetNextWindowPos({viewport->WorkPos.x+(viewport->WorkSize.x-size.x)/2,
                           viewport->WorkPos.y+(viewport->WorkSize.y-size.y)/2},ImGuiCond_Always);
    ImGui::SetNextWindowFocus();
    present=false;
}
