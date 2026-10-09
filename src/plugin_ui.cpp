// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <core.h>
#include <cstdio>

namespace {
const ImVec4 mint{.36f,.85f,.68f,1},amber{1,.73f,.34f,1},muted{.60f,.65f,.70f,1};
void hint(const char* text){if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextUnformatted(text);ImGui::EndTooltip();}}
}
void CWNotebook::sidebar(){
    auto s=engine_.snapshot();
    manualSpeed_=s.manualWpm>0;manualPitch_=s.manualTone>0;
    if(manualSpeed_)speed_=s.manualWpm;
    if(manualPitch_)pitch_=s.manualTone;
    ImGui::TextColored(mint,"CW NOTEBOOK");
    ImGui::TextWrapped("Morse to text, with a saved reception log.");
    if(ImGui::Button(window_?"Hide transcript":"Open transcript",ImVec2(-1,0)))window_=!window_;
    if(ImGui::BeginCombo("Receiver",streamName_.c_str())){
        for(const auto& n:sigpath::sinkManager.getStreamNames())if(ImGui::Selectable(n.c_str(),n==streamName_))select(n);
        ImGui::EndCombo();
    }
    if(!supportedMode_){
        ImGui::TextWrapped("Select CW on this receiver, then tune a Morse signal.");
        if(ImGui::Button("Use CW mode",ImVec2(-1,0))){
            auto it=core::moduleManager.instances.find(streamName_);
            if(it!=core::moduleManager.instances.end())it->second.instance->handleDebugCommand("set_demod","CW");
        }
    }else ImGui::Text("%.1f WPM  |  offset %+.0f Hz",s.decoder.wpm,s.decoder.tone-800);
    bool save=s.autosave;if(ImGui::Checkbox("Autosave Markdown",&save))engine_.command("autosave",save?"1":"0");
    if(!s.error.empty())ImGui::TextColored(amber,"Log needs attention: open transcript.");
}
void CWNotebook::logControls(const cw::Snapshot& s){
    if(ImGui::Button("Save now"))engine_.command("save");hint("Save pending text and sync the Markdown file to disk.");
    ImGui::SameLine();if(ImGui::Button("Open log"))openPath(s.path,false);
    ImGui::SameLine();if(ImGui::Button("Show in Finder"))openPath(s.path,true);
    ImGui::SameLine();if(ImGui::Button("Rename...")){
        auto name=std::filesystem::path(s.path).filename().string();std::snprintf(filename_,sizeof(filename_),"%s",name.c_str());renaming_=true;
    }
    ImGui::SameLine();if(ImGui::Button("New log"))engine_.command("new");
    ImGui::SameLine();if(ImGui::Button("Clear log")){engine_.command("clear_log");renaming_=false;}
    hint("Archive the current log, then completely clear the file and decoded text box. New decoding continues.");
    ImGui::SameLine();if(ImGui::Button("Copy to clipboard"))
        transcript_.copyReport(frequency_.load(),s.decoder.wpm,s.decoder.tone-800,s.manualWpm>0);
    hint("Copy local date/time, weekday, current frequency, speed, carrier offset and all text in the decoded box. A held view copies its displayed text.");
    if(renaming_){
        ImGui::TextUnformatted("The current file and future text use this name.");
        ImGui::SetNextItemWidth(430);bool enter=ImGui::InputText("##filename",filename_,sizeof(filename_),ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::TextDisabled(".md is added automatically. Existing files are never replaced.");
        if(ImGui::Button("Apply filename")||enter){engine_.command("rename",filename_);renaming_=false;}
        ImGui::SameLine();if(ImGui::Button("Cancel rename"))renaming_=false;
    }
}
void CWNotebook::draw(){
    if(++frame_%5==0)metadata();
    if(!enabled_ || !window_)return;
    auto s=engine_.snapshot();
    if(s.transcriptReset!=lastTranscriptReset_){
        transcript_.clear();follow_=true;lastTranscriptReset_=s.transcriptReset;
    }
    manualSpeed_=s.manualWpm>0;manualPitch_=s.manualTone>0;
    if(manualSpeed_)speed_=s.manualWpm;
    if(manualPitch_)pitch_=s.manualTone;
    if(s.blocks!=lastBlocks_){lastBlocks_=s.blocks;lastAudioTime_=ImGui::GetTime();}
    ImGui::SetNextWindowSize(ImVec2(870,620),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(370,150),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(720,420),ImVec2(1800,1400));
    ImGui::SetNextWindowBgAlpha(1.0f);
    if(!ImGui::Begin("CW Notebook###cw_notebook_window",&window_,ImGuiWindowFlags_NoCollapse)){ImGui::End();return;}
    windowMin_=ImGui::GetWindowPos();auto windowSize=ImGui::GetWindowSize();
    windowMax_=ImVec2(windowMin_.x+windowSize.x,windowMin_.y+windowSize.y);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(10,9));
    const char* state=!supportedMode_?"Select CW mode":!playing_?"Receiver stopped":s.paused?"Paused":ImGui::GetTime()-lastAudioTime_>2?"Waiting for audio":!s.decoder.signal?"Listening for a tone":!s.decoder.calibrated?"Learning Morse timing":"Decoding";
    ImGui::TextColored(s.decoder.calibrated?mint:amber,"%s",state);
    ImGui::SameLine();ImGui::TextDisabled("  %s  /  %.3f kHz",streamName_.c_str(),frequency_.load()/1000);
    ImGui::Separator();
    ImGui::Text("Speed  %.1f WPM",s.decoder.wpm);ImGui::SameLine();
    ImGui::Text("  Carrier offset  %+.0f Hz",s.decoder.tone-800);ImGui::SameLine();
    ImGui::TextColored(s.decoder.keyed?mint:muted,s.decoder.keyed?"  KEY DOWN":"  KEY UP");
    ImGui::SameLine();ImGui::TextDisabled("  %s",manualSpeed_?"Manual speed":"Automatic speed");
    if(ImGui::Button(s.paused?"Resume":"Pause"))engine_.command("pause",s.paused?"0":"1");
    ImGui::SameLine();if(ImGui::Button("Relearn"))engine_.command("reset");hint("Forget timing and tone estimates after changing stations or conditions.");
    ImGui::SameLine();if(ImGui::Button("Copy text"))transcript_.copyAll(s.text);
    ImGui::SameLine();if(ImGui::Button("Clear view"))engine_.command("clear_view");hint("Clear the screen only. Saved and pending log text is retained.");
    ImGui::SameLine();ImGui::Checkbox("Follow text",&follow_);
    ImGui::SameLine();ImGui::SetNextItemWidth(100);ImGui::SliderFloat("Text size",&textSize_,.9f,1.7f,"%.1fx");
    ImGui::TextWrapped("Changing station? Select CW, tune its carrier, then choose Auto / new station.");
    if(ImGui::Button("Auto / new station")){engine_.command("wpm","0");engine_.command("tone","0");engine_.command("reset");engine_.command("pause","0");}
    ImGui::SameLine();ImGui::TextDisabled("Clears speed/tone locks. Keeps your text and log.");
    if(ImGui::CollapsingHeader("Operating guide")){
        ImGui::TextWrapped("1. Start Brown's receiver. Select CW in Radio and tune one narrow, keyed signal; a steady carrier alone is not Morse.");
        ImGui::TextWrapped("2. Center the carrier on the tuned frequency (within 250 Hz). The decoder uses its own 500 Hz channel; changing Radio's audio filter does not change that channel.");
        ImGui::TextWrapped("3. On a new band or station, choose Auto / new station. Allow several characters for learning. Retuning resets timing automatically, but manual locks otherwise remain in effect.");
        ImGui::TextWrapped("4. If Auto jumps during fading, set an approximate WPM in Decoder settings. Relearn resets detection while keeping those manual settings.");
        ImGui::TextWrapped("5. Autosave adds frequency sections to the current file. Choose New log for a separate file; Clear view only clears the screen. Clear log clears the file and decoded text box, keeping a dated backup.");
        ImGui::TextWrapped("No text? Check receiver running, Resume, CW mode, carrier centered, antenna and signal strength. Speaker mute does not stop decoding.");
    }
    if(ImGui::CollapsingHeader("Decoder settings")){
        if(ImGui::Checkbox("Set speed manually",&manualSpeed_))engine_.command("wpm",manualSpeed_?std::to_string(speed_):"0");
        ImGui::SameLine();ImGui::SetNextItemWidth(150);
        if(ImGui::SliderFloat("WPM",&speed_,3,200,"%.1f")&&manualSpeed_)engine_.command("wpm",std::to_string(speed_));
        ImGui::TextDisabled("If fading makes Auto jump, lock an approximate speed and use Relearn.");
        if(ImGui::Checkbox("Lock carrier offset",&manualPitch_))engine_.command("tone",manualPitch_?std::to_string(pitch_):"0");
        ImGui::SameLine();ImGui::SetNextItemWidth(150);
        float offset=pitch_-800;
        if(ImGui::SliderFloat("Hz",&offset,-220,220,"%+.0f")&&manualPitch_){pitch_=offset+800;engine_.command("tone",std::to_string(pitch_));}
        ImGui::TextDisabled("Any tuned RF band. One CW signal at a time. [?] means uncertain Morse.");
        ImGui::TextDisabled("Dedicated 500 Hz receiver channel. Speaker volume and audio AGC do not affect decoding.");
    }
    float reserve=175+(s.error.empty()?0:45)+(s.notice.empty()?0:40)+(renaming_?120:0);
    if(s.text.empty())ImGui::TextColored(muted,"Waiting for decoded Morse. Tune one clear keyed signal in CW mode.");
    transcript_.draw(s.text,std::max(100.f,ImGui::GetContentRegionAvail().y-reserve),textSize_,follow_);
    bool save=s.autosave;if(ImGui::Checkbox("Autosave Markdown",&save))engine_.command("autosave",save?"1":"0");
    ImGui::SameLine();ImGui::TextDisabled("%llu characters saved this session",static_cast<unsigned long long>(s.saved));
    if(s.dropped){ImGui::SameLine();ImGui::TextColored(amber,"%llu audio gaps; timing reset",static_cast<unsigned long long>(s.dropped));}
    logControls(s);
    ImGui::TextColored(muted,"%s",s.path.empty()?"A log is created when the first text is decoded, or choose Save now.":s.path.c_str());
    if(!s.error.empty())ImGui::TextWrapped("Save error: %s. Text remains in memory; choose Save now to retry.",s.error.c_str());
    if(!s.notice.empty())ImGui::TextWrapped("%s",s.notice.c_str());
    ImGui::PopStyleVar();ImGui::End();
}
