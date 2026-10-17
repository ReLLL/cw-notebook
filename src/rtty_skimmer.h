// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_decoder.h"
#include "rtty_selection.h"
#include "rtty_focus.h"
#include <memory>
#include <cstdint>

namespace rtty {
struct Station {
    uint64_t id=0;
    double frequency=0,center=0,shift=0,baud=0;
    bool reverse=false,active=false;
    std::string text;
    double lastTextAt=0;
};
struct Status {
    struct Signal {double center=0,shift=0;bool available=false,decoding=false,confirmed=false;};
    std::vector<Station> stations;
    std::vector<Signal> signals;
    double center=0,rate=0,seconds=0,processingMs=0;
    size_t candidates=0,decoders=0,limited=0;
    Focus focus;
    size_t focusedDecoders=0;
    bool receiving=false;
    uint64_t blocks=0,dropped=0;
    std::string error;
};
class Skimmer {
public:
    static constexpr size_t maxDecoders=16,maxTabs=32,channelSize=128;
    static constexpr double maxRate=4096000;
    Skimmer();
    ~Skimmer();
    void process(const IQ* samples,size_t count,double rate,double center);
    void discontinuity();
    Status snapshot() const;
    void close(uint64_t id);
    void closeAll();
    void selection(Selection selection);
    void focus(Focus focus);
private:
    struct Channel;
    void configure(double rate,double center);
    void frame();
    void discover();
    void updateStations();
    double rate_=0,center_=0,binHz_=0,seconds_=0,lastDiscovery_=-1;
    uint64_t nextId_=1,blocks_=0;
    size_t size_=0,filled_=0,candidates_=0,limited_=0;
    std::unique_ptr<FFT> fft_;
    FFT channelFFT_{channelSize};
    std::vector<IQ> input_,spectrum_,band_;
    std::vector<float> window_,power_,noiseScratch_;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<Station> stations_;
    std::vector<Status::Signal> signals_;
    Selection selection_;
    Focus focus_;
    struct Cooldown {double frequency,shift,until;};
    std::vector<Cooldown> cooldowns_;
};
}
