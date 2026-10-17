// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_skimmer.h"
#include <algorithm>

namespace rtty {
class SignalList {
public:
    void refresh(){ready_=false;}
    void update(const Status& status,const Selection& selection){
        if(ready_)return;
        rows_=status.signals;
        auto retain=[&](const auto& targets){for(const auto& target:targets){
            if(!std::any_of(rows_.begin(),rows_.end(),[&](const auto& v){return matches(target,v.center,v.shift);}))
                rows_.push_back({target.center,target.shift,false,false,false});
        }};
        retain(selection.pinned);retain(selection.ignored);
        std::sort(rows_.begin(),rows_.end(),[](const auto& a,const auto& b){return a.center<b.center;});
        ready_=true;
    }
    const std::vector<Status::Signal>& rows()const{return rows_;}
private:
    bool ready_=false;
    std::vector<Status::Signal> rows_;
};
}
