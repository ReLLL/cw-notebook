// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "engine.h"
#include "transcript_view.h"
#include "stream_watchdog.h"
#include <module.h>
#include <gui/gui.h>
#include <dsp/sink/handler_sink.h>
#include <signal_path/signal_path.h>
#include <atomic>

class CWNotebook final:public ModuleManager::Instance {
public:
    explicit CWNotebook(std::string name);
    ~CWNotebook() override;
    void postInit() override;
    void enable() override;
    void disable() override;
    bool isEnabled() override { return enabled_; }
    std::string handleDebugCommand(const std::string& cmd,const std::string& args) override;
    void sidebar();
    void draw();
private:
    void select(const std::string& stream);
    void detach();
    void metadata();
    static void audio(dsp::complex_t* samples,int count,void* ctx);
    void openPath(const std::string& path,bool reveal);
    void logControls(const cw::Snapshot& s);
    std::string name_,streamName_="Radio";
    cw::Engine engine_;
    TranscriptView transcript_;
    cw::StreamWatchdog watchdog_;
    std::atomic<uint64_t> inputBlocks_{0};
    uint64_t reconnects_=0;
    bool showRecovery_=false;
    double lastFrequency_=0,lastCenter_=0,lastRate_=0;
    dsp::sink::Handler<dsp::complex_t> sink_;
    dsp::channel::RxVFO* audioStream_=nullptr;
    EventHandler<std::string> registered_,unregistering_;
    EventHandler<ImGuiContext*> drawHandler_;
    EventHandler<ImGui::WaterFall::InputHandlerArgs> inputHandler_;
    ImVec2 windowMin_{0,0},windowMax_{0,0};
    std::atomic<int> sampleRate_{8000};
    std::atomic<double> frequency_{0};
    std::atomic<bool> accept_{false};
    double mixerPhase_=0,channelOffset_=0;
    bool enabled_=true,window_=true,follow_=true,manualSpeed_=false,manualPitch_=false;
    bool presentWindow_=true;
    bool supportedMode_=false,playing_=false;
    bool renaming_=false;
    bool ownsMouseGesture_=false;
    float speed_=20,pitch_=800,textSize_=1.15;
    int frame_=0;
    uint64_t lastBlocks_=0, lastTranscriptReset_=0;
    double lastAudioTime_=0;
    char filename_[256]{};
};
