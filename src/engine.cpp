// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>

namespace cw {
Engine::Engine(const std::filesystem::path& folder):journal_(folder),folder_(folder){
    decoder_.output=[this](const std::string& s){decoded(s);};
    thread_=std::thread([this]{work();});
}
Engine::~Engine(){stop_=true;cv_.notify_one();if(thread_.joinable())thread_.join();}
void Engine::audio(std::vector<float> samples,int rate,double frequency){
    std::lock_guard<std::mutex> lock(queueMutex_);
    if(queuedSamples_+samples.size()>size_t(std::max(rate,4000))*2){dropped_++;generation_++;return;}
    queuedSamples_+=samples.size();
    queue_.push_back({std::move(samples),rate,frequency,generation_.load()});cv_.notify_one();
}
void Engine::command(std::string name,std::string value){
    std::lock_guard<std::mutex> lock(queueMutex_);
    if(commands_.size()<64)commands_.push_back({std::move(name),std::move(value)});
    cv_.notify_one();
}
Snapshot Engine::snapshot(){std::lock_guard<std::mutex> lock(viewMutex_);return view_;}
void Engine::decoded(const std::string& text){
    if(text==" "){
        std::lock_guard<std::mutex> lock(viewMutex_);
        if(transcriptBreakPending_ || view_.text.empty() || view_.text.back()==' ' || view_.text.back()=='\n')return;
    }
    std::string formatted=text;
    {
        std::lock_guard<std::mutex> lock(viewMutex_);
        if(transcriptBreakPending_){
            // A reset is not received text. Separate runs only once new text arrives.
            auto end=view_.text.find_last_not_of(" \n");
            if(end!=std::string::npos){view_.text.resize(end+1);view_.text+="\n\n";}
            transcriptBreakPending_=false;
        }
        auto end=view_.text.find_last_not_of(" \n");
        if(text=="<AR>" || text=="<SK>" || (text=="=" && end!=std::string::npos && view_.text[end]=='='))formatted+="\n\n";
    }
    if(pending_.empty() || pending_.back().frequency!=frequency_)pending_.push_back({"",frequency_,decoder_.stats().wpm});
    pending_.back().text+=formatted;unsavedBytes_+=formatted.size();
    // Retain unsaved data on I/O failure, with a visible bounded-memory limit.
    if(unsavedBytes_>2*1024*1024){paused_=true;std::lock_guard<std::mutex> l(viewMutex_);view_.error="Unsaved text reached 2 MB. Decoding paused; save to recover.";}
    std::lock_guard<std::mutex> lock(viewMutex_);
    view_.text+=formatted;
    if(view_.text.size()>100000)view_.text.erase(0,view_.text.size()-80000);
}
void Engine::flush(){
    if(pending_.empty())return;
    if(journal_.path().empty())journal_.create(frequency_);
    while(!pending_.empty()){
        const auto& segment=pending_.front();
        journal_.append(segment.text,segment.frequency,segment.wpm);
        saved_+=segment.text.size();unsavedBytes_-=segment.text.size();pending_.pop_front();
    }
    persist();
}
void Engine::persist(){
    if(journal_.path().empty())return;
    auto tmp=folder_/".last-log.tmp";
    {std::ofstream out(tmp);out.exceptions(std::ios::badbit|std::ios::failbit);out<<journal_.path().filename().string();}
    std::filesystem::rename(tmp,folder_/".last-log");
}
void Engine::preferences(){
    std::filesystem::create_directories(folder_);
    auto temp=folder_/".decoder-settings.tmp";
    {std::ofstream out(temp);out.exceptions(std::ios::badbit|std::ios::failbit);out<<autoSave_<<' '<<manualWpm_<<' '<<manualTone_<<'\n';}
    std::filesystem::rename(temp,folder_/".decoder-settings");
}
void Engine::apply(const Command& c){
    if(c.name=="capture"){
        if(diagnostic_.is_open())diagnostic_.close();
        diagnostic_.clear();std::filesystem::create_directories(folder_);
        std::filesystem::create_directories(folder_/"Diagnostics");
        auto path=folder_/"Diagnostics"/("diagnostic-"+utc("%Y%m%d-%H%M%SZ")+".f32");
        if(std::filesystem::exists(path))throw std::runtime_error("Diagnostic file already exists");
        diagnostic_.open(path,std::ios::binary);diagnostic_.exceptions(std::ios::badbit|std::ios::failbit);
        diagnosticRemaining_=8000*20;
        std::lock_guard<std::mutex> lock(viewMutex_);view_.notice="20-second diagnostic capture: "+path.string();
    }
    else if(c.name=="pause") {paused_=c.value=="1";decoder_.reset(manualWpm_,manualTone_);if(autoSave_)flush();}
    else if(c.name=="autosave") {autoSave_=c.value=="1";if(autoSave_)flush();}
    else if(c.name=="save") {flush();if(journal_.path().empty())journal_.create(frequency_);persist();}
    else if(c.name=="reset")decoder_.reset(manualWpm_,manualTone_);
    else if(c.name=="clear_view"){std::lock_guard<std::mutex> lock(viewMutex_);view_.text.clear();++view_.transcriptReset;}
    else if(c.name=="rename"){flush();if(journal_.path().empty())journal_.create(frequency_);journal_.rename(c.value);persist();}
    else if(c.name=="new"){flush();journal_.create(frequency_);persist();saved_=0;}
    else if(c.name=="clear_log"){
        flush();if(journal_.path().empty())journal_.create(frequency_);
        auto backup=journal_.clear();saved_=0;
        decoder_.reset(manualWpm_,manualTone_);transcriptBreakPending_=false;
        {std::lock_guard<std::mutex> lock(queueMutex_);queue_.clear();queuedSamples_=0;}
        discardCurrentPacket_=true;
        std::lock_guard<std::mutex> lock(viewMutex_);
        view_.text.clear();++view_.transcriptReset;
        view_.notice="Log and decoded text cleared. Previous log archived as "+backup.filename().string();
    }
    else if(c.name=="wpm" || c.name=="tone"){
        float v=std::stof(c.value);
        if(!std::isfinite(v) || (v!=0 && (c.name=="wpm"?(v<3||v>200):(v<200||v>1800))))throw std::runtime_error("Setting outside supported range");
        if(c.name=="wpm")manualWpm_=v;else manualTone_=v;
        decoder_.reset(manualWpm_,manualTone_);
    }
    if(c.name=="autosave"||c.name=="wpm"||c.name=="tone")preferences();
    {std::lock_guard<std::mutex> lock(viewMutex_);view_.error.clear();}
}
void Engine::work(){
    auto lastFlush=std::chrono::steady_clock::now();
    try{
        std::ifstream prefs(folder_/".decoder-settings");float wpm=0,tone=0;bool save=true;
        if(prefs>>save>>wpm>>tone && std::isfinite(wpm)&&std::isfinite(tone) &&
           (wpm==0||(wpm>=3&&wpm<=200)) && (tone==0||(tone>=200&&tone<=1800))){
            manualWpm_=wpm;manualTone_=tone;autoSave_=save;decoder_.reset(wpm,tone);
        }
        std::ifstream in(folder_/".last-log");std::string name;
        if(std::getline(in,name) && !name.empty())journal_.resume(name);
    }catch(const std::exception& e){std::lock_guard<std::mutex> l(viewMutex_);view_.error=e.what();}
    while(!stop_){
        Packet p{};std::deque<Command> commands;
        {std::unique_lock<std::mutex> lock(queueMutex_);cv_.wait_for(lock,std::chrono::milliseconds(100),[this]{return stop_||!queue_.empty()||!commands_.empty();});
            commands.swap(commands_);
            if(!queue_.empty()){p=std::move(queue_.front());queue_.pop_front();queuedSamples_-=p.samples.size();}
        }
        try{
            for(const auto& c:commands)apply(c);
            if(discardCurrentPacket_){p.samples.clear();discardCurrentPacket_=false;}
            if(!p.samples.empty()){
                if(diagnosticRemaining_){
                    auto n=std::min(diagnosticRemaining_,p.samples.size());
                    diagnostic_.write(reinterpret_cast<const char*>(p.samples.data()),n*sizeof(float));diagnosticRemaining_-=n;
                    if(!diagnosticRemaining_)diagnostic_.close();
                }
                if(p.rate!=rate_ || std::abs(p.frequency-frequency_)>5 || p.generation!=lastGeneration_){
                    if(frequency_!=0)transcriptBreakPending_=true;
                    decoder_.reset(manualWpm_,manualTone_);frequency_=p.frequency;rate_=p.rate;lastGeneration_=p.generation;
                }
                if(!paused_)decoder_.process(p.samples.data(),p.samples.size(),p.rate);
                blocks_++;
            }
            auto now=std::chrono::steady_clock::now();
            if(now-lastFlush>std::chrono::seconds(1)){
                if(autoSave_)flush();lastFlush=now;
            }
        }catch(const std::exception& e){std::lock_guard<std::mutex> lock(viewMutex_);view_.error=e.what();}
        {std::lock_guard<std::mutex> lock(viewMutex_);view_.decoder=decoder_.stats();view_.frequency=frequency_;view_.blocks=blocks_;view_.dropped=dropped_;
            view_.path=journal_.path().string();view_.saved=saved_;view_.paused=paused_;view_.autosave=autoSave_;
            view_.manualWpm=manualWpm_;view_.manualTone=manualTone_;}
    }
    try{flush();}catch(...){ /* Previous successfully synced rows remain recoverable. */ }
}
}
