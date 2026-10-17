// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_decoder.h"
#include <algorithm>
#include <cmath>

namespace rtty {
namespace {
constexpr char letters[]="\0E\nA SIU\rDRJNFCKTZLWHYPQOBG\0MXV\0";
constexpr char figureTable[]="\0" "3\n- '87\r$4\a,!:(5\")2#6019?&\0./;\0";
}
bool Decoder::Framer::acquisitionEvidence() const{
    // A periodic signal can fit start/stop bits yet print only I or P. Require
    // varied recent payload evidence, or the recognizable alternating RY test.
    // This qualifies acquisition only; never filter a confirmed transcript.
    std::array<unsigned,128> counts{};unsigned total=0,distinct=0,dominant=0,ry=0,changes=0;
    unsigned char lastPrintable=0;
    char previous=0;bool testPattern=false;
    for(size_t i=text.size()>64?text.size()-64:0;i<text.size();++i){
        auto c=static_cast<unsigned char>(text[i]);
        if(c=='R'||c=='Y')ry=(previous=='R'||previous=='Y')&&c!=previous?ry+1:1;else ry=0;
        if(ry>=12)testPattern=true;
        previous=char(c);
        if(c>='!'&&c<127){
            if(lastPrintable&&c!=lastPrintable)++changes;lastPrintable=c;
            ++total;if(counts[c]++==0)++distinct;dominant=std::max(dominant,counts[c]);
        }
    }
    double entropy=0;
    if(total)for(auto count:counts)if(count){double p=double(count)/total;entropy-=p*std::log2(p);}
    return testPattern||(total>=12&&distinct>=4&&dominant*4<=total*3&&changes>=4&&changes*3>=total-1&&entropy>=2.2);
}
void Decoder::Framer::sample(bool mark,bool reliable,double period){
    if(!receiving&&!reliable)return;
    if(!receiving){
        if(previous&&!mark&&idle>=period*.6&&reliable){receiving=true;age=0;next=period*.5;bit=-1;code=0;}
        idle=mark&&reliable?idle+1:0;
    }else{
        ++age;
        if(age>=next){
            const bool valid=reliable&&(bit==-1?!mark:bit>=5?mark:true);
            if(!valid){++errors;receiving=false;idle=0;}
            else if(bit<0){bit=0;next+=period;}
            else if(bit<5){if(mark)code|=1<<bit;++bit;next+=period;}
            else if(bit==5){++bit;next+=period*.5;}
            else{
                ++frames;receiving=false;idle=period;
                if(code==31)figures=false;
                else if(code==27)figures=true;
                else{
                    char c=(figures?figureTable:letters)[code];
                    if(c=='\r')c='\n';
                    if(c&&c!='\a'){
                        if(c!='\n'||text.empty()||text.back()!='\n')text+=c;
                        if(c>='!'&&c<='Z')++printable;
                    }
                }
                if(text.size()>40000)text.erase(0,text.size()-32000);
            }
        }
    }
    if(reliable)previous=mark;
}
Decoder::Decoder(double rate,double center,double shift){
    constexpr double pi=3.141592653589793;
    highStep_=std::polar(1.f,float(-2*pi*(center+shift/2)/rate));
    lowStep_=std::polar(1.f,float(-2*pi*(center-shift/2)/rate));
    const double bauds[]{45.45,50,75,100};
    for(size_t i=0;i<timings_.size();++i){
        auto& t=timings_[i];t.baud=bauds[i];t.period=rate/t.baud;
        size_t n=std::max(size_t(4),size_t(std::lround(t.period*.6)));
        t.high.resize(n);t.low.resize(n);t.power.resize(n);
    }
}
void Decoder::process(const IQ* samples,size_t count){
    for(size_t i=0;i<count;++i){
        IQ h=samples[i]*highPhase_,l=samples[i]*lowPhase_;
        highPhase_*=highStep_;lowPhase_*=lowStep_;
        if((++samples_&4095)==0){highPhase_/=std::abs(highPhase_);lowPhase_/=std::abs(lowPhase_);}
        for(auto& t:timings_){
            t.highSum+=h-t.high[t.index];t.lowSum+=l-t.low[t.index];
            float power=std::norm(samples[i]);t.powerSum+=power-t.power[t.index];t.power[t.index]=power;
            t.high[t.index]=h;t.low[t.index]=l;t.index=(t.index+1)%t.high.size();
            float hp=std::norm(t.highSum),lp=std::norm(t.lowSum);
            // A weak sideband can carry the same bits as its parent signal.
            // Require the proposed tone pair to explain substantial channel power.
            bool valid=std::isfinite(hp+lp)&&hp+lp>std::max(1e-12f,t.powerSum*t.high.size()*.3f)&&std::abs(hp-lp)>(hp+lp)*.16f;
            t.framers[0].sample(hp>lp,valid,t.period);
            t.framers[1].sample(lp>hp,valid,t.period);
        }
    }
}
DecodeResult Decoder::result() const{
    DecodeResult result;double best=0;int candidate=-1;
    for(size_t i=0;i<timings_.size();++i)for(size_t p=0;p<2;++p){
        const auto& t=timings_[i];
        const auto& f=t.framers[p];
        const int trial=int(i*2+p);
        if(selected_>=0&&selected_!=trial)continue;
        double score=double(f.frames)-1.5*f.errors;
        if(f.frames>=8&&f.printable>=4&&f.errors<=f.frames/3&&score>best){
            best=score;candidate=trial;result={f.text,t.baud,p==1,true,f.frames,f.errors};
        }
    }
    if(selected_<0&&candidate>=0){
        // Do not let a weaker, wrong-rate interpretation qualify merely because
        // the strongest framing trial correctly reports an ambiguous pattern.
        if(!timings_[size_t(candidate)/2].framers[size_t(candidate)%2].acquisitionEvidence())return {};
        selected_=candidate;
    }
    return result;
}
}
