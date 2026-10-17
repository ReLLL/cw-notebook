// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <chrono>
#include <string>

namespace rtty {
inline double monotonicSeconds(){
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline bool recentText(double last,double now){return last>0&&now>=last&&now-last<=60;}
// Display-only folding of exact alternating R/Y runs. Never repair errors or
// alter the received transcript. A fixed label stays steady as the run grows.
inline std::string compactRY(const std::string& raw){
    std::string shown;shown.reserve(raw.size());
    for(size_t i=0;i<raw.size();){
        size_t end=i;
        if(raw[i]=='R'||raw[i]=='Y'){
            end=i+1;
            while(end<raw.size()&&(raw[end]=='R'||raw[end]=='Y')&&raw[end]!=raw[end-1])++end;
        }
        if(end-i>=12){shown+="[RY test pattern]";i=end;}
        else shown+=raw[i++];
    }
    return shown;
}
}
