// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <core.h>
#include <cstdlib>
#include <fstream>
#include <imgui_internal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
SDRPP_MOD_INFO{"cw_notebook","CW decoder with recoverable Markdown logs","ReLLL",CW_VERSION_MAJOR,CW_VERSION_MINOR,CW_VERSION_PATCH,1};
namespace {
std::filesystem::path logFolder(){const char* home=std::getenv("HOME");return std::filesystem::path(home?home:".")/"Documents/SDR++Brown/CW Logs";}
}
CWNotebook::CWNotebook(std::string name):name_(std::move(name)),engine_(logFolder()){
    gui::menu.registerEntry(name_,[](void* ctx){static_cast<CWNotebook*>(ctx)->sidebar();},this,this);
    drawHandler_={[](ImGuiContext*,void* ctx){static_cast<CWNotebook*>(ctx)->draw();},this};
    gui::mainWindow.onWaterfallDrawn.bindHandler(&drawHandler_);
    inputHandler_={[](ImGui::WaterFall::InputHandlerArgs,void* ctx){
        auto* self=static_cast<CWNotebook*>(ctx);
        auto p=ImGui::GetMousePos();
        if(!self->enabled_ || !self->window_){self->ownsMouseGesture_=false;return;}
        const bool inside=p.x>=self->windowMin_.x && p.x<=self->windowMax_.x && p.y>=self->windowMin_.y && p.y<=self->windowMax_.y;
        const bool clicked=ImGui::IsMouseClicked(0)||ImGui::IsMouseClicked(1)||ImGui::IsMouseClicked(2);
        const bool down=ImGui::IsMouseDown(0)||ImGui::IsMouseDown(1)||ImGui::IsMouseDown(2);
        auto* window=ImGui::FindWindowByName("CW Notebook###cw_notebook_window");
        auto* active=GImGui->ActiveIdWindow;
        const bool focusedControl=active && window && active->RootWindow==window && ImGui::GetActiveID()!=0;
        if(clicked && (inside || self->transcript_.popupOpen()))self->ownsMouseGesture_=true;
        // Keep the complete gesture, even after leaving the window. A popup's
        // dismissal click and text-selection keys must not reach the waterfall.
        if(inside || self->ownsMouseGesture_ || self->transcript_.popupOpen() || (focusedControl && !clicked)){
            gui::waterfall.inputHandled=true;
            gui::mainWindow.lockWaterfallControls=true;
        }
        if(!down)self->ownsMouseGesture_=false;
    },this};
    gui::waterfall.onInputProcess.bindHandler(&inputHandler_);
    registered_={[](std::string name,void* ctx){auto* self=static_cast<CWNotebook*>(ctx);if(self->enabled_ && !self->audioStream_ && name==self->streamName_)self->select(name);},this};
    unregistering_={[](std::string name,void* ctx){auto* self=static_cast<CWNotebook*>(ctx);if(name==self->streamName_)self->detach();},this};
    sigpath::sinkManager.onStreamRegistered.bindHandler(&registered_);
    sigpath::sinkManager.onStreamUnregister.bindHandler(&unregistering_);
    sink_.init(nullptr,audio,this);
}
CWNotebook::~CWNotebook(){
    accept_=false;
    gui::mainWindow.onWaterfallDrawn.unbindHandler(&drawHandler_);
    gui::waterfall.onInputProcess.unbindHandler(&inputHandler_);
    sigpath::sinkManager.onStreamRegistered.unbindHandler(&registered_);
    sigpath::sinkManager.onStreamUnregister.unbindHandler(&unregistering_);
    detach();gui::menu.removeEntry(name_);
}
void CWNotebook::postInit(){if(enabled_){select(streamName_);metadata();}}
void CWNotebook::enable(){enabled_=true;select(streamName_);metadata();}
void CWNotebook::disable(){enabled_=false;accept_=false;detach();engine_.command("save");}
void CWNotebook::detach(){
    if(!audioStream_)return;
    sink_.stop();sigpath::iqFrontEnd.removeVFO("cw_notebook_channel");audioStream_=nullptr;
    engine_.command("reset");
}
void CWNotebook::select(const std::string& name){
    detach();streamName_=name;
    auto names=sigpath::sinkManager.getStreamNames();
    if(std::find(names.begin(),names.end(),name)==names.end())return;
    channelOffset_=sigpath::vfoManager.getOffset(name);
    audioStream_=sigpath::iqFrontEnd.addVFO("cw_notebook_channel",8000,500,channelOffset_);
    if(audioStream_){sink_.setInput(&audioStream_->out);sink_.start();}
}
void CWNotebook::metadata(){
    playing_=gui::mainWindow.isPlaying();
    double frequency=gui::waterfall.getCenterFrequency();
    if(sigpath::vfoManager.vfoExists(streamName_)){
        double offset=sigpath::vfoManager.getOffset(streamName_);frequency+=offset;
        if(audioStream_ && std::abs(offset-channelOffset_)>.1){audioStream_->setOffset(offset);channelOffset_=offset;}
    }
    else frequency=double(gui::freqSelect.frequency);
    frequency_=frequency;
    auto it=core::moduleManager.instances.find(streamName_);
    supportedMode_=false;
    if(it!=core::moduleManager.instances.end() && it->second.instance){
        try {auto j=json::parse(it->second.instance->handleDebugCommand("get_demod",""));
            auto mode=j.value("demod","");supportedMode_=mode=="CW"||mode=="USB"||mode=="LSB";
        }catch(const json::exception&){}
    }
    bool ready=enabled_ && supportedMode_ && playing_;
    if(ready!=accept_)engine_.command("reset");
    accept_=ready;
}
void CWNotebook::audio(dsp::complex_t* samples,int count,void* ctx){
    auto* self=static_cast<CWNotebook*>(ctx);
    if(!self->accept_ || count<=0 || count>1000000)return;
    std::vector<float> mono(size_t(count),0.f);
    for(int i=0;i<count;i++){
        mono[i]=samples[i].re*std::cos(self->mixerPhase_)-samples[i].im*std::sin(self->mixerPhase_);
        self->mixerPhase_+=2*3.141592653589793*800/8000;
        if(self->mixerPhase_>=2*3.141592653589793)self->mixerPhase_-=2*3.141592653589793;
    }
    self->engine_.audio(std::move(mono),self->sampleRate_,self->frequency_);
}
void CWNotebook::openPath(const std::string& path,bool reveal){
    if(path.empty())return;
    pid_t pid;std::vector<char*> args{const_cast<char*>("/usr/bin/open")};
    if(reveal)args.push_back(const_cast<char*>("-R"));
    args.push_back(const_cast<char*>(path.c_str()));args.push_back(nullptr);
    if(posix_spawn(&pid,"/usr/bin/open",nullptr,nullptr,args.data(),environ)==0){
        // open exits quickly; reap without blocking the UI.
        std::thread([pid]{int status;while(waitpid(pid,&status,0)<0 && errno==EINTR){}}).detach();
    }
}
std::string CWNotebook::handleDebugCommand(const std::string& cmd,const std::string& args){
    if(cmd=="status"){
        auto s=engine_.snapshot();return json{{"text",s.text},{"path",s.path},{"error",s.error},{"notice",s.notice},{"frequency",s.frequency},
            {"wpm",s.decoder.wpm},{"tone",s.decoder.tone},{"calibrated",s.decoder.calibrated},{"signal",s.decoder.signal},
            {"blocks",s.blocks},{"dropped",s.dropped},{"saved",s.saved},{"paused",s.paused},{"autosave",s.autosave},
            {"level",s.decoder.level},{"threshold",s.decoder.threshold},{"contrast_db",s.decoder.snr},{"input","IQ 8k / 500Hz"},{"version",CW_VERSION_STRING},
            {"manual_wpm",s.manualWpm},{"manual_tone",s.manualTone}}.dump();
    }
    if(cmd=="capture"||cmd=="save"||cmd=="new"||cmd=="rename"||cmd=="clear_log"||cmd=="clear_view"||cmd=="pause"||cmd=="autosave"||cmd=="reset"||cmd=="wpm"||cmd=="tone"){
        engine_.command(cmd,args);return "{\"status\":\"queued\"}";
    }
    return "{\"error\":\"Unknown command\"}";
}
MOD_EXPORT void _INIT_(){}
MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name){return new CWNotebook(std::move(name));}
MOD_EXPORT void _DELETE_INSTANCE_(ModuleManager::Instance* instance){delete instance;}
MOD_EXPORT void _END_(){}
