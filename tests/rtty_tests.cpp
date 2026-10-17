// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_skimmer.h"
#include "rtty_engine.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
// Independent ITA2 fixture: LETTERS, CQ CQ DE TEST, FIGURES, 123, LETTERS, K.
const std::vector<int> message{31,14,23,4,14,23,4,9,1,4,16,1,5,16,4,27,23,19,1,31,4,15,8,2};
struct Transmitter {
    double rate,offset,shift,baud,time=0;
    bool reverse=false;
    rtty::IQ phase{1,0},step{1,0};
    size_t index=0;
    std::vector<bool> bits;
    Transmitter(double sr,double f,double s,double b,bool rev=false):rate(sr),offset(f),shift(s),baud(b),reverse(rev){
        // Half-bit units preserve exactly 1.5 stop bits, including noninteger baud.
        bits.assign(60,true);
        for(int repeat=0;repeat<6;++repeat)for(int code:message){
            bits.insert(bits.end(),2,false);
            for(int bit=0;bit<5;++bit)bits.insert(bits.end(),2,(code&(1<<bit))!=0);
            bits.insert(bits.end(),3,true);
        }
        update();
    }
    void update(){bool mark=index<bits.size()?bits[index]:true;if(reverse)mark=!mark;step=std::polar(1.f,float(2*3.141592653589793*(offset+(mark?1:-1)*shift/2)/rate));}
    void codes(const std::vector<int>& values,int repeats=12){
        bits.assign(60,true);index=0;time=0;
        for(int repeat=0;repeat<repeats;++repeat)for(int code:values){
            bits.insert(bits.end(),2,false);
            for(int bit=0;bit<5;++bit)bits.insert(bits.end(),2,(code&(1<<bit))!=0);
            bits.insert(bits.end(),3,true);
        }
        update();
    }
    rtty::IQ sample(){
        size_t next=size_t(time*baud*2);if(next!=index){index=next;update();}
        phase*=step;time+=1/rate;return phase;
    }
};
void decoder(){
    for(double baud:{45.45,50.,75.,100.})for(double shift:{170.,200.,425.,450.,850.})for(bool reverse:{false,true}){
        rtty::Decoder d(4000,0,shift);Transmitter tx(4000,0,shift,baud,reverse);
        std::vector<rtty::IQ> block(137);
        for(int b=0;b<400;++b){
            for(auto& x:block)x=tx.sample();d.process(block.data(),block.size());
            const auto live=d.result();
            require(!live.locked||(live.reverse==reverse&&live.baud==baud),"Early acquisition latched the wrong baud/polarity trial");
        }
        auto r=d.result();std::cout<<baud<<'/'<<shift<<'/'<<reverse<<" -> "<<r.baud<<": "<<r.text.substr(0,70)<<'\n';
        require(r.locked&&r.reverse==reverse&&r.baud==baud,"Baud/polarity acquisition failed");
        require(r.text.find("CQ CQ DE TEST 123 K")!=std::string::npos,"Baudot message failed");
    }
}
void shift200(){
    for(double rate:{8000.,96000.,4096000.})for(bool reverse:{false,true}){
        constexpr double center=7100000;
        const double offset=-rate*.2+13.7;
        rtty::Skimmer skim;Transmitter tx(rate,offset,200,50,reverse);
        std::vector<rtty::IQ> block(8191);
        for(size_t n=0;n<size_t(rate*10);n+=block.size()){
            for(auto& x:block)x=tx.sample();skim.process(block.data(),block.size(),rate,center);
        }
        const auto s=skim.snapshot();
        std::cout<<"200 Hz discovery: "<<rate<<" Hz, reverse="<<reverse<<", "<<s.stations.size()<<" tabs\n";
        require(s.stations.size()==1,"Exact 200 Hz discovery failed or duplicated a tab");
        const auto& station=s.stations.front();
        std::cout<<"Format "<<station.shift<<'/'<<station.baud<<'/'<<station.reverse<<": "<<station.text.substr(0,100)<<'\n';
        require(station.shift==200&&station.reverse==reverse&&station.baud==50,"200 Hz signal used the wrong format");
        require(std::abs(station.frequency-(center+offset+(reverse?-100:100)))<12,"200 Hz mark frequency is wrong");
        require(station.text.find("CQ CQ DE TEST 123 K")!=std::string::npos,"200 Hz discovery lost message text");
    }
}
void continuity(){
    // A long test pattern followed by changing text, with repeatable HF fades.
    // Already displayed text must not be replaced by another trial's history.
    rtty::Decoder decoder(4000,0,450);Transmitter tx(4000,0,450,50,true);
    tx.bits.assign(60,true);
    for(int repeat=0;repeat<20;++repeat){
        std::vector<int> codes{31};
        for(int i=0;i<30;++i){codes.push_back(10);codes.push_back(21);}
        codes.insert(codes.end(),message.begin(),message.end());
        for(int code:codes){
            tx.bits.insert(tx.bits.end(),2,false);
            for(int bit=0;bit<5;++bit)tx.bits.insert(tx.bits.end(),2,(code&(1<<bit))!=0);
            tx.bits.insert(tx.bits.end(),3,true);
        }
    }
    std::mt19937 rng(412);std::normal_distribution<float> noise(0,.09);
    std::vector<rtty::IQ> block(137);std::string previous;
    unsigned switches=0;double lastBaud=0;size_t good=0;
    for(size_t n=0;n<4000*120;n+=block.size()){
        for(auto& x:block){
            double time=tx.time;
            float amplitude=std::fmod(time,3.7)<.18?.035f:.6f;
            x=amplitude*tx.sample()+rtty::IQ(noise(rng),noise(rng));
        }
        decoder.process(block.data(),block.size());auto result=decoder.result();
        if(!result.locked)continue;
        if(result.baud!=lastBaud){++switches;lastBaud=result.baud;}
        if(result.text.compare(0,previous.size(),previous)!=0){
            std::cout<<"History replacement at "<<tx.time<<" s, baud "<<result.baud<<", switches "<<switches<<'\n';
            require(false,"Fading replaced already displayed RTTY text");
        }
        previous=result.text;
    }
    for(size_t pos=0;(pos=previous.find("CQ CQ DE TEST 123 K",pos))!=std::string::npos;pos+=18)++good;
    std::cout<<"Fading: "<<good<<" intact messages; "<<switches<<" trial choices; "<<previous.size()<<" characters\n";
    require(good>=1,"Repeated fading prevented RTTY recovery");
}
void ambiguousAcquisition(){
    // Framing alone is ambiguous: periodic interference can fit repeated
    // Baudot I/P, with occasional other letters. Do not identify it as RTTY
    // until there is more evidence; these are not language-correction tests.
    for(bool focused:{false,true})for(double shift:{170.,200.})for(int code:{6,22}){
        rtty::Skimmer skim;constexpr double rate=96000;
        if(focused)skim.focus({7105000,7107000});
        Transmitter tx(rate,6000,shift,code==6?100:75,true);
        std::vector<int> pattern;
        for(int rare:{7,14,22,19}){
            pattern.insert(pattern.end(),24,code);pattern.push_back(rare);
            pattern.push_back(4);pattern.push_back(8);pattern.push_back(2);
        }
        tx.codes(pattern,30);
        std::vector<rtty::IQ> block(997);
        for(size_t n=0;n<size_t(rate*14);n+=block.size()){
            for(auto& x:block)x=tx.sample();skim.process(block.data(),block.size(),rate,7100000);
        }
        const auto s=skim.snapshot();
        std::cout<<"Ambiguous repeated "<<(code==6?'I':'P')<<" at "<<shift<<" Hz: "<<s.stations.size()<<" tabs\n";
        for(const auto& station:s.stations)std::cout<<"Trial "<<station.baud<<'/'<<station.reverse<<": "<<station.text.substr(0,180)<<'\n';
        require(s.stations.empty(),"Repeated single-letter pattern falsely confirmed an RTTY tab");
    }
    // A legitimate alternating RY test is recognizable with only two letters.
    // After acquisition, preserve actual repeated letters inside a message.
    rtty::Decoder decoder(4000,0,450);Transmitter tx(4000,0,450,50,true);
    std::vector<int> codes{31};for(int i=0;i<12;++i){codes.push_back(10);codes.push_back(21);}
    codes.insert(codes.end(),message.begin(),message.end());
    codes.insert(codes.end(),20,6);codes.push_back(4);codes.insert(codes.end(),20,22);
    tx.codes(codes,4);std::vector<rtty::IQ> block(137);std::string prefix;
    for(int i=0;i<1500;++i){
        for(auto& x:block)x=tx.sample();decoder.process(block.data(),block.size());auto result=decoder.result();
        if(result.locked){require(result.baud==50&&result.reverse,"RY selected the wrong trial");prefix=result.text;}
    }
    require(prefix.find("RYRYRYRYRYRY")!=std::string::npos,"Real RY test pattern was rejected");
    require(prefix.find("IIIIIIIIIIIIIIIIIIII")!=std::string::npos&&prefix.find("PPPPPPPPPPPPPPPPPPPP")!=std::string::npos,"Confirmed reception had repeated characters filtered or rewritten");
    for(double baud:{45.45,50.,75.,100.})for(bool reverse:{false,true}){
        rtty::Decoder ryDecoder(4000,0,450);Transmitter ry(4000,0,450,baud,reverse);
        ry.codes({31,10,21,10,21,10,21,10,21,10,21,10,21,10,21,10,21},30);
        bool acquired=false;
        for(int i=0;i<116;++i){
            for(auto& x:block)x=ry.sample();ryDecoder.process(block.data(),block.size());auto result=ryDecoder.result();
            if(result.locked){require(result.baud==baud&&result.reverse==reverse,"RY-only acquisition chose the wrong trial");acquired=true;}
        }
        require(acquired,"RY-only traffic failed to acquire within the four-second candidate lifetime");
    }
}
void gappedAcquisition(){
    constexpr double rate=96000,center=14080000;
    rtty::Skimmer skim;
    Transmitter a(rate,-18000,450,50,true),b(rate,18000,170,45.45),i(rate,-32000,170,100,true),p(rate,32000,170,75);
    a.codes(message,100);b.codes(message,100);
    std::vector<int> first,second;
    for(int rare:{7,14,22,19}){
        first.insert(first.end(),24,6);first.insert(first.end(),{rare,4,8,2});
        second.insert(second.end(),24,22);second.insert(second.end(),{rare,4,8,2});
    }
    i.codes(first,100);p.codes(second,100);std::vector<rtty::IQ> block(997);
    size_t nextGap=size_t(rate*6.173);
    for(size_t n=0;n<size_t(rate*60);n+=block.size()){
        for(auto& x:block){
            const auto iq=.14f*(a.sample()+b.sample()+i.sample()+p.sample());
            // Native File source fixture uses stereo signed-16 IQ.
            x={float(std::lround(iq.real()*30000))/32768,float(std::lround(iq.imag()*30000))/32768};
        }
        if(n>=nextGap){skim.discontinuity();nextGap+=size_t(rate*6.173);}
        skim.process(block.data(),block.size(),rate,center);
        for(const auto& station:skim.snapshot().stations)
            require(std::abs(station.center-center)<25000,"Gap/reacquisition opened an ambiguous periodic stream");
    }
    require(skim.snapshot().stations.size()==2,"Gap regression failed to retain both legitimate message streams");
}
void wideband(double rate=1024000){
    constexpr double center=14080000;
    rtty::Skimmer skim;
    const double af=rate==96000?-18000:-180123,bf=rate==96000?21000:210031,cf=rate==96000?44000:498011;
    const double df=rate==96000?-31000:-50013;
    Transmitter a(rate,af,170,45.45),b(rate,bf,425,50,true),c(rate,cf,850,75),d(rate,df,200,100);
    std::vector<rtty::IQ> block(8191);
    std::mt19937 rng(71);std::normal_distribution<float> noise(0,.015);
    double ms=0;
    for(size_t n=0;n<size_t(rate*10);n+=block.size()){
        for(auto& x:block)x=.4f*a.sample()+.3f*b.sample()+.25f*c.sample()+.3f*d.sample()+rtty::IQ(noise(rng),noise(rng));
        auto start=std::chrono::steady_clock::now();skim.process(block.data(),block.size(),rate,center);
        ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    }
    auto status=skim.snapshot();
    std::cout<<"Wideband: "<<status.stations.size()<<" tabs; "<<status.candidates<<" candidates; "<<ms<<" ms / 10 s\n";
    for(const auto& s:status.stations)std::cout<<s.frequency<<' '<<s.shift<<' '<<s.baud<<": "<<s.text.substr(0,70)<<'\n';
    for(double f:{center+af+85,center+bf-212.5,center+cf+425,center+df+100}){
        bool found=false;
        for(const auto& s:status.stations)if(std::abs(s.frequency-f)<35&&s.text.find("CQ CQ DE TEST 123 K")!=std::string::npos)found=true;
        require(found,"Full-spectrum multistation decode failed");
    }
    require(status.stations.size()==4,"Duplicate or false tabs");
    skim.discontinuity();require(skim.snapshot().decoders==0,"Gap retained decoder state");
    skim.process(block.data(),0,rate,center+1000);
    for(const auto& s:skim.snapshot().stations)require(!s.active,"Retune left an active stale station");
    skim.closeAll();require(skim.snapshot().stations.empty()&&skim.snapshot().decoders==0,"Close all retained tabs or pending text");
}
void ratesAndEdges(){
    for(double rate:{8000.,96000.,2400000.,4096000.}){
        rtty::Skimmer skim;Transmitter tx(rate,rate/2-540,850,100,true);
        std::vector<rtty::IQ> block(8191);double ms=0;
        for(size_t n=0;n<size_t(rate*5);n+=block.size()){
            for(auto& x:block)x=tx.sample();
            auto start=std::chrono::steady_clock::now();skim.process(block.data(),block.size(),rate,7100000);
            ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        }
        auto status=skim.snapshot();
        std::cout<<"Rate "<<rate<<" edge decode "<<status.stations.size()<<" tabs; "<<ms<<" ms / 5 s\n";
        for(const auto& station:status.stations)std::cout<<"Edge format "<<station.shift<<'/'<<station.baud<<'/'<<station.reverse<<": "<<station.text.substr(0,100)<<'\n';
        require(status.stations.size()==1&&status.stations[0].text.find("CQ CQ DE TEST 123 K")!=std::string::npos,"Sample-rate or capture-edge decode failed");
    }
}
void worker(){
    rtty::Engine engine;std::vector<rtty::IQ> block(4096);
    engine.configure(96000,7100000,true);
    for(int i=0;i<1000;++i)engine.input(block.data(),block.size());
    require(!engine.input(block.data(),rtty::Engine::packetSize+1),"Oversized packet accepted");
    engine.closeAll();engine.configure(96000,7101000,false);
    for(int i=0;i<100&&engine.snapshot()->decoders;++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    require(engine.snapshot()->stations.empty(),"Asynchronous close all failed");
    engine.configure(9000000,7100000,true);
    require(!engine.input(block.data(),block.size()),"Unsupported rate accepted");
    engine.focus({7040000,7045000});
    for(int i=0;i<100&&engine.snapshot()->focus.low!=7040000;++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    require(engine.snapshot()->focus.low==7040000&&!engine.snapshot()->receiving,"Worker lost a passband update without input");
}
void capacity(){
    constexpr double rate=96000;
    std::vector<Transmitter> signals;
    for(int i=0;i<24;++i)signals.emplace_back(rate,-36000+i*3000,170,45.45);
    std::vector<rtty::IQ> block(2047);rtty::Skimmer skim;double ms=0;
    for(size_t n=0;n<size_t(rate*10);n+=block.size()){
        for(auto& x:block){x={};for(auto& tx:signals)x+=.03f*tx.sample();}
        auto start=std::chrono::steady_clock::now();skim.process(block.data(),block.size(),rate,14080000);
        ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        require(skim.snapshot().decoders<=rtty::Skimmer::maxDecoders,"Decoder budget exceeded");
    }
    auto s=skim.snapshot();std::cout<<"Capacity: "<<s.candidates<<" candidates, "<<s.decoders<<" decoders, "<<s.stations.size()<<" tabs, "<<s.limited<<" limited; "<<ms<<" ms / 10 s\n";
    require(s.limited>0&&s.stations.size()==rtty::Skimmer::maxDecoders,"Capacity limit was hidden or decoding failed");
    auto waiting=std::find_if(s.signals.begin(),s.signals.end(),[](const auto& signal){return !signal.decoding;});
    require(waiting!=s.signals.end(),"Waiting candidates unavailable for operator selection");
    rtty::Selection selection;selection.pinned.push_back({waiting->center,waiting->shift});skim.selection(selection);
    auto feed=[&](double seconds){
        for(size_t n=0;n<size_t(rate*seconds);n+=block.size()){
            for(auto& x:block){x={};for(auto& tx:signals)x+=.03f*tx.sample();}
            skim.process(block.data(),block.size(),rate,14080000);
        }
    };
    feed(4);auto prioritized=skim.snapshot();
    require(prioritized.decoders==16,"Pinning exceeded or lost decoder capacity");
    require(std::any_of(prioritized.stations.begin(),prioritized.stations.end(),[&](const auto& station){return station.active&&rtty::matches(selection.pinned[0],station.center,station.shift);}),"Pinned waiting signal did not displace an automatic decoder");
    selection.onlyPinned=true;skim.selection(selection);feed(2);
    require(skim.snapshot().decoders==1,"Only-pinned mode retained unwanted decoders");
    auto chosen=selection.pinned[0];selection.pinned.clear();selection.ignored.push_back(chosen);skim.selection(selection);feed(2);
    auto ignored=skim.snapshot();require(ignored.decoders==0,"Ignoring retained an active decoder");
    require(std::none_of(ignored.stations.begin(),ignored.stations.end(),[&](const auto& station){return rtty::matches(chosen,station.center,station.shift);}),"Ignored signal reopened its tab");
    selection={};skim.selection(selection);feed(3);
    require(skim.snapshot().decoders==16,"Clearing selection failed to restore automatic decoding");
    auto beforeClose=skim.snapshot();skim.close(beforeClose.stations[0].id);require(skim.snapshot().stations.size()==beforeClose.stations.size()-1,"Individual close failed");
    skim.closeAll();require(skim.snapshot().stations.empty()&&skim.snapshot().decoders==0,"Close all failed with full decoder budget");
}
void tunedPriority(){
    constexpr double rate=96000,center=7045000;
    std::vector<Transmitter> signals;
    for(int i=0;i<20;++i){signals.emplace_back(rate,-30000+i*3000,170,45.45);signals.back().codes(message,100);}
    rtty::Skimmer skim;std::vector<rtty::IQ> block(2047);
    auto feed=[&](double seconds){
        for(size_t n=0;n<size_t(rate*seconds);n+=block.size()){
            for(auto& x:block){x={};for(size_t i=0;i<signals.size();++i)x+=(i<16?.03f:.006f)*signals[i].sample();}
            skim.process(block.data(),block.size(),rate,center);
            require(skim.snapshot().decoders<=16,"Tuned priority exceeded the decoder budget");
        }
    };
    feed(10);const auto before=skim.snapshot();
    require(before.decoders==16,"Tuned priority fixture did not fill the decoder budget");
    skim.focus({center+20000,center+28000});feed(.3);
    require(skim.snapshot().focusedDecoders==3,"Tuned candidates were not admitted on the next discovery pass");
    feed(7.7);
    auto s=skim.snapshot();
    require(s.focusedDecoders==3&&s.decoders==16,"Tuned passband did not give all three streams priority while retaining band decoding");
    for(const auto& a:s.stations)if(s.focus.contains(a.center,a.shift))std::cout<<"Tuned "<<a.center<<" active="<<a.active<<": "<<a.text<<'\n';
    for(double offset:{21000.,24000.,27000.})
        require(std::any_of(s.stations.begin(),s.stations.end(),[&](const auto& a){return a.active&&std::abs(a.center-center-offset)<30&&a.text.find("CQ CQ DE TEST 123 K")!=std::string::npos;}),"A tuned RTTY stream failed to decode");
    for(const auto& old:before.stations)
        require(std::any_of(s.stations.begin(),s.stations.end(),[&](const auto& a){return a.id==old.id&&a.text.compare(0,old.text.size(),old.text)==0;}),"Tuned priority lost existing tab text");
    skim.focus({center+17000,center+19000});feed(6);s=skim.snapshot();
    require(s.focusedDecoders==1&&s.decoders==16,"Priority did not follow a changed passband");
    require(std::any_of(s.stations.begin(),s.stations.end(),[&](const auto& a){return a.active&&std::abs(a.center-center-18000)<30;}),"Retuning priority left the new station waiting");
    rtty::Selection selection;selection.ignored={{center+18000,170}};skim.selection(selection);feed(.5);
    require(skim.snapshot().focusedDecoders==0,"Tuned priority overrode an explicit ignore");
    selection={};selection.onlyPinned=true;skim.selection(selection);feed(.5);
    require(skim.snapshot().decoders==0,"Tuned priority overrode Only pinned");
}
void tunedWeakPeaks(){
    constexpr double rate=1024000,center=7045000;
    constexpr size_t period=65536;
    std::vector<rtty::IQ> carriers(period);
    size_t tones=0;
    for(int bin=-30000;bin<=30000;bin+=197){
        if(std::abs(bin*rate/period-1500)<3000)continue;
        carriers[(bin+int(period))%period]=std::polar(.04f*period,float(tones++*.71));
    }
    require(tones>256,"Weak focused fixture did not exceed the global peak budget");
    rtty::FFT fft(period);fft.transform(carriers,true);
    rtty::Skimmer focused,baseline;focused.focus({center+500,center+2500});
    Transmitter tx(rate,1500,200,50);std::vector<rtty::IQ> block(8191);size_t index=0;
    for(size_t n=0;n<size_t(rate*10);n+=block.size()){
        for(auto& x:block){x=carriers[(index++)%period]+.004f*tx.sample();}
        focused.process(block.data(),block.size(),rate,center);baseline.process(block.data(),block.size(),rate,center);
    }
    auto decoded=[&](const auto& s){return std::any_of(s.stations.begin(),s.stations.end(),[&](const auto& a){return std::abs(a.center-center-1500)<30&&a.shift==200&&a.text.find("CQ CQ DE TEST 123 K")!=std::string::npos;});};
    require(!decoded(baseline.snapshot()),"Weak fixture was not excluded by the global peak budget");
    require(decoded(focused.snapshot()),"Tuned weak pair was lost behind stronger out-of-passband peaks");
}
void rejection(){
    rtty::Skimmer skim;constexpr double rate=96000;
    std::mt19937 rng(907);std::normal_distribution<float> noise(0,.2);
    std::vector<rtty::IQ> block(1009);
    Transmitter carrier(rate,3000,0,50),other(rate,3170,0,50);
    for(size_t n=0;n<size_t(rate*12);n+=block.size()){
        for(auto& x:block)x=carrier.sample()+other.sample()+rtty::IQ(noise(rng),noise(rng));
        skim.process(block.data(),block.size(),rate,7100000);
    }
    require(skim.snapshot().stations.empty(),"Noise or unmodulated pair opened an RTTY tab");
}
void reacquisition(){
    rtty::Skimmer skim;constexpr double rate=96000;
    std::string previous;
    for(double shift:{425.,450.,425.}){
        Transmitter tx(rate,4000,shift,50);std::vector<rtty::IQ> block(997);
        skim.discontinuity();
        for(size_t n=0;n<size_t(rate*10);n+=block.size()){
            for(auto& x:block)x=tx.sample();skim.process(block.data(),block.size(),rate,7100000);
        }
        auto status=skim.snapshot();
        require(status.stations.size()==1,"Reacquiring an ambiguous 425/450 Hz pair duplicated its tab");
        require(status.stations[0].text.find("CQ CQ DE TEST 123 K")!=std::string::npos,"Reacquisition lost confirmed text");
        require(status.stations[0].text.compare(0,previous.size(),previous)==0,"Reacquisition replaced earlier confirmed text");
        previous=status.stations[0].text;
    }
}
}
int main(int argc,char** argv){try{if(argc>1){if(std::string(argv[1])=="ambiguous"){ambiguousAcquisition();gappedAcquisition();}else if(std::string(argv[1])=="focus"){tunedPriority();tunedWeakPeaks();worker();}else if(std::string(argv[1])=="200")shift200();else if(std::string(argv[1])=="edges")ratesAndEdges();else continuity();return 0;}decoder();shift200();ambiguousAcquisition();gappedAcquisition();continuity();wideband();wideband(96000);ratesAndEdges();rejection();reacquisition();worker();capacity();tunedPriority();tunedWeakPeaks();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
