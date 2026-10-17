// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace rtty {
inline constexpr std::array<double,5> supportedShifts{170,200,425,450,850};
struct Target {double center=0,shift=170;};
struct Selection {std::vector<Target> pinned,ignored;bool onlyPinned=false;};
inline bool matches(const Target& a,double center,double shift){return std::abs(a.center-center)<45&&std::abs(a.shift-shift)<30;}
inline bool contains(const std::vector<Target>& list,double center,double shift){
    return std::any_of(list.begin(),list.end(),[&](const auto& t){return matches(t,center,shift);});
}
inline void remove(std::vector<Target>& list,double center,double shift){
    list.erase(std::remove_if(list.begin(),list.end(),[&](const auto& t){return matches(t,center,shift);}),list.end());
}
inline bool valid(const Target& t){
    return std::isfinite(t.center)&&t.center>0&&t.center<1e12&&
        std::find(supportedShifts.begin(),supportedShifts.end(),t.shift)!=supportedShifts.end();
}
inline Selection validated(Selection selection){
    auto filter=[](auto& list,size_t limit){
        std::vector<Target> unique;
        for(const auto& target:list){
            if(unique.size()>=limit)break;
            if(valid(target)&&!contains(unique,target.center,target.shift))unique.push_back(target);
        }
        list=std::move(unique);
    };
    filter(selection.pinned,16);filter(selection.ignored,64);
    for(const auto& t:selection.pinned)remove(selection.ignored,t.center,t.shift);
    return selection;
}
}
