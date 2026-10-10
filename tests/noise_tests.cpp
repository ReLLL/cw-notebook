// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

static std::string code(char c){
    for(int length=1;length<=5;++length)for(int bits=0;bits<(1<<length);++bits){
        std::string p;
        for(int i=0;i<length;++i)p+=(bits&(1<<i))?'-':'.';
        if(cw::morse(p)==std::string(1,c))return p;
    }
    return {};
}
static std::vector<float> signal(float speed,unsigned seed,bool keyed=true){
    std::vector<float> samples;std::mt19937 rng(seed);
    std::normal_distribution<float> noise(0,1);
    double time=0;float i1=0,i2=0,q1=0,q2=0;
    const float alpha=1-std::exp(-2*3.141592653589793*143/8000);
    auto run=[&](bool key,double units){
        for(int n=0;n<std::lround(units*1200/speed*8);++n){
            i1+=alpha*(noise(rng)-i1);i2+=alpha*(i1-i2);
            q1+=alpha*(noise(rng)-q1);q2+=alpha*(q1-q2);
            double phase=2*3.141592653589793*815*time;
            samples.push_back((key&&keyed?.6f:0)*std::sin(phase)+
                .8f*(i2*std::cos(phase)+q2*std::sin(phase)));
            time+=1.0/8000;
        }
    };
    run(false,8);
    for(char c:std::string("VVV CQ CQ DE TEST 123 QRS QRQ ABCDE K ")){
        if(c==' '){run(false,4);continue;}
        for(char mark:code(c)){run(true,mark=='.'?1:3);run(false,1);}
        run(false,2);
    }
    run(false,12);return samples;
}
int main(){try{
    for(bool recovery:{false,true})for(unsigned seed:{42,91,182}){
        cw::Decoder decoder;decoder.recovery(recovery);std::string text;
        decoder.output=[&](const std::string& s){text+=s;};
        auto samples=signal(20,seed);
        decoder.process(samples.data(),samples.size(),8000);
        std::cout<<seed<<" recovery="<<recovery<<" "<<decoder.stats().wpm<<" WPM: "<<text<<'\n';
        if(!decoder.stats().calibrated||std::abs(decoder.stats().wpm-20)>2)
            throw std::runtime_error("Band-limited noise corrupted automatic speed");
        if(text.find("CQ CQ DE TEST 123 QRS QRQ ABCDE K")==std::string::npos)
            throw std::runtime_error("Band-limited noise fragmented otherwise audible marks");
        decoder.reset();text.clear();samples=signal(20,seed,false);
        decoder.process(samples.data(),samples.size(),8000);
        if(!text.empty())throw std::runtime_error("Band-limited noise alone printed letters");
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
