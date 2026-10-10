// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
int main(){try{
    for(bool recovery:{false,true}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();
        auto run=[&](bool key,int ms){for(int i=0;i<ms;++i)timing.tick(key);};
        // Distorted slow elements include two long marks. A fast fit must
        // not discard both as outliers and relabel the shorter marks dashes.
        const int marks[]={48,81,82,163,83,82,40,161,74,49};
        const int gaps[]={37,37,39,157,37,40,79,158,39,100};
        for(int i=0;i<10;++i){run(true,marks[i]);run(false,gaps[i]);}
        check(!timing.calibrated()||timing.wpm()<30,"Long slow marks discarded to manufacture a fast lock");
        for(int letter=0;letter<8;++letter){
            for(int mark:{180,60,180,60}){run(true,mark);run(false,60);}
            run(false,120);
        }
        check(timing.calibrated()&&std::abs(timing.wpm()-20)<2,"Slow timing did not recover after distorted prefix");
    }
    for(bool recovery:{false,true}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string text;
        timing.output=[&](const std::string& s){text+=s;};
        auto run=[&](bool key,int ms){for(int i=0;i<ms;++i)timing.tick(key);};
        auto send=[&](int unit){
            // Mixed, ordinary Morse timings; no station-specific vocabulary.
            for(int letter=0;letter<8;++letter){
                for(int mark:{3,1,3,1}){run(true,mark*unit);run(false,unit);}
                run(false,2*unit);
            }
        };
        send(30);check(timing.calibrated()&&std::abs(timing.wpm()-40)<2,"Failed to establish fast starting estimate");
        text.clear();send(60);run(false,420);
        check(std::abs(timing.wpm()-20)<2,"Established fast estimate trapped slower incoming timing");
        check(text.find("CCCC")!=std::string::npos,"Did not resume correct letters after speed correction: "+text);
        send(30);run(false,420);
        check(std::abs(timing.wpm()-40)<2,"Could not track a sustained faster operator");
        // A short different-speed fragment is insufficient to change lock.
        run(true,180);run(false,60);run(true,60);run(false,600);
        check(std::abs(timing.wpm()-40)<2,"Brief slow interference changed established speed");
        // A manual lock is intentional and must never be overridden.
        timing.setSpeed(40);send(60);
        check(std::abs(timing.wpm()-40)<.1,"Automatic recovery changed manual speed");
    }
    for(bool recovery:{false,true}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string text;
        timing.output=[&](const std::string& s){text+=s;};
        auto run=[&](bool key,int ms){for(int i=0;i<ms;++i)timing.tick(key);};
        // Interference followed by distorted, but distinguishable 60 ms dots
        // and 180 ms dashes. Edge displacement changes mark and gap equally.
        for(int i=0;i<64;++i){run(true,i%2?12:5);run(false,11);}
        check(!timing.calibrated(),"Interference established timing");
        int dots=0;
        for(int letter=0;letter<8;++letter){
            for(int element=0;element<4;++element){
                int ideal=element==3?180:60;
                int displacement=element==3?-20:(++dots%2? -20:20);
                run(true,ideal+displacement);
                run(false,(element==3?180:60)-displacement);
            }
            if(letter==3)check(timing.calibrated(),"Recent valid keying remained provisional after noise");
        }
        run(false,500);
        check(text.find("VVVV")!=std::string::npos,"Recent-window acquisition misread distorted marks: "+text);
        check(timing.wpm()>17&&timing.wpm()<24,"Distorted dots pulled timing to wrong speed");
    }
    for(bool recovery:{false,true}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string confirmed;
        timing.output=[&](const std::string& s){confirmed+=s;};
        auto run=[&](bool key,int ms){for(int i=0;i<ms;++i)timing.tick(key);};
        run(true,100);run(false,700);run(true,100);run(false,700);
        check(timing.provisional().empty(),"Preview should have a short observation window");
        run(true,100);run(false,700);
        check(!timing.provisional().empty()&&confirmed.empty(),"Provisional letters must appear within 2-3 seconds without becoming confirmed");
        check(timing.provisionalWpm()>=5&&timing.provisionalWpm()<=45,"Preview exceeded ordinary ham speeds");
        for(int ms:{300,100,300,100}){run(true,ms);run(false,100);}run(false,600);
        check(confirmed=="E E E C "&&timing.provisional().empty(),"Confirmed replay must replace provisional text without duplication");
        timing.reset();
        check(timing.provisional().empty()&&timing.bufferedRuns()==0,"Retune/reset retained provisional text");
    }
    for(bool recovery:{false,true})for(double speed:{5,8,12,18,25,40}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string text;
        timing.output=[&](const std::string& s){text+=s;};
        auto run=[&](bool key,double units){for(int i=0;i<std::lround(units*1200/speed);++i)timing.tick(key);};
        run(true,3);run(false,1);run(true,1);run(false,1);run(true,1);run(false,3);
        check(timing.calibrated()&&text=="D","Clean first D not flushed by its character gap at "+std::to_string(speed));
        run(false,20);check(text=="D ","Silence duplicated first character");
    }
    for(bool recovery:{false,true}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string text;
        timing.output=[&](const std::string& s){text+=s;};
        auto run=[&](bool key,int ms){for(int i=0;i<ms;++i)timing.tick(key);};
        // All dots are ambiguous until mixed evidence arrives. Keep the entire
        // prefix, including more runs than either the old cap or memory spool.
        for(int i=0;i<2300;++i){run(true,60);run(false,420);}
        check(text.empty()&&!timing.calibrated(),"Guessed speed from one duration class");
        run(false,15000);
        for(int ms:{180,60,180,60}){run(true,ms);run(false,60);}run(false,360);
        std::string expected;for(int i=0;i<2300;++i)expected+="E ";expected+="C ";
        check(text==expected,"Buffered ambiguous prefix lost, changed or duplicated: "+std::to_string(text.size()));
        timing.reset();text.clear();run(false,15000);
        check(text.empty(),"Explicit reset replayed old station");
        run(true,60);run(false,420);
        check(text.empty(),"Single E should remain ambiguous");
        timing.setSpeed(20);
        check(text=="E ","Manual speed lost pending reception");
    }
    // Exercise the actual envelope and quiet watchdog, not just Timing.
    for(bool recovery:{false,true}){
        cw::Decoder decoder;decoder.recovery(recovery);decoder.reset(0,800);
        std::string text;decoder.output=[&](const std::string& s){text+=s;};
        double phase=0;
        auto run=[&](bool key,int ms){
            std::vector<float> audio(ms*8);
            for(auto& sample:audio){sample=key?.6f*std::sin(phase):0;phase+=2*3.141592653589793*800/8000;}
            decoder.process(audio.data(),audio.size(),8000);
        };
        run(false,500);
        for(int i=0;i<8;++i){run(true,100);run(false,700);}
        auto buffered=decoder.stats().bufferedRuns;
        check(buffered>0&&text.empty(),"Envelope did not buffer ambiguous reception");
        run(false,15000);
        check(decoder.stats().bufferedRuns==buffered,"Silence watchdog discarded learning data");
        for(int ms:{300,100,300,100}){run(true,ms);run(false,100);}run(false,600);
        check(text=="E E E E E E E E C ","Audio learning replay lost initial text: "+text);
    }
    for(bool recovery:{false,true}){
        cw::Timing timing;timing.recovery(recovery);timing.reset();std::string text;
        timing.output=[&](const std::string& s){text+=s;};
        auto run=[&](bool key,int ms){for(int i=0;i<ms;++i)timing.tick(key);};
        for(int ms:{45,15,45,15}){run(true,ms);run(false,15);}run(false,105);
        check(!timing.calibrated()&&text.empty(),"Default ham range accepted 80 WPM");
        timing.extendedSpeed(true);
        check(timing.calibrated()&&text=="C ","Extended range did not replay held 80 WPM character");
        timing.extendedSpeed(false);
        check(!timing.calibrated(),"Returning to ham range retained fast lock");
        timing.setSpeed(80);
        check(timing.calibrated()&&std::abs(timing.wpm()-80)<1,"Manual speed should override ham range");
    }
    std::cout<<"Acquisition checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
