// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
#include <cstdio>

void CWNotebook::receiveStatus(const cw::Snapshot& cw){
    auto r=rtty_?rtty_->snapshot():std::make_shared<const rtty::Status>();
    const char* state=!rttyEnabled_?"off":lastRate_>rtty::Skimmer::maxRate?"unsupported rate":!playing_?"receiver stopped":"monitoring";
    char line[320],capture[180];
    std::snprintf(line,sizeof(line),"RTTY %s | %zu tuned | %zu/16 decoders | %zu tabs | %zu waiting | %llu gaps",state,r->focusedDecoders,r->decoders,r->stations.size(),r->limited,static_cast<unsigned long long>(r->dropped));
    std::snprintf(capture,sizeof(capture),"Captured RF: %.3f-%.3f kHz at %.3f MS/s.",(lastCenter_-lastRate_/2)/1000,(lastCenter_+lastRate_/2)/1000,lastRate_/1e6);
    std::string summary=line,details=summary+"\n"+capture;
    if(r->focus.valid()){
        char priority[180];std::snprintf(priority,sizeof(priority),"\nRadio priority: %.3f-%.3f kHz. Every eligible pair here is checked first; the rest of the capture is still scanned.",r->focus.low/1000,r->focus.high/1000);details+=priority;
    }
    if(r->limited)details+="\nChoose priority frequencies in Signals, or close inactive tabs if tab capacity is full.";
    if(r->dropped)details+="\nInput gaps restart affected RTTY decoding. Lower the capture rate if the gap count keeps increasing.";
    if(lastRate_>rtty::Skimmer::maxRate)details+="\nSelect a capture rate of 4.096 MS/s or less to receive RTTY.";
    if(rttyEnabled_&&r->stations.empty())details+="\nListening for RTTY. Tabs open after tone and framing checks.";
    if(!rttyEnabled_)details+="\nRTTY processing is stopped; existing tab text remains available to copy.";
    std::string notice=!cw.error.empty()?"CW: "+cw.error:!r->error.empty()?"RTTY: "+r->error:!rttyNotice_.empty()?rttyNotice_:"";
    if(cw.dropped)details+="\nCW input gaps: "+std::to_string(cw.dropped)+". CW detection restarted.";
    if(notice.empty()&&ImGui::GetTime()-aiNoticeAt_<20&&!aiNotice_.empty())notice=aiNotice_;
    if(!notice.empty()){summary=notice+" | "+summary;details=notice+"\n"+details;}
    else if(!cw.notice.empty())details+="\nCW: "+cw.notice;
    details+="\nRTTY tab circles: green for one minute after confirmed activity. Each returning burst restarts the minute; yellow otherwise. Hover controls for help.";
    notebook::statusBar(summary.c_str(),details.c_str(),!notice.empty()||r->limited||r->dropped||cw.dropped);
}
