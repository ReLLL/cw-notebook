// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include "receive_channel.h"
#include <iostream>
#include <stdexcept>

static std::string code(char c){
    for(int n=1;n<=5;++n)for(int bits=0;bits<(1<<n);++bits){
        std::string p;for(int i=0;i<n;++i)p+=(bits&(1<<i))?'-':'.';
        if(cw::morse(p)==std::string(1,c))return p;
    }
    return {};
}
static void check(const std::string& mode,double width,double rfOffset,bool recovery,bool amAudio=false){
    const double dial=30000;
    auto p=cw::receiveChannel(mode,dial,width,amAudio);
    if(!p.supported)throw std::runtime_error("Receive mode rejected");
    double relative=dial+rfOffset-p.center;
    if(std::abs(relative)>p.width/2)throw std::runtime_error("Fixture outside receive passband");
    cw::ChannelMixer mixer;std::vector<float> audio;double time=0;
    auto run=[&](bool key,double units){
        for(int i=0;i<std::lround(units*.06*p.sampleRate);++i){
            double phase=2*3.141592653589793*relative*time;
            float level=amAudio?.5f*(1+(key?.7f*std::sin(2*3.141592653589793*750*time):0)):(key?.5f:0);
            audio.push_back(mixer.sample(level*std::cos(phase),level*std::sin(phase),p));
            time+=1.0/p.sampleRate;
        }
    };
    run(false,8);
    const std::string body="VVV CQ CQ DE TEST 123 K ";
    for(char c:body){
        if(c==' '){run(false,4);continue;}
        for(char mark:code(c)){run(true,mark=='.'?1:3);run(false,1);}
        run(false,2);
    }
    run(false,12);
    cw::Decoder decoder;decoder.wideInput(p.wide);decoder.recovery(recovery);
    std::string text;decoder.output=[&](const std::string& s){text+=s;};
    // Match the real callback's arbitrary block boundaries.
    for(size_t i=0;i<audio.size();i+=137)decoder.process(audio.data()+i,std::min(size_t(137),audio.size()-i),p.sampleRate);
    auto stats=decoder.stats();
    std::cout<<mode<<" RF offset="<<rfOffset<<" recovery="<<recovery<<" "<<stats.wpm<<" WPM: "<<text<<'\n';
    if(text.find("CQ CQ DE TEST 123 K")==std::string::npos||!stats.calibrated||std::abs(stats.wpm-20)>2)
        throw std::runtime_error("IQ channel failed to decode keyed carrier");
    if(amAudio){
        if(std::abs(stats.tone-750)>20||std::isfinite(p.carrierOffset(stats.tone)))throw std::runtime_error("AM tone mislabeled as RF offset");
    }else if(std::abs(p.carrierOffset(stats.tone)-rfOffset)>20)throw std::runtime_error("Carrier offset sign/reference incorrect");
    decoder.reset();text.clear();audio.assign(p.sampleRate*3,0);
    decoder.process(audio.data(),audio.size(),p.sampleRate);
    if(!text.empty())throw std::runtime_error("Silence generated text");
}
int main(){try{
    for(bool recovery:{false,true}){
        check("CW",400,0,recovery);
        for(double tone:{150.,750.,2400.,5000.}){
            double width=tone>3000?6000:3000;
            check("USB",width,tone,recovery);check("LSB",width,-tone,recovery);
        }
        for(double tone:{-2100.,-750.,0.,750.,2100.})check("DSB",4600,tone,recovery);
        check("USB",12000,11000,recovery);check("LSB",12000,-11000,recovery);
        check("DSB",12000,-5100,recovery);check("DSB",12000,5100,recovery);
        check("AM",24000,-11000,recovery);check("AM",10000,500,recovery,true);
    }
    if(cw::receiveChannel("FM",0,500).supported)
        throw std::runtime_error("Unsupported modes enabled");
    if(cw::receiveChannel("USB",0,NAN).supported)throw std::runtime_error("Invalid bandwidth accepted");
    auto p=cw::receiveChannel("USB",10000,12000);
    if(!p.supported||p.width!=12000||p.center!=16000||p.sampleRate!=32000)throw std::runtime_error("Full wide filter not captured");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
