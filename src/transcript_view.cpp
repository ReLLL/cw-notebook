// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "transcript_view.h"
#include <imgui_internal.h>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

void TranscriptView::clear(){
    source_.clear();selection_.clear();buffer_.assign(1,'\0');width_=0;
}
void TranscriptView::copied(const std::string& text,const char* kind){
    ImGui::SetClipboardText(text.c_str());
    copyNotice_=std::string("Copied ")+kind+" ("+std::to_string(text.size())+" characters).";
    copiedAt_=ImGui::GetTime();
}
void TranscriptView::copyAll(const std::string& text){copied(text,"all text");}
void TranscriptView::copyReport(double frequency,float wpm,float carrierOffset,bool manualSpeed){
    const auto now=std::time(nullptr);
    std::tm local{};
    localtime_r(&now,&local);
    char offset[16]{};
    std::strftime(offset,sizeof(offset),"%z",&local);
    std::string zone=offset;
    if(zone.size()==5)zone.insert(3,":");
    std::ostringstream report;
    report.imbue(std::locale::classic());
    report<<"CW reception\n"
          <<"Date/time:      "<<std::put_time(&local,"%Y-%m-%d %H:%M:%S")<<'\n'
          <<"Day:            "<<std::put_time(&local,"%A")<<'\n'
          <<"Time zone:      Local (UTC"<<zone<<")\n"
          <<"Frequency:      "<<std::fixed<<std::setprecision(3)<<frequency/1000<<" kHz\n"
          <<"Speed:          "<<std::setprecision(1)<<wpm<<" WPM ("<<(manualSpeed?"manual":"estimated")<<")\n"
          <<"Carrier offset: "<<std::showpos<<std::setprecision(0)<<carrierOffset<<std::noshowpos<<" Hz\n\n"
          <<"Decoded Morse\n-------------\n"<<source_;
    if(source_.empty() || source_.back()!='\n')report<<'\n';
    copied(report.str(),"reception details and displayed text");
}
void TranscriptView::update(const std::string& text,float width){
    if(source_==text && width_==width && fontSize_==ImGui::GetFontSize())return;
    source_=text;width_=width;fontSize_=ImGui::GetFontSize();selection_.clear();buffer_.clear();
    const char* p=text.data();const char* end=p+text.size();
    while(p<end){
        const char* paragraph=std::find(p,end,'\n');
        while(p<paragraph){
            const char* next=ImGui::GetFont()->CalcWordWrapPositionA(ImGui::GetFontSize()/ImGui::GetFont()->FontSize,p,paragraph,width);
            if(next<=p)next=p+1; // Decoder output is ASCII, including prosigns.
            buffer_.insert(buffer_.end(),p,next);p=next;
            if(p<paragraph){while(p<paragraph && *p==' ')++p;buffer_.push_back('\n');}
        }
        if(p<end){buffer_.push_back('\n');++p;}
    }
    buffer_.push_back('\0');
}
int TranscriptView::selectionCallback(ImGuiInputTextCallbackData* data){
    auto& self=*static_cast<TranscriptView*>(data->UserData);
    int first=std::min(data->SelectionStart,data->SelectionEnd);
    int last=std::max(data->SelectionStart,data->SelectionEnd);
    self.selection_.assign(data->Buf+first,data->Buf+last);
    if(!self.selection_.empty() && ImGui::GetIO().KeySuper && ImGui::IsKeyPressed(ImGuiKey_C))self.copied(self.selection_,"selection");
    return 0;
}
void TranscriptView::draw(const std::string& text,float height,float scale,bool& follow){
    ImGui::PushID(this);
    ImGui::SetWindowFontScale(scale);
    const auto id=ImGui::GetID("##decoded_morse");
    if(follow && !wasFollowing_ && ImGui::GetActiveID()==id)ImGui::ClearActiveID();
    if(follow || (source_.empty() && !text.empty()))
        update(text,std::max(80.f,ImGui::GetContentRegionAvail().x-2*ImGui::GetStyle().FramePadding.x-ImGui::GetStyle().ScrollbarSize-8));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(.055f,.067f,.078f,1));
    const bool macKeys=ImGui::GetIO().ConfigMacOSXBehaviors;
    ImGui::GetIO().ConfigMacOSXBehaviors=true;
    ImGui::InputTextMultiline("##decoded_morse",buffer_.data(),buffer_.size(),ImVec2(-1,height),
        ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_CallbackAlways,selectionCallback,this);
    ImGui::GetIO().ConfigMacOSXBehaviors=macKeys;
    const bool hovered=ImGui::IsItemHovered();
    if(hovered && (ImGui::IsMouseClicked(0)||ImGui::IsMouseClicked(1)||ImGui::GetIO().MouseWheel!=0))follow=false;
    popupOpen_=ImGui::BeginPopupContextItem("copy_morse");
    if(popupOpen_){
        if(ImGui::MenuItem("Copy selection","Cmd+C",false,!selection_.empty()))copied(selection_,"selection");
        if(ImGui::MenuItem("Copy all",nullptr,false,!text.empty()))copyAll(text);
        ImGui::Separator();
        if(ImGui::MenuItem("Follow live text"))follow=true;
        ImGui::EndPopup();
    }
    if(follow){
        // This is the host's existing multiline child, not a second text widget.
        // Internal headers are checked against the installed Brown ABI at build validation.
        for(auto* child:ImGui::GetCurrentWindow()->DC.ChildWindows)
            if(child->ChildId==id)ImGui::SetScrollY(child,child->ScrollMax.y);
    }
    ImGui::PopStyleColor();ImGui::SetWindowFontScale(1);
    if(ImGui::GetTime()-copiedAt_<3)ImGui::TextColored(ImVec4(.36f,.85f,.68f,1),"%s",copyNotice_.c_str());
    else ImGui::TextDisabled(follow?"Select text and press Cmd+C, or right-click for copy options.":"View held for selection. Decoding and autosave continue. Enable Follow text to catch up.");
    wasFollowing_=follow;
    ImGui::PopID();
}
