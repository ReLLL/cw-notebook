// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <complex>
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace cw {
struct DecoderStats {
    float tone = 800, wpm = 20, snr = 0, level = 0, threshold = 0;
    bool keyed = false, calibrated = false, signal = false;
};
class Timing {
public:
    std::function<void(const std::string&)> output;
    void reset(float manualWpm = 0);
    void tick(bool key, double seconds = .001);
    float wpm() const { return float(1.2 / unit_); }
    bool calibrated() const { return ready_; }
private:
    struct Run { bool key; double seconds; };
    void estimate();
    void consume(const Run& run);
    void letter();
    bool key_ = false, started_ = false, ready_ = false, letterSent_ = false, wordSent_ = false;
    double duration_ = 0, unit_ = .06, cutoff_ = .12, manual_ = 0;
    std::deque<double> marks_, gaps_;
    std::vector<Run> pending_;
    std::string symbol_;
};
class Decoder {
public:
    std::function<void(const std::string&)> output;
    std::function<void(bool,float,float)> trace;
    Decoder();
    void reset(float manualWpm = 0, float manualTone = 0);
    void process(const float* mono, size_t count, int sampleRate);
    DecoderStats stats() const { return stats_; }
private:
    void sample(float x);
    void findTone();
    void envelope(float x);
    Timing timing_;
    DecoderStats stats_;
    std::array<float, 1024> audio_{};
    std::array<float, 4096> levels_{};
    size_t audioPos_ = 0, levelPos_ = 0, samples_ = 0, ticks_ = 0;
    float manualTone_ = 0, noise_ = 0, peak_ = 0, low1_ = 0, low2_ = 0;
    float iqI_ = 0, iqQ_ = 0, i2_ = 0, q2_ = 0, sum_ = 0;
    double phase_ = 0, resample_ = 0;
    bool key_ = false, toneLocked_ = false;
    int debounce_ = 0, rate_ = 0;
    float candidateTone_ = 0;
    int stableTones_ = 0;
    size_t lastToneTick_ = 0;
};
std::string morse(const std::string& symbol);
}
