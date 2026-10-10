// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace cw {
double Timing::recentUnit(const std::vector<double>& marks,bool relock) const {
    if(marks.size()<8)return 0;
    if(relock&&marks.size()<16)return 0;
    double best=std::numeric_limits<double>::infinity(),result=0;
    // Fit the Morse 1:3 model directly instead of letting tiny noise pulses
    // define the dot cluster. No text or station identity enters this fit.
    for(double speed=extendedSpeed_?3:5;speed<=(extendedSpeed_?200:45);speed+=.5){
        double unit=1.2/speed;
        if(ready_&&!relock&&(unit<unit_*.7||unit>unit_*1.4))continue;
        size_t dots=0,dashes=0,inner=0,longMarks=0;double loss=0;
        for(double mark:marks){
            double x=mark/unit;
            if(x>=.55&&x<=1.55)++dots;
            if(x>=2.2&&x<=3.8)++dashes;
            if(x>4.3)++longMarks;
            double error=std::min(std::abs(x-1),std::abs(x-3));
            loss+=std::min(error*error,1.0);
        }
        size_t n=0;
        for(auto it=gaps_.rbegin();it!=gaps_.rend()&&n<16;++it,++n)
            if(*it>=unit*.55&&*it<=unit*1.6)++inner;
        // Several long marks contradict a fast-speed interpretation. They
        // cannot all be discarded as interference simply because a faster
        // candidate fits the remaining short marks better.
        if(longMarks>=2||dots<3||dashes<3||inner<3||dots+dashes<marks.size()*.75)continue;
        if(relock&&(inner<6||dots+dashes<marks.size()*.9||loss/marks.size()>.12))continue;
        if(marks.back()<unit*.55||marks.back()>unit*3.8)continue;
        if(loss<best){best=loss;result=unit;}
    }
    return result;
}
}
