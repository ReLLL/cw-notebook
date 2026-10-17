// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_fft.h"
#include <array>
#include <string>

namespace rtty {
struct DecodeResult {
    std::string text;
    double baud=0;
    bool reverse=false,locked=false;
    unsigned frames=0,errors=0;
};
class Decoder {
public:
    Decoder(double rate,double center,double shift);
    void process(const IQ* samples,size_t count);
    DecodeResult result() const;
private:
    struct Framer {
        bool previous=true,receiving=false,figures=false;
        double idle=0,age=0,next=0;
        int bit=0,code=0;
        unsigned frames=0,errors=0,printable=0;
        std::string text;
        void sample(bool mark,bool reliable,double period);
        bool acquisitionEvidence() const;
    };
    struct Timing {
        double baud=0,period=0;
        std::vector<IQ> high,low;
        std::vector<float> power;
        IQ highSum{},lowSum{};
        float powerSum=0;
        size_t index=0;
        std::array<Framer,2> framers;
    };
    IQ highPhase_{1,0},lowPhase_{1,0},highStep_,lowStep_;
    std::array<Timing,4> timings_;
    size_t samples_=0;
    mutable int selected_=-1;
};
}
