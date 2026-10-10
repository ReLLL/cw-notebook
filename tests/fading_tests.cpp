// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

static void check(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
static std::string code(char c){
    for(int length=1;length<=5;++length)for(int bits=0;bits<(1<<length);++bits){
        std::string pattern;
        for(int i=0;i<length;++i)pattern+=(bits&(1<<i))?'-':'.';
        if(cw::morse(pattern)==std::string(1,c))return pattern;
    }
    return {};
}
static std::vector<float> signal(float speed,bool fade,bool noiseOnly=false,
    const std::string& message="VVV CQ CQ DE TEST K CQ CQ DE TEST K "){
    std::vector<float> samples;
    std::mt19937 rng(91);std::normal_distribution<float> noise(0,.001f);
    double time=0;
    auto run=[&](bool key,double units,float gain){
        for(int n=0;n<std::lround(units*1200/speed*8);++n){
            samples.push_back((key&&!noiseOnly?gain*std::sin(2*3.141592653589793*800*time):0)+noise(rng));
            time+=1.0/8000;
        }
    };
    run(false,8,0);
    for(size_t i=0;i<message.size();++i){
        if(message[i]==' '){run(false,4,0);continue;}
        // ~19 dB abrupt attenuation, still well above the background noise.
        float gain=fade&&i%3!=0?.07f:.6f;
        for(char mark:code(message[i])){run(true,mark=='.'?1:3,gain);run(false,1,0);}
        run(false,2,0);
    }
    run(false,12,0);return samples;
}
static std::string decode(const std::vector<float>& samples,bool recovery){
    cw::Decoder decoder;decoder.recovery(recovery);
    std::string text;decoder.output=[&](const std::string& s){text+=s;};
    for(size_t i=0;i<samples.size();i+=317)
        decoder.process(samples.data()+i,std::min(size_t(317),samples.size()-i),8000);
    return text;
}
int main(){try{
    for(float speed:{8,12,20,35,40}){
        auto audio=signal(speed,true);
        auto recovered=decode(audio,true);
        std::cout<<speed<<" WPM fade: "<<recovered<<'\n';
        check(recovered.find("CQ CQ DE TEST K CQ CQ DE TEST K")!=std::string::npos,
            "Audible faded characters lost at "+std::to_string(speed));
        auto clean=signal(speed,false);
        check(decode(clean,true)==decode(clean,false),"Recovery changed clean Morse");
        check(decode(signal(speed,false,true),true).empty(),"Fading recovery printed noise");
    }
    auto varied=decode(signal(20,true,false,"VVV NR 381 QRA R7DX ZK8A 029 QTC END "),true);
    check(varied.find("NR 381 QRA R7DX ZK8A 029 QTC END")!=std::string::npos,
        "Fading recovery failed varied letters/digits: "+varied);
    auto ended=signal(20,false);
    auto before=decode(ended,true);
    auto noise=signal(20,false,true);
    ended.insert(ended.end(),noise.begin(),noise.begin()+80000);
    check(decode(ended,true)==before,"Loss of carrier generated extra letters");
    std::cout<<"Fading checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
