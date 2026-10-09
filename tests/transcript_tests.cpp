// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "transcript_view.h"
#include "window_presentation.h"
#include <imgui_internal.h>
#include <iostream>
#include <stdexcept>

// Brown's widget registry is unrelated to transcript rendering.
namespace ImGui {void sdrcppRegisterWidget(ImGuiID,int,const ImRect&) {}}

std::string clipboard;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    ImGui::CreateContext();auto& io=ImGui::GetIO();
    io.DisplaySize={1000,800};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
    io.SetClipboardTextFn=[](void*,const char* s){clipboard=s;};
    io.GetClipboardTextFn=[](void*){return clipboard.c_str();};
    unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    TranscriptView view;bool follow=true;ImVec2 textPoint;ImGuiID inputID=0;
    auto frame=[&](const std::string& text){
        ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Transcript test",nullptr,ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushID(&view);inputID=ImGui::GetID("##decoded_morse");ImGui::PopID();
        view.draw(text,300,1,follow);
        for(auto* child:ImGui::GetCurrentWindow()->DC.ChildWindows)
            if(child->ChildId==inputID)textPoint={child->InnerRect.Min.x+15,child->InnerRect.Min.y+10};
        ImGui::End();ImGui::Render();
    };
    auto shown=[&](const char* token){view.copyReport(7020000,20,0,false);return clipboard.find(token)!=std::string::npos;};
    frame("CQ CQ");frame("CQ CQ");
    io.MousePos=textPoint;io.MouseDown[0]=true;frame("CQ CQ");
    io.MouseDown[0]=false;frame("CQ CQ DE TEST");frame("CQ CQ DE TEST");
    require(shown("DE TEST"),"Clicking transcript froze incoming text");
    require(follow,"A plain click should not disable auto-scroll");
    auto* state=ImGui::GetInputTextState(inputID);require(state!=nullptr,"Text widget was not active");
    state->Stb.select_start=0;state->Stb.select_end=2;
    frame("CQ CQ DE TEST NEXT");
    require(state->HasSelection()&&state->Stb.select_end==2,"Appending text lost selection");
    require(state->CurLenA==18,"Active ImGui buffer did not refresh");
    require(!follow,"Selection should hold scroll position");
    follow=true;frame("CQ CQ DE TEST NEXT");frame("CQ CQ DE TEST NEXT");
    require(follow,"Resuming auto-scroll immediately held the view again");
    follow=false;frame("CQ CQ DE TEST NEXT");
    require(shown("NEXT"),"Disabling follow froze content instead of only scrolling");
    io.MouseWheel=1;frame("CQ CQ DE TEST NEXT MORE");io.MouseWheel=0;
    require(shown("MORE"),"Scrolling froze incoming text");
    view.clear();follow=true;frame("NEW STATION");
    require(shown("NEW STATION")&&!shown("DE TEST"),"Clearing retained old transcript");
    // The same presentation helper used on plugin enable and sidebar reopening
    // must recover a hidden/off-screen window after the host display shrinks.
    bool present=false;
    auto windowFrame=[&](bool request){
        ImGui::NewFrame();
        if(request){present=true;prepareNotebookWindow(present);}
        else {ImGui::SetNextWindowPos({1700,1200});ImGui::SetNextWindowSize({870,620});}
        ImGui::Begin("Notebook placement");ImGui::TextUnformatted("Transcript");ImGui::End();
        ImGui::Begin("Other panel");ImGui::End();
        ImGui::Render();
    };
    windowFrame(false);io.DisplaySize={640,480};windowFrame(true);
    auto* notebook=ImGui::FindWindowByName("Notebook placement");
    require(!present,"Presentation request was not consumed");
    require(notebook->Pos.x>=0&&notebook->Pos.y>=0&&
        notebook->Pos.x+notebook->Size.x<=640&&notebook->Pos.y+notebook->Size.y<=480,
        "Reopened window remained outside visible area");
    require(GImGui->NavWindow==notebook,"Reopened window was not focused");
    require(GImGui->Windows.back()==notebook,"Reopened window was not brought to front");
    ImGui::DestroyContext();std::cout<<"Transcript refresh and window presentation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
