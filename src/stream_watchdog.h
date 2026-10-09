// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
namespace cw {
class StreamWatchdog {
public:
    bool check(double now,bool ready,bool paused,uint64_t blocks){
        if(!ready||paused||blocks!=blocks_){blocks_=blocks;lastData_=now;nextRetry_=now+3;delay_=3;return false;}
        if(now-lastData_<3||now<nextRetry_)return false;
        nextRetry_=now+delay_;delay_=std::min(30.,delay_*2);return true;
    }
    void grace(double now){lastData_=now;nextRetry_=now+3;delay_=3;}
private:
    double lastData_=0,nextRetry_=0,delay_=3;
    uint64_t blocks_=0;
};
}
