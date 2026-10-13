// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <limits>

namespace cw {
struct ReceiveChannel {
    bool supported=false,wide=false,amAudio=false;
    int sampleRate=8000,sideband=1;
    double width=500,center=0,mixer=800,zeroTone=800;
    double carrierOffset(double tone) const {return amAudio?std::numeric_limits<double>::quiet_NaN():sideband*(tone-zeroTone);}
};
inline ReceiveChannel receiveChannel(const std::string& mode,double dialOffset,double selectedWidth,bool amAudio=false){
    ReceiveChannel p;
    p.supported=mode=="CW"||mode=="USB"||mode=="LSB"||mode=="DSB"||mode=="AM";
    p.center=dialOffset;
    if(!p.supported)return p;
    p.wide=mode!="CW";
    p.amAudio=mode=="AM"&&amAudio;
    double limit=mode=="AM"?24000:p.wide?12000:1200;
    if(!std::isfinite(selectedWidth)||selectedWidth<50||selectedWidth>limit){p.supported=false;return p;}
    p.width=selectedWidth;
    if(p.wide){
        p.sampleRate=p.width<=6000?16000:p.width<=12000?32000:64000;p.mixer=p.width/2+200;
        if(mode=="USB"){p.center+=p.width/2;p.zeroTone=200;}
        else if(mode=="LSB"){p.center-=p.width/2;p.sideband=-1;p.zeroTone=200;}
        else p.zeroTone=p.mixer;
    }
    return p;
}
class ChannelMixer {
public:
    float sample(float re,float im,const ReceiveChannel& p){
        if(p.amAudio){
            float envelope=std::hypot(re,im);
            dc_+=(1-std::exp(-2*3.141592653589793*50/p.sampleRate))*(envelope-dc_);
            return envelope-dc_;
        }
        float result=re*std::cos(phase_)-p.sideband*im*std::sin(phase_);
        phase_+=2*3.141592653589793*p.mixer/p.sampleRate;
        if(phase_>=2*3.141592653589793)phase_-=2*3.141592653589793;
        return result;
    }
    void reset(){phase_=0;dc_=0;}
private:
    double phase_=0;
    float dc_=0;
};
}
