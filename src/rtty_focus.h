// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

namespace rtty {
struct Focus {
    double low=0,high=0;
    bool valid() const{return std::isfinite(low)&&std::isfinite(high)&&low>0&&high>low;}
    bool contains(double center,double shift=0) const{return valid()&&center-shift/2>=low&&center+shift/2<=high;}
    bool operator==(const Focus& other) const{return low==other.low&&high==other.high;}
};
inline Focus radioFocus(const std::string& mode,double dial,double width,double center,double rate){
    if(!std::isfinite(dial)||!std::isfinite(width)||width<=0||!std::isfinite(center)||
       !std::isfinite(rate)||rate<=0||center<=0)return {};
    if(mode!="USB"&&mode!="LSB"&&mode!="AM"&&mode!="CW"&&mode!="DSB"&&
       mode!="NFM"&&mode!="WFM"&&mode!="RAW"&&mode!="DSD"&&mode!="oldDSD")return {};
    const double middle=dial+(mode=="USB"?width/2:mode=="LSB"?-width/2:0);
    Focus focus{std::max(middle-width/2,center-rate/2),std::min(middle+width/2,center+rate/2)};
    return focus.valid()?focus:Focus{};
}
}
