// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "transcript_view.h"
#include "window_presentation.h"
#include "notebook_style.h"
#include "rtty_skimmer.h"
#include <imgui_internal.h>
#include <iostream>
#include <stdexcept>

// Brown's widget registry is unrelated to transcript rendering.
namespace ImGui {void sdrcppRegisterWidget(ImGuiID,int,const ImRect&) {}}

std::string clipboard;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    ImGui::CreateContext();auto& io=ImGui::GetIO();
    const ImGuiStyle hostStyle=ImGui::GetStyle();
    const int hostColors=GImGui->ColorStack.Size,hostVars=GImGui->StyleVarStack.Size;
    {notebook::Style notebookStyle;require(ImGui::GetStyle().Colors[ImGuiCol_CheckMark].x==notebook::brass.x,"Notebook theme missing");}
    require(GImGui->ColorStack.Size==hostColors&&GImGui->StyleVarStack.Size==hostVars,"Notebook theme leaked style stack entries into host");
    for(int i=0;i<ImGuiCol_COUNT;++i){
        const auto a=hostStyle.Colors[i],b=ImGui::GetStyle().Colors[i];
        require(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w,"Notebook changed the host's theme");
    }
    require(ImGui::GetStyle().WindowPadding.x==hostStyle.WindowPadding.x&&ImGui::GetStyle().FramePadding.y==hostStyle.FramePadding.y,"Notebook changed host geometry");
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
    const std::string hiddenUpdate="NEW STATION WHILE SETTINGS OPEN";
    view.copyReport(7020000,12,-20,false,false,&hiddenUpdate);
    require(clipboard.find("WHILE SETTINGS OPEN")!=std::string::npos,"Copy report omitted text received on another tab");
    rtty::Station rttyStation;rttyStation.frequency=10100580;rttyStation.center=10100805;
    rttyStation.baud=50;rttyStation.shift=450;rttyStation.reverse=true;rttyStation.text="RYRYRYRYRYRY\nCQ DE TEST";
    rtty::Status rttyCapture;rttyCapture.rate=3200000;rttyCapture.center=10114000;
    view.copyRTTYReport(rttyStation,rttyCapture,true);
    require(clipboard.find("RTTY reception")!=std::string::npos&&clipboard.find("10100.580 kHz")!=std::string::npos&&clipboard.find("30 m")!=std::string::npos,"RTTY clipboard omitted metadata");
    require(clipboard.find("[RY test pattern]")!=std::string::npos&&clipboard.find("CQ DE TEST")!=std::string::npos,"RTTY clipboard did not copy the displayed view");
    view.copyRTTYReport(rttyStation,rttyCapture,false);
    require(clipboard.find(rttyStation.text)!=std::string::npos,"Original RTTY clipboard lost received characters");
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
    // Standard SDR++ lacks Brown's draw event. Draw during EndFramePre with
    // no sidebar/main window, then remove the hook before module destruction.
    struct HookView {TranscriptView view;std::string text="CQ DE HOOK";bool follow=true;int frames=0;} hooked;
    ImGuiContextHook hook;hook.Type=ImGuiContextHookType_EndFramePre;hook.UserData=&hooked;
    hook.Callback=[](ImGuiContext*,ImGuiContextHook* h){
        auto& v=*static_cast<HookView*>(h->UserData);++v.frames;
        ImGui::Begin("Hook transcript");v.view.draw(v.text,200,1,v.follow);ImGui::End();
    };
    auto hookID=ImGui::AddContextHook(ImGui::GetCurrentContext(),&hook);
    ImGui::NewFrame();ImGui::Render();hooked.text+=" NEXT";
    ImGui::NewFrame();ImGui::Render();
    require(hooked.frames==2,"SDR++ frame hook did not draw without a sidebar");
    hooked.view.copyReport(7020000,20,0,false);
    require(clipboard.find("HOOK NEXT")!=std::string::npos,"Frame-hook transcript failed to refresh");
    ImGui::RemoveContextHook(ImGui::GetCurrentContext(),hookID);
    ImGui::NewFrame();ImGui::Render();
    require(hooked.frames==2,"Removed frame hook still invoked destroyed-module callback");
    auto popupFrame=[&](const char* owner,const char* title){
        ImGui::NewFrame();ImGui::Begin(owner);ImGui::OpenPopup(title);
        if(ImGui::BeginPopupModal(title)){ImGui::TextUnformatted("Modal test");ImGui::EndPopup();}
        ImGui::End();ImGui::Render();
    };
    popupFrame("CW Notebook###cw_notebook_window","CW Settings");
    require(GImGui->OpenPopupStack.Size==1,"Notebook modal fixture did not open");
    notebook::closePopups();require(GImGui->OpenPopupStack.empty(),"Disabled notebook left a modal blocking host input");
    popupFrame("Other plugin","How to use");notebook::closePopups();
    require(GImGui->OpenPopupStack.Size==1,"Notebook closed another plugin's modal");
    ImGui::ClosePopupToLevel(0,true);
    // Status changes draw inside a fixed footer, with no exterior margin and
    // no added rows before the tab strip. Test both selected and inactive dots.
    ImVec2 fixedTab{};bool haveTab=false;
    for(int pass=0;pass<4;++pass){
        ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("RTTY status layout",nullptr,ImGuiWindowFlags_NoSavedSettings);
        bool cwEnabled=true,rttyEnabled=true;
        ImGui::Checkbox("CW",&cwEnabled);ImGui::SameLine();
        ImGui::Checkbox("RTTY",&rttyEnabled);
        if(ImGui::BeginTabBar("activity tabs")){
            for(int tab=0;tab<2;++tab){
                bool selected=ImGui::BeginTabItem(tab?"    10100.580 kHz RTTY":"CW");
                if(tab){
                    auto* draw=ImGui::GetWindowDrawList();int start=draw->VtxBuffer.Size;
                    notebook::tabActivity(pass%2==0);
                    if(pass>0)require(draw->VtxBuffer.Size>start,"Inactive RTTY tab did not draw its filled activity circle");
                    const auto wanted=ImGui::GetColorU32(pass%2==0?ImVec4{.28f,.86f,.39f,1}:ImVec4{.98f,.78f,.20f,1});
                    bool color=false;for(int v=start;v<draw->VtxBuffer.Size;++v)if(draw->VtxBuffer[v].col==wanted)color=true;
                    if(pass>0)require(color,"RTTY circle used the wrong activity color");
                    auto point=ImGui::GetItemRectMin();
                    if(pass>0){
                        if(haveTab)require(point.x==fixedTab.x&&point.y==fixedTab.y,"Status changes shifted frequency tabs");
                        fixedTab=point;haveTab=true;
                    }
                }
                if(selected){ImGui::TextUnformatted("Received text");ImGui::EndTabItem();}
            }
            ImGui::EndTabBar();
        }
        auto* draw=ImGui::GetWindowDrawList();int start=draw->VtxBuffer.Size;
        notebook::statusBar(pass%2?"RTTY monitoring | 16/16 decoders | 8 waiting | 123 gaps":"RTTY monitoring", "Detailed status",pass%2);
        require(draw->VtxBuffer[start].pos.x==10&&draw->VtxBuffer[start].pos.y==610-notebook::statusHeight(),"Status bar has an exterior margin");
        require(draw->VtxBuffer[start+2].pos.x==810&&draw->VtxBuffer[start+2].pos.y==610,"Status bar does not reach window edges");
        ImGui::End();ImGui::Render();
    }
    ImGui::DestroyContext();std::cout<<"Transcript refresh and window presentation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
