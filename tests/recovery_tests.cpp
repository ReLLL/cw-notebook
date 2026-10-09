// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include "stream_watchdog.h"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::string code(char c){
    for(int len=1;len<=6;len++)for(int bits=0;bits<(1<<len);bits++){
        std::string s;for(int i=0;i<len;i++)s+=(bits&(1<<i))?'-':'.';
        if(cw::morse(s)==std::string(1,c))return s;
    }
    return {};
}
std::string receive(bool glitches,double letterGap=3,double innerGap=1){
    cw::Timing timing;timing.reset(25);std::string out;
    timing.output=[&](const std::string& s){out+=s;};
    auto run=[&](bool key,double units){for(int i=0;i<std::lround(units*48);i++)timing.tick(key);};
    const std::string message="VVV VVV VVV CQ CQ DE UT5MD NAME VLAD HW K";
    for(char c:message){
        if(c==' '){run(false,7-letterGap);continue;}
        auto symbols=code(c);
        for(size_t i=0;i<symbols.size();i++){
            double duration=symbols[i]=='.'?1:3;
            if(glitches){run(true,duration*.45);run(false,.08);run(true,duration*.55-.08);}
            else run(true,duration);
            double gap=i+1==symbols.size()?letterGap:innerGap;
            if(glitches){run(false,gap*.5);run(true,.04);run(false,gap*.5-.04);}
            else run(false,gap);
        }
    }
    run(false,10);return out;
}
int main(){try{
    for(bool recovery:{false,true})for(float speed:{5,8,12,18,25,40}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string decoded;
        timing.output=[&](const std::string& s){decoded+=s;};
        auto run=[&](bool key,double units){for(int i=0;i<std::lround(units*1200/speed);i++)timing.tick(key);};
        for(char c:std::string("CQ")){
            for(char s:code(c)){run(true,s=='.'?1:3);run(false,1);}
            run(false,2);
        }
        run(false,10);
        check(timing.calibrated()&&decoded=="CQ ","Short CQ remained stuck learning at "+std::to_string(speed)+": "+decoded);
    }
    for(bool recovery:{false,true})for(float speed:{5,8,12,18}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string decoded;
        timing.output=[&](const std::string& s){decoded+=s;};
        auto run=[&](bool key,double seconds){for(int i=0;i<std::lround(seconds*1000);i++)timing.tick(key);};
        // A few startup/interference pulses must not establish a fast station.
        for(int i=0;i<6;i++){run(true,i%2?.03:.01);run(false,.01);}
        check(!timing.calibrated()&&decoded.empty(),"Premature fast lock from startup pulses");
        for(char c:std::string("VVV VVV CQ CQ DE TEST SLOW CW")){
            if(c==' '){run(false,4*1.2/speed);continue;}
            for(char s:code(c)){run(true,(s=='.'?1:3)*1.2/speed);run(false,1.2/speed);}
            run(false,2*1.2/speed);
        }
        run(false,10*1.2/speed);
        check(decoded.find("VVV VVV CQ")==0,"Startup pulses leaked letters into slow decode: "+decoded);
        check(decoded.find("CQ CQ DE TEST SLOW CW")!=std::string::npos,"Slow acquisition failed at "+std::to_string(speed)+": "+decoded);
        check(std::abs(timing.wpm()-speed)<speed*.12,"Slow acquisition retained fast speed");
    }
    for(float speed:{5,12,20,25,40,80,120,200}){
        cw::Timing raw,recovered;raw.recovery(false);raw.reset(speed);recovered.reset(speed);
        std::string a,b;raw.output=[&](const std::string& s){a+=s;};recovered.output=[&](const std::string& s){b+=s;};
        std::mt19937 random(87);std::uniform_int_distribution<int> letter(0,35);
        std::string message="CQ DE ";for(int i=0;i<120;i++){int v=letter(random);message+=v<26?char('A'+v):char('0'+v-26);if(i%6==5)message+=' ';}
        auto run=[&](bool key,double units){for(int i=0;i<std::lround(units*1200/speed);i++){raw.tick(key);recovered.tick(key);}};
        for(char c:message){if(c==' '){run(false,4);continue;}for(char s:code(c)){run(true,s=='.'?1:3);run(false,1);}run(false,2);}run(false,10);
        check(a==b,"Recovery changed clean random Morse at "+std::to_string(speed));
    }
    cw::StreamWatchdog watchdog;
    check(!watchdog.check(0,true,false,1),"Watchdog restarted active stream");
    check(!watchdog.check(2,true,false,1),"Watchdog grace too short");
    check(watchdog.check(4,true,false,1),"Watchdog missed stalled stream");
    check(!watchdog.check(5,true,false,1),"Watchdog restart storm");
    check(!watchdog.check(10,true,true,1),"Watchdog ignored Pause");
    check(!watchdog.check(20,false,false,1),"Watchdog restarted stopped receiver");
    check(!watchdog.check(21,true,false,2),"Watchdog missed stream recovery");
    auto text=receive(true);std::cout<<"Fades and impulses: "<<text<<'\n';
    check(text.find("CQ CQ DE UT5MD NAME VLAD HW K")!=std::string::npos,"Short fade/impulse split valid Morse");
    text=receive(false,1.65);std::cout<<"Compressed spacing: "<<text<<'\n';
    check(text.find("CQ CQ DE UT5MD NAME VLAD HW K")!=std::string::npos,"Compressed character gaps merged letters");
    text=receive(false,3,1.65);std::cout<<"Stretched elements: "<<text<<'\n';
    check(text.find("CQ CQ DE UT5MD NAME VLAD HW K")!=std::string::npos,"Stretched element gaps split letters");
    std::cout<<"Recovery checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
