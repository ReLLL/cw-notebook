// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "window_presentation.h"
#include <core.h>
#include <cstdio>

namespace {
const ImVec4 live{.36f,.85f,.68f,1},warning{1,.73f,.34f,1},muted{.60f,.65f,.70f,1};
void hint(const char* text){if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextUnformatted(text);ImGui::EndTooltip();}}
}
void CWNotebook::sidebar(){
    auto s=engine_.snapshot();
    if(ImGui::Button(window_?"Bring CW Notebook to front":"Open CW Notebook",ImVec2(-1,0))){window_=true;presentWindow_=true;}
    if(ImGui::BeginCombo("Receiver",streamName_.c_str())){
        for(const auto& n:sigpath::sinkManager.getStreamNames())if(ImGui::Selectable(n.c_str(),n==streamName_))select(n);
        ImGui::EndCombo();
    }
    if(!supportedMode_){
        if(ImGui::Button("Use CW mode",ImVec2(-1,0))){
            auto it=core::moduleManager.instances.find(streamName_);
            if(it!=core::moduleManager.instances.end())it->second.instance->handleDebugCommand("set_demod","CW");
        }
    }else if(s.decoder.calibrated)ImGui::Text("%.1f WPM | %+.0f Hz",s.decoder.wpm,s.decoder.tone-800);
    else ImGui::TextUnformatted("Learning dot / dash timing");
    if(!s.error.empty())ImGui::TextColored(warning,"Open notebook: attention needed");
}
void CWNotebook::logControls(const cw::Snapshot& s){
    if(ImGui::Button("Save now"))engine_.command("save");hint("Sync pending Markdown text to disk.");
    ImGui::SameLine();if(ImGui::Button("Clear log")){engine_.command("clear_log");renaming_=false;}
    hint("Archive the current log, then clear the file, transcript and held timing data.");
    ImGui::SameLine();if(ImGui::Button("Copy to clipboard")){
        const auto& d=showRecovery_?s.recoveredDecoder:s.decoder;
        const auto text=cw::transcriptText(s,showRecovery_);
        transcript_.copyReport(frequency_.load(),d.wpm,d.tone-800,s.manualWpm>0,showRecovery_,&text);
    }
    hint("Copy the displayed version with date, weekday, frequency, speed and carrier offset.");
    ImGui::SameLine();if(ImGui::Button("Log file..."))ImGui::OpenPopup("notebook_log_actions");
    controlsPopupOpen_=ImGui::IsPopupOpen("notebook_log_actions");
    if(ImGui::BeginPopup("notebook_log_actions")){
        if(ImGui::MenuItem("Open Markdown"))openPath(s.path,false);
        if(ImGui::MenuItem("Show in Finder"))openPath(s.path,true);
        if(ImGui::MenuItem("Rename...")){
            auto name=std::filesystem::path(s.path).filename().string();std::snprintf(filename_,sizeof(filename_),"%s",name.c_str());renaming_=true;
        }
        if(ImGui::MenuItem("New log"))engine_.command("new");
        ImGui::EndPopup();
    }
    if(renaming_){
        ImGui::SetNextItemWidth(std::max(120.f,ImGui::GetContentRegionAvail().x-210));
        bool enter=ImGui::InputText("##filename",filename_,sizeof(filename_),ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();if(ImGui::Button("Apply filename")||enter){engine_.command("rename",filename_);renaming_=false;}
        ImGui::SameLine();if(ImGui::Button("Cancel"))renaming_=false;
    }
    bool save=s.autosave;if(ImGui::Checkbox("Autosave Markdown",&save))engine_.command("autosave",save?"1":"0");
    ImGui::SameLine();ImGui::TextDisabled("%llu characters saved",static_cast<unsigned long long>(s.saved));
    auto name=std::filesystem::path(s.path).filename().string();
    ImGui::TextDisabled("%s",name.empty()?"A log is created when text arrives.":name.c_str());hint(s.path.c_str());
}
void CWNotebook::decoderControls(const cw::Snapshot& s){
    bool extended=s.extendedSpeed;
    if(ImGui::Checkbox("Extended automatic speed (3-200 WPM)",&extended))engine_.command("extended_speed",extended?"1":"0");
    ImGui::TextWrapped("Default: 5-45 WPM. Implausible estimates are rejected, not forced into letters.");
    if(ImGui::Checkbox("Manual speed",&manualSpeed_))engine_.command("wpm",manualSpeed_?std::to_string(speed_):"0");
    ImGui::SameLine();ImGui::SetNextItemWidth(180);
    if(ImGui::SliderFloat("WPM",&speed_,3,200,"%.1f")&&manualSpeed_)engine_.command("wpm",std::to_string(speed_));
    hint("Manual speed also decodes held key/gap intervals; it does not discard the buffer.");
    if(ImGui::Checkbox("Lock carrier offset",&manualPitch_))engine_.command("tone",manualPitch_?std::to_string(pitch_):"0");
    ImGui::SameLine();ImGui::SetNextItemWidth(180);float offset=pitch_-800;
    if(ImGui::SliderFloat("Hz",&offset,-220,220,"%+.0f")&&manualPitch_){pitch_=offset+800;engine_.command("tone",std::to_string(pitch_));}
    bool retune=s.autoRetune;
    if(ImGui::Checkbox("Release manual locks when tuning a new station",&retune))engine_.command("auto_retune",retune?"1":"0");
    if((manualSpeed_||manualPitch_)&&ImGui::Button("Return to automatic detection")){
        engine_.command("wpm","0");engine_.command("tone","0");
    }
    ImGui::Separator();
    bool recovery=s.timingRecovery;
    if(ImGui::Checkbox("Timing and fade repair in separate recovery version",&recovery))engine_.command("timing_recovery",recovery?"1":"0");
    ImGui::TextWrapped("Follows sudden fades, filters brief glitches and adapts to uneven gaps. Select Timing + fade recovery in Transcript to compare. Original text is kept; no word replacement or vocabulary guessing.");
    ImGui::TextWrapped("CW filter follows Radio; speaker volume and audio AGC do not affect decoding.");
    if(ImGui::CollapsingHeader("Diagnostics")){
        ImGui::Text("Glitches filtered: %llu | Quiet relearns: %llu | Reconnects: %llu",
            static_cast<unsigned long long>(s.recoveredDecoder.filteredGlitches),static_cast<unsigned long long>(s.recoveredDecoder.recoveries),static_cast<unsigned long long>(reconnects_));
        if(ImGui::Button("Reconnect decoder"))handleDebugCommand("reconnect","");
        ImGui::SameLine();if(ImGui::Button("Save 20s signal"))engine_.command("capture");
        hint("Save a private recording for offline diagnosis. Reception continues.");
    }
}
void CWNotebook::draw(){
    if(++frame_%5==0)metadata();
    if(!enabled_||!window_)return;
    auto s=engine_.snapshot();
    if(s.transcriptReset!=lastTranscriptReset_){transcript_.clear();follow_=true;lastTranscriptReset_=s.transcriptReset;}
    manualSpeed_=s.manualWpm>0;manualPitch_=s.manualTone>0;
    if(manualSpeed_)speed_=s.manualWpm;
    if(manualPitch_)pitch_=s.manualTone;
    if(s.blocks!=lastBlocks_){lastBlocks_=s.blocks;lastAudioTime_=ImGui::GetTime();}
    prepareNotebookWindow(presentWindow_);ImGui::SetNextWindowBgAlpha(1);
    if(!ImGui::Begin("CW Notebook###cw_notebook_window",&window_,ImGuiWindowFlags_NoCollapse)){ImGui::End();return;}
    windowMin_=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();windowMax_={windowMin_.x+size.x,windowMin_.y+size.y};
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{8,6});
    const auto& d=showRecovery_?s.recoveredDecoder:s.decoder;
    const char* state=!supportedMode_?"Select CW mode":!playing_?"Receiver stopped":s.paused?"Paused":ImGui::GetTime()-lastAudioTime_>2?"Waiting for audio":!d.signal?"Listening":!d.calibrated?"Learning timing":"Decoding";
    ImGui::TextColored(d.calibrated?live:warning,"%s",state);
    ImGui::SameLine();ImGui::Text("%.3f kHz",frequency_.load()/1000);
    ImGui::SameLine();ImGui::TextColored(d.keyed?live:muted,d.keyed?"KEY DOWN":"KEY UP");
    ImGui::SameLine();if(ImGui::Button(s.paused?"Resume":"Pause"))engine_.command("pause",s.paused?"0":"1");
    ImGui::SameLine();if(ImGui::Button("Relearn"))engine_.command("reset");
    hint("Restart detection. Keeps confirmed text and log, but discards held intervals. Retuning already does this automatically.");
    if(d.calibrated)ImGui::Text("%.1f WPM",d.wpm);else ImGui::TextUnformatted("WPM --");
    ImGui::SameLine();ImGui::TextDisabled("%s | Offset %+.0f Hz | Filter %.0f Hz%s",manualSpeed_?"Manual":s.extendedSpeed?"Auto 3-200":"Auto 5-45",d.tone-800,bandwidth_,followsBandwidth_?" (Radio)":" (centered CW)");
    if(!d.calibrated&&d.bufferedRuns)
        ImGui::TextColored(warning,"%llu intervals held | provisional text updates while learning",static_cast<unsigned long long>(d.bufferedRuns));
    ImGui::Separator();
    const float footer=ImGui::GetFrameHeightWithSpacing()*3+ImGui::GetTextLineHeightWithSpacing()+12+(renaming_?ImGui::GetFrameHeightWithSpacing():0);
    if(ImGui::BeginTabBar("notebook_pages")){
        if(ImGui::BeginTabItem("Transcript")){
            ImGui::SetNextItemWidth(300);int version=showRecovery_?1:0;
            if(ImGui::Combo("##transcript_version",&version,"Original decode\0Timing + fade recovery (unverified)\0")){showRecovery_=version==1;transcript_.clear();follow_=true;}
            hint("Both versions are retained. Switching never rewrites the original.");
            ImGui::SameLine();ImGui::Checkbox("Follow",&follow_);hint("Scroll to new text. Selection holds scrolling; decoding continues.");
            ImGui::SameLine();ImGui::SetNextItemWidth(90);ImGui::SliderFloat("Text size",&textSize_,.9f,1.7f,"%.1fx");
            ImGui::SameLine();if(ImGui::Button("Clear view"))engine_.command("clear_view");hint("Clear displayed text only. The log is retained.");
            const auto text=cw::transcriptText(s,showRecovery_);
            if(text.empty())ImGui::TextDisabled("Tune one keyed signal in CW mode. Text appears as timing becomes clear.");
            transcript_.draw(text,std::max(80.f,ImGui::GetContentRegionAvail().y-footer-24),textSize_,follow_);
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Decoder")){
            ImGui::BeginChild("decoder_options",{0,std::max(80.f,ImGui::GetContentRegionAvail().y-footer)},false);
            decoderControls(s);ImGui::EndChild();ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("How to use")){
            ImGui::BeginChild("operating_guide",{0,std::max(80.f,ImGui::GetContentRegionAvail().y-footer)},false);
            ImGui::TextWrapped("1. Start the receiver, select CW and place one keyed carrier inside Radio's filter. Narrow the filter to exclude nearby stations; CW Notebook follows its width.");
            ImGui::Spacing();ImGui::TextWrapped("2. Measured key/gap durations are buffered. After two seconds of detected keying, provisional letters appear and can change as timing improves. Mixed dot/dash evidence confirms and replaces that draft. Only confirmed text is autosaved; copying includes the labeled draft. Manual WPM can decode held data.");
            ImGui::Spacing();ImGui::TextWrapped("3. Changing frequency starts fresh timing and a new text line. Relearn manually restarts detection. Both discard uncommitted intervals, so use them for a new signal, not merely to refresh the display.");
            ImGui::Spacing();ImGui::TextWrapped("4. Select text and press Cmd+C, or right-click for copy options. Copy to clipboard adds reception details. Clear view affects the display; Clear log archives the old file and clears both.");
            ImGui::Spacing();ImGui::TextWrapped("No letters? Check Resume, receiver running and a keyed carrier inside the filter. Auto defaults to 5-45 WPM. For unusually slow or fast Morse, set manual WPM or enable the extended range in Decoder.");
            ImGui::Spacing();ImGui::TextWrapped("USB/LSB use a centered 500 Hz CW channel. Use CW mode to follow Radio's bandwidth. This decodes one signal at a time. Timing + fade recovery is an unverified alternative; confirmed original text stays unchanged.");
            ImGui::EndChild();ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::Separator();logControls(s);
    if(!s.error.empty())ImGui::TextWrapped("Attention: %s",s.error.c_str());
    else if(s.dropped)ImGui::TextColored(warning,"%llu audio gaps; detection restarted",static_cast<unsigned long long>(s.dropped));
    else if(!s.notice.empty()){ImGui::TextDisabled("Last action: %s",s.notice.c_str());hint(s.notice.c_str());}
    ImGui::PopStyleVar();ImGui::End();
}
