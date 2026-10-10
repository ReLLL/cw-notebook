// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace cw {
void Timing::preview(double seconds){
    if(ready_)return;
    learningSeconds_+=seconds;previewSeconds_+=seconds;
    if(learningSeconds_<2||previewSeconds_<.25)return;
    previewSeconds_=0;
    const auto& runs=pending_.recent();
    if(runs.empty())return;
    // A bounded, explicitly tentative view. The complete measurement history
    // remains in RunBuffer for confirmed replay; no preview goes to output().
    double best=std::numeric_limits<double>::infinity(),unit=.06;
    for(double speed=5;speed<=45;speed+=.5){
        double u=1.2/speed,score=0;size_t count=0;
        for(const auto& r:runs){
            double x=r.seconds/u;
            if(!r.key&&x>7)continue;
            double cost=std::min((x-1)*(x-1),(x-3)*(x-3)/2.25);
            if(!r.key)cost=std::min(cost,(x-7)*(x-7)/4);
            score+=std::min(cost,16.0);++count;
        }
        if(!count)continue;
        // Modest 20 WPM prior resolves ambiguous one-class data without ever
        // changing the confirmed estimator's evidence requirements.
        score=score/count+.08*std::abs(std::log(speed/20));
        if(score<best){best=score;unit=u;}
    }
    if(acquisitionUnit_>0)unit=acquisitionUnit_;
    std::string text,symbol;
    auto letter=[&]{if(!symbol.empty()){text+=morse(symbol);symbol.clear();}};
    auto consume=[&](const KeyRun& r){
        if(r.key){
            if(r.seconds<unit*.18)return;
            if(r.seconds>unit*7||symbol.size()>=9){letter();text+="[?]";return;}
            symbol+=r.seconds<unit*2?'.':'-';
        }else{
            if(r.seconds>unit*2)letter();
            if(r.seconds>unit*5&&!text.empty()&&text.back()!=' ')text+=' ';
        }
    };
    // A tail may begin halfway through a character; label it as a preview tail.
    if(pending_.size()>runs.size())text="[Earlier intervals retained] ";
    for(const auto& r:runs)consume(r);
    consume({key_,duration_});
    letter(); // Last provisional character is allowed to change as its marks arrive.
    provisional_=std::move(text);provisionalWpm_=float(1.2/unit);
}
}
