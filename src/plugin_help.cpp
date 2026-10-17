// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"

void CWNotebook::openNotebookModal(int modal){
    if(modal==1){cwSettingsOpen_=true;ImGui::OpenPopup("CW Settings");}
    if(modal==2){helpOpen_=true;ImGui::OpenPopup("How to use");}
    if(modal==3){signalList_.refresh();signalsOpen_=true;ImGui::OpenPopup("RTTY signals");}
    if(modal==4)cwSettingsOpen_=helpOpen_=signalsOpen_=false;
}
void CWNotebook::operatingGuide(){
    ImGui::TextColored(notebook::brass,"RTTY across the capture");
    ImGui::TextWrapped("Start the receiver and enable RTTY in the toolbar. The choice is remembered. CW has its own checkbox for independent pause/resume. Confirmed streams open frequency tabs without changing your tuning or focus. A dot marks unread text. Selecting a tab only reads it; Tune Radio is an explicit listening action.");
    ImGui::Spacing();ImGui::TextWrapped("Crowded band? Open Signals, pin the frequencies you want, or choose Decode only pinned signals. Rows stay fixed until Refresh list; their states update live. Up to 16 candidates decode together. Ignore & close prevents an unwanted frequency reopening; clear ignores in Signals. Pins and ignores are remembered per host. Close all tabs clears RTTY tabs and pending text, keeps your choices, and returns to CW. Fresh traffic can open new tabs.");
    ImGui::Spacing();ImGui::TextWrapped("Every eligible RTTY pair inside Radio's tuned filter is checked first, following frequency, mode and bandwidth changes automatically. Other captured frequencies keep scanning. Pins come next; ignored signals and Decode only pinned signals are still respected. Both RTTY tones must fit inside the filter for tuned priority.");
    ImGui::Spacing();ImGui::TextWrapped("The filled circle before each RTTY frequency stays green for one minute after confirmed activity, then turns yellow. Each returning burst restarts the minute. Follow RTTY scrolls to new text; turning it off does not stop decoding. Compact RY folds exact long RYRY test patterns for reading. Copy to clipboard includes the displayed view plus band, frequency, time and decoding metadata. Turn Compact RY off to copy every test letter. Hover controls and the fixed status bar for explanations.");
    ImGui::Spacing();ImGui::TextWrapped("Repeated single-letter patterns alone do not confirm RTTY. A new stream needs consistent framing and varied payload, or an alternating RY test. Short, repetitive or weak real signals may stay Checking longer. Confirmed reception is not corrected or filtered.");
    ImGui::Spacing();ImGui::TextWrapped("Tabs show estimated mark frequency rounded to 1 Hz. Signals lists pair centers before polarity is known. Receiver calibration, noise and FFT resolution limit accuracy. RTTY text stays in memory; copy it before closing or quitting. Weak, overlapping or unsupported signals may be missed; two tones alone do not prove RTTY.");
    ImGui::Spacing();ImGui::TextDisabled("45.45 / 50 / 75 / 100 baud | 170 / 200 / 425 / 450 / 850 Hz | five-bit ITA2, both polarities");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::TextColored(notebook::brass,"CW notebook");
    ImGui::TextWrapped("Select CW, USB, LSB, DSB or AM and place one Morse signal inside Radio's filter. Narrow the filter to exclude nearby signals. USB receives above the dial; LSB below; DSB and AM on either side. AM offers keyed RF carrier or AM audio tone input.");
    ImGui::Spacing();ImGui::TextWrapped("Mixed dot/dash evidence establishes timing. Provisional text can appear after about two seconds and may change; only confirmed text is autosaved. Manual WPM can interpret held intervals. Default automatic speed is 5-45 WPM; CW Settings offers manual controls and an extended range.");
    ImGui::Spacing();ImGui::TextWrapped("Changing frequency starts fresh timing and a new line. Relearn restarts detection and discards held intervals while keeping confirmed text. Original and Timing + fade recovery remain separate; changing views does not rewrite the original.");
    ImGui::Spacing();ImGui::TextWrapped("Select text and press Cmd+C, or right-click for copy options. Follow changes scrolling only. Clear view keeps the log. Clear log archives the previous file before clearing. Markdown autosave applies to CW; RTTY copying is separate.");
}
void CWNotebook::notebookModals(const cw::Snapshot& s){
    const auto* viewport=ImGui::GetMainViewport();
    const ImVec2 size{std::max(300.f,std::min(740.f,viewport->WorkSize.x-32)),std::max(280.f,std::min(600.f,viewport->WorkSize.y-32))};
    auto modal=[&](const char* title,bool& open,auto content){
        ImGui::SetNextWindowSize(size,ImGuiCond_Appearing);
        ImGui::SetNextWindowPos({viewport->WorkPos.x+viewport->WorkSize.x/2,viewport->WorkPos.y+viewport->WorkSize.y/2},ImGuiCond_Appearing,{.5f,.5f});
        if(ImGui::BeginPopupModal(title,&open,ImGuiWindowFlags_NoSavedSettings)){
            controlsPopupOpen_=true;
            ImGui::BeginChild("modal_content",{0,-ImGui::GetFrameHeightWithSpacing()-8},false);content();ImGui::EndChild();
            ImGui::Separator();if(ImGui::Button("Close",{100,0})){open=false;ImGui::CloseCurrentPopup();}
            notebook::hint("Return to the notebook. Reception continues; your choices are kept.");
            ImGui::EndPopup();
        }
    };
    modal("CW Settings",cwSettingsOpen_,[&]{decoderControls(s);});
    modal("How to use",helpOpen_,[&]{operatingGuide();});
    modal("RTTY signals",signalsOpen_,[&]{signalPicker();});
}
