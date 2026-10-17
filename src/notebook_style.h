// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>

namespace notebook {
void closePopups();
inline const ImVec4 text{.92f,.91f,.87f,1},muted{.66f,.68f,.67f,1};
inline const ImVec4 brass{.84f,.71f,.44f,1},receiving{.51f,.79f,.63f,1};
inline void hint(const char* text){
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)){
        ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetFontSize()*29);
        ImGui::TextUnformatted(text);ImGui::PopTextWrapPos();ImGui::EndTooltip();
    }
}
inline void tabActivity(bool recent){
    if(!ImGui::IsItemVisible())return;
    const auto min=ImGui::GetItemRectMin(),max=ImGui::GetItemRectMax();
    auto* draw=ImGui::GetWindowDrawList();
    draw->PushClipRect(min,max,true);
    draw->AddCircleFilled({min.x+ImGui::GetStyle().FramePadding.x+3.5f,(min.y+max.y)/2},3.5f,
        ImGui::GetColorU32(recent?ImVec4{.28f,.86f,.39f,1}:ImVec4{.98f,.78f,.20f,1}),12);
    draw->PopClipRect();
    hint(recent?"Green: RTTY was received within the last minute. Each new burst restarts the minute. Select this tab to read it; Radio tuning stays unchanged.":
        "Yellow: no confirmed RTTY activity in the last minute. New reception turns this green again. Select this tab to read its received text.");
}
inline float statusHeight(){return ImGui::GetTextLineHeight()+8;}
// Draw directly against the window edges. Changing text never adds layout rows.
inline void statusBar(const char* summary,const char* details,bool warning){
    const auto pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
    const ImVec2 min{pos.x,pos.y+size.y-statusHeight()},max{pos.x+size.x,pos.y+size.y};
    auto* draw=ImGui::GetWindowDrawList();
    draw->PushClipRect(pos,{pos.x+size.x,pos.y+size.y},false);
    draw->AddRectFilled(min,max,ImGui::GetColorU32(ImVec4{.12f,.14f,.15f,1}));
    draw->AddLine(min,{max.x,min.y},ImGui::GetColorU32(ImGuiCol_Separator));
    draw->PushClipRect({min.x+8,min.y},{max.x-8,max.y},true);
    draw->AddText({min.x+8,min.y+4},ImGui::GetColorU32(warning?brass:muted),summary);
    draw->PopClipRect();
    draw->PopClipRect();
    if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect(min,max)){
        ImGui::BeginTooltip();ImGui::PushTextWrapPos(ImGui::GetFontSize()*35);
        ImGui::TextUnformatted(details);ImGui::PopTextWrapPos();ImGui::EndTooltip();
    }
}
// Scoped instrument-panel colors. Never change the host's global theme/fonts.
class Style {
public:
    Style(){
        ImGui::PushStyleColor(ImGuiCol_WindowBg,{.075f,.080f,.084f,1});
        ImGui::PushStyleColor(ImGuiCol_PopupBg,{.095f,.100f,.104f,1});
        ImGui::PushStyleColor(ImGuiCol_Text,text);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,muted);
        ImGui::PushStyleColor(ImGuiCol_Border,{.24f,.26f,.27f,1});
        ImGui::PushStyleColor(ImGuiCol_Button,{.18f,.20f,.21f,1});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{.27f,.29f,.29f,1});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,{.34f,.31f,.24f,1});
        ImGui::PushStyleColor(ImGuiCol_FrameBg,{.12f,.14f,.15f,1});
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,{.22f,.24f,.25f,1});
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,{.27f,.28f,.28f,1});
        ImGui::PushStyleColor(ImGuiCol_CheckMark,brass);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,brass);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,{.95f,.84f,.62f,1});
        ImGui::PushStyleColor(ImGuiCol_Tab,{.13f,.15f,.16f,1});
        ImGui::PushStyleColor(ImGuiCol_TabActive,{.32f,.28f,.20f,1});
        ImGui::PushStyleColor(ImGuiCol_TabHovered,{.39f,.34f,.24f,1});
        ImGui::PushStyleColor(ImGuiCol_Separator,{.25f,.27f,.28f,1});
        ImGui::PushStyleColor(ImGuiCol_Header,{.25f,.23f,.18f,1});
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,{.34f,.30f,.22f,1});
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,{.40f,.34f,.23f,1});
        ImGui::PushStyleColor(ImGuiCol_TitleBg,{.12f,.14f,.15f,1});
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive,{.18f,.20f,.21f,1});
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,{.55f,.43f,.22f,.55f});
        ImGui::PushStyleColor(ImGuiCol_NavHighlight,brass);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{14,12});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{8,5});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{10,8});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,6);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,3);
        ImGui::PushStyleVar(ImGuiStyleVar_TabRounding,3);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,6);
    }
    ~Style(){ImGui::PopStyleVar(7);ImGui::PopStyleColor(25);}
};
inline void flow(float width){
    if(ImGui::GetItemRectMax().x+10+width<ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x)ImGui::SameLine();
}
inline void flow(const char* label){flow(ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x);}
}
