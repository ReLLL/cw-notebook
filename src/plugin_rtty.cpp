// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
#include "rtty_presentation.h"
#include "rtty_report.h"
#include <cstdio>

void CWNotebook::setRttyMonitoring(bool enabled){
    rttyEnabled_=enabled;rttyEnabledView_=enabled;
    cw::host::setRttyEnabled(name_,enabled);
    if(enabled&&!rtty_){rtty_=std::make_unique<RttyReceiver>();rtty_->selection(rttySelection_);}
    if(rtty_)rtty_->enabled(enabled_&&enabled);
    metadata();
}
void CWNotebook::listenRtty(double mark){
    if(!cw::host::listenRTTY(streamName_,mark))rttyNotice_="Move the capture center so Radio's full 3.2 kHz filter fits before listening.";
    else rttyNotice_.clear();
}
void CWNotebook::rttyControls(){
    bool cwEnabled=!engine_.snapshot().paused;
    if(ImGui::Checkbox("CW",&cwEnabled))engine_.command("pause",cwEnabled?"0":"1");
    notebook::hint("Enable or pause CW decoding. Existing text and Markdown logs are kept. Turning CW back on relearns timing; RTTY continues independently.");
    ImGui::SameLine();
    bool enable=rttyEnabled_;
    if(ImGui::Checkbox("RTTY",&enable))setRttyMonitoring(enable);
    notebook::hint("Scan the full captured RF spectrum for RTTY. All eligible pairs inside Radio's current filter get first priority, followed by pins and other signals. Turn off to stop RTTY processing and keep existing text. This choice is remembered.");
    notebook::flow("Close all tabs");
    if(ImGui::Button("Close all tabs")){if(rtty_)rtty_->closeAll();selectCW_=true;}
    notebook::hint("Close all RTTY tabs and discard their text. CW stays open. Monitoring continues, so fresh traffic can open new tabs. Copy wanted text first; RTTY is not autosaved.");
    notebook::flow("CW Settings");if(ImGui::Button("CW Settings"))openNotebookModal(1);
    notebook::hint("Open CW speed, tone, recovery and diagnostic controls. RTTY has separate controls in its tabs and Signals.");
    notebook::flow("How to use");if(ImGui::Button("How to use"))openNotebookModal(2);
    notebook::hint("Open the operating guide for CW and RTTY, including tuning, copying, pins and receiver limits.");
    notebook::flow("Signals...");if(ImGui::Button("Signals..."))openNotebookModal(3);
    notebook::hint("Choose which RTTY frequencies to decode. Pin priority stations, ignore unwanted ones, or decode only your pins. The table stays still until Refresh list.");
}
void CWNotebook::rttyTabs(float footer){
    if(!rtty_)return;
    auto snapshot=rtty_->snapshot();
    for(auto it=rttyViews_.begin();it!=rttyViews_.end();){
        bool exists=std::any_of(snapshot->stations.begin(),snapshot->stations.end(),[&](const auto& s){return s.id==it->first;});
        if(!exists)it=rttyViews_.erase(it);else ++it;
    }
    for(const auto& s:snapshot->stations){
        char title[96];std::snprintf(title,sizeof(title),"    %.3f kHz RTTY###rtty_%llu",s.frequency/1000,static_cast<unsigned long long>(s.id));
        bool open=true;
        auto& view=rttyViews_[s.id];
        auto flags=view.seen==s.text?ImGuiTabItemFlags_None:ImGuiTabItemFlags_UnsavedDocument;
        const bool selected=ImGui::BeginTabItem(title,&open,flags);
        notebook::tabActivity(rtty::recentText(s.lastTextAt,rtty::monotonicSeconds()));
        if(selected){
            view.seen=s.text;
            ImGui::TextDisabled("Estimated mark %.0f Hz | %.2f baud | %.0f Hz shift | %s | %s",s.frequency,s.baud,s.shift,s.reverse?"Reverse":"Normal",s.active?"Receiving":"Inactive");
            if(ImGui::Button("Copy to clipboard"))view.transcript.copyRTTYReport(s,*snapshot,view.compact);
            notebook::hint("Copy the displayed RTTY text with date, day, local time zone, band, estimated mark frequency, pair center, baud, shift, polarity and capture details. Turn off Compact RY to include every RY test letter.");
            notebook::flow("Explain with AI...");
            if(ImGui::Button("Explain with AI..."))ImGui::OpenPopup("notebook_rtty_ai_actions");
            notebook::hint("Choose ChatGPT, Claude or Gemini. Shares this tab's displayed RTTY text and reception metadata when you choose a site, and copies the complete prompt. You still press Send.");
            controlsPopupOpen_|=ImGui::IsPopupOpen("notebook_rtty_ai_actions");
            if(ImGui::BeginPopup("notebook_rtty_ai_actions")){
                const bool hasText=!s.text.empty();
                if(!hasText)ImGui::TextDisabled("Receive text first.");
                for(int provider=0;provider<3;++provider){
                    const char* names[]{"ChatGPT","Claude","Gemini"};
                    if(ImGui::MenuItem(names[provider],nullptr,false,hasText))
                        openAnalysisPrompt(provider,rtty::analysisPrompt(s,*snapshot,view.compact,std::time(nullptr),cw::host::grid(name_)));
                    notebook::hint(provider==2?"Copy the full RTTY analysis prompt and open Gemini. Paste and send it yourself.":"Open the site with this tab's displayed RTTY text and metadata. The full prompt is also copied for pasting; you still press Send.");
                }
                ImGui::Separator();ImGui::TextDisabled("Full prompt copied for every site.");
                ImGui::TextDisabled("Gemini / long transcripts: paste and send.");
                ImGui::EndPopup();
            }
            notebook::flow(110);ImGui::Checkbox("Follow RTTY",&view.follow);
            notebook::hint("Automatically scroll to the newest text in this tab. Turn off or select text to hold your reading position. Decoding continues either way.");
            notebook::flow(100);bool pinned=rtty::contains(rttySelection_.pinned,s.center,s.shift);
            if(ImGui::Checkbox("Pin signal",&pinned))pinRtty(s.center,s.shift,pinned);
            notebook::hint("Give this frequency priority over automatic candidates when the 16 decoder slots are busy. A pin is remembered and works with Decode only pinned signals. It does not tune Radio or freeze this tab.");
            notebook::flow("Compact RY");ImGui::Checkbox("Compact RY",&view.compact);
            notebook::hint("Show long, exact RYRY test sequences as [RY test pattern] to keep messages readable. These letters are transmitted test traffic. Turn off to display and copy them all. The report labels which view you copied.");
            notebook::flow("Ignore & close");if(ImGui::Button("Ignore & close"))ignoreRtty(s.center,s.shift,true);
            notebook::hint("Stop decoding this frequency, discard its tab and prevent it reopening. Undo in Signals by unticking Ignore or choosing Clear ignored. Copy wanted text first.");
            notebook::flow("Tune Radio");if(ImGui::Button("Tune Radio")){
                listenRtty(s.frequency);
            }
            notebook::hint("Listen through Radio's current audio output. Selects USB / 3.2 kHz with mark at 2125 Hz; CW relearns at the new dial. Only this button changes tuning; opening a tab does not.");
            ImGui::TextDisabled("RTTY text is held in these tabs; copy it before closing. CW Markdown logging remains separate.");
            if(view.rawCache!=s.text){view.rawCache=s.text;view.compactCache=rtty::compactRY(s.text);}
            view.transcript.draw(view.compact?view.compactCache:s.text,std::max(80.f,ImGui::GetContentRegionAvail().y-footer-24),textSize_,view.follow);
            if(view.transcript.popupOpen())controlsPopupOpen_=true;
            ImGui::EndTabItem();
        }
        if(!open)rtty_->close(s.id);
    }
}
