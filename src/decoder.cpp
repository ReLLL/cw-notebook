// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace cw {
namespace {
constexpr double pi = 3.14159265358979323846;
double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[v.size()/2];
}
}
std::string morse(const std::string& s) {
    static const std::unordered_map<std::string, std::string> table = {
        {".-","A"},{"-...","B"},{"-.-.","C"},{"-..","D"},{".","E"},{"..-.","F"},
        {"--.","G"},{"....","H"},{"..","I"},{".---","J"},{"-.-","K"},{".-..","L"},
        {"--","M"},{"-.","N"},{"---","O"},{".--.","P"},{"--.-","Q"},{".-.","R"},
        {"...","S"},{"-","T"},{"..-","U"},{"...-","V"},{".--","W"},{"-..-","X"},
        {"-.--","Y"},{"--..","Z"},{"-----","0"},{".----","1"},{"..---","2"},
        {"...--","3"},{"....-","4"},{".....","5"},{"-....","6"},{"--...","7"},
        {"---..","8"},{"----.","9"},{".-.-.-","."},{"--..--",","},{"..--..","?"},
        {"-..-.","/"},{"-...-","="},{".-.-.","<AR>"},{"...-.-","<SK>"},
        {"-.-.-","<KA>"},{"........","<HH>"},{".-...","<AS>"},{".--.-.","@"},
        {"-....-","-"},{".----.","'"},{"-.--.","("},{"-.--.-",")"},{"---...",":"},
        {"-.-.--","!"},{".-..-.","\""},{"..--.-","_"},{"-.-.-.",";"}
    };
    auto it = table.find(s);
    return it == table.end() ? "[?]" : it->second;
}
void Timing::reset(float manualWpm) {
    key_ = started_ = ready_ = letterSent_ = wordSent_ = false;
    duration_ = 0; manual_ = manualWpm;
    unit_ = manualWpm > 0 ? 1.2 / std::clamp(double(manualWpm),3.0,200.0) : .06;
    cutoff_ = 2 * unit_;
    marks_.clear(); gaps_.clear(); pending_.clear(); symbol_.clear();
    ready_ = manualWpm > 0;
}
void Timing::estimate() {
    if (manual_ > 0 || marks_.size() < 4) return;
    std::vector<double> v(marks_.begin(), marks_.end());
    std::sort(v.begin(), v.end());
    double a=v[v.size()/5], b=v[v.size()*4/5];
    for (int n=0;n<8;n++) {
        std::vector<double> lo,hi;
        for (double x:v) (x<(a+b)/2 ? lo:hi).push_back(x);
        if (lo.empty() || hi.empty()) break;
        a=median(lo); b=median(hi);
    }
    if (b/a > 1.8 && b/a < 5.5 && a>=.004 && b<=1.3) {
        unit_=std::clamp((a+b/3)/2,.006,.4);
        cutoff_=(a+b)/2;
        ready_=true;
    } else if (!ready_ && gaps_.size()>=6) {
        std::vector<double> g(gaps_.begin(),gaps_.end());
        std::sort(g.begin(),g.end());
        double u=g[g.size()/5];
        if(u>=.006 && u<=.4 && (a/u<1.5 || (a/u>2.2 && a/u<3.8))) {
            unit_=u; cutoff_=2*u; ready_=true;
        }
    }
}
void Timing::letter() {
    if (!symbol_.empty() && output) output(morse(symbol_));
    symbol_.clear(); letterSent_=true;
}
void Timing::consume(const Run& r) {
    if (r.key) {
        if (r.seconds < std::max(.002,unit_*.18)) return;
        if (r.seconds > unit_*7 || symbol_.size()>=9) {
            if (output) output("[?]");
            symbol_.clear(); return;
        }
        symbol_ += r.seconds < cutoff_ ? '.' : '-';
        letterSent_=wordSent_=false;
    } else {
        if (r.seconds>unit_*2 && !letterSent_) letter();
        if (r.seconds>unit_*5 && !wordSent_) {
            if (output) output(" ");
            wordSent_=true;
        }
    }
}
void Timing::tick(bool key, double seconds) {
    if (!started_) { if(!key)return; started_=true; key_=key; }
    if (key != key_) {
        Run r{key_,duration_};
        if (key_ && duration_>=.003 && duration_<=1.3) {
            marks_.push_back(duration_); if(marks_.size()>64)marks_.pop_front();
        } else if (!key_ && duration_>=.003 && duration_<1.5) {
            gaps_.push_back(duration_); if(gaps_.size()>64)gaps_.pop_front();
        }
        bool wasReady=ready_;
        estimate();
        if (!wasReady) {
            pending_.push_back(r);
            if(pending_.size()>512) pending_.erase(pending_.begin(),pending_.begin()+256);
            if(ready_) { for(const auto& p:pending_)consume(p); pending_.clear(); }
        } else consume(r);
        key_=key; duration_=0;
    }
    duration_+=seconds;
    if (ready_ && !key_) consume({false,duration_});
}
Decoder::Decoder() { reset(); }
void Decoder::reset(float wpm, float tone) {
    timing_.reset(wpm);
    timing_.output=[this](const std::string& s){if(output)output(s);};
    stats_={}; if(wpm>0)stats_.wpm=wpm; manualTone_=tone; if(tone>0)stats_.tone=tone;
    audio_.fill(0); levels_.fill(0);
    audioPos_=levelPos_=samples_=ticks_=0;
    noise_=peak_=low1_=low2_=iqI_=iqQ_=i2_=q2_=sum_=0;
    phase_=resample_=0; key_=false; toneLocked_=tone>0; debounce_=rate_=0;
    candidateTone_=0;stableTones_=0;lastToneTick_=0;
}
void Decoder::process(const float* mono,size_t count,int rate) {
    if(rate<4000 || rate>384000)return;
    rate_=rate;
    const float alpha=1-std::exp(-2*pi*1700/rate);
    for(size_t i=0;i<count;i++) {
        float x=std::isfinite(mono[i])?mono[i]:0;
        low1_+=alpha*(x-low1_); low2_+=alpha*(low1_-low2_);
        resample_+=4000;
        if(resample_>=rate) { resample_-=rate; sample(low2_); }
    }
}
void Decoder::findTone() {
    if(manualTone_>0) {stats_.tone=manualTone_;stats_.signal=true;return;}
    std::array<std::complex<float>,1024> fft;
    for(size_t i=0;i<1024;i++)fft[i]={audio_[(audioPos_+i)%1024]*float(.5-.5*std::cos(2*pi*i/1023)),0};
    for(size_t i=1,j=0;i<1024;i++) {
        size_t bit=512;for(;j&bit;bit>>=1)j^=bit;j^=bit;
        if(i<j)std::swap(fft[i],fft[j]);
    }
    for(size_t len=2;len<=1024;len*=2) {
        auto step=std::polar(1.f,float(-2*pi/len));
        for(size_t i=0;i<1024;i+=len) {
            std::complex<float>w=1;
            for(size_t j=0;j<len/2;j++) {auto u=fft[i+j],v=fft[i+j+len/2]*w;fft[i+j]=u+v;fft[i+j+len/2]=u-v;w*=step;}
        }
    }
    size_t best=51;
    std::vector<float> powers;
    for(size_t i=51;i<461;i++) {powers.push_back(std::norm(fft[i]));if(std::norm(fft[i])>std::norm(fft[best]))best=i;}
    std::sort(powers.begin(),powers.end());
    float peak=std::norm(fft[best]), floor=powers[powers.size()/2];
    std::vector<float> local;
    for(int i=std::max(1,int(best)-16);i<std::min(511,int(best)+17);i++)
        if(std::abs(i-int(best))>3)local.push_back(std::norm(fft[i]));
    std::sort(local.begin(),local.end());
    floor=std::max(floor,local[local.size()/2]);
    stats_.snr=10*std::log10((peak+1e-15f)/(floor+1e-15f));
    stats_.signal=stats_.snr>13 && peak>1e-10;
    if(stats_.signal) {
        float a=std::abs(fft[best-1]),b=std::abs(fft[best]),c=std::abs(fft[best+1]);
        float offset=.5f*(a-c)/(a-2*b+c-1e-20f);
        float f=(best+std::clamp(offset,-.5f,.5f))*4000/1024;
        stableTones_=std::abs(f-candidateTone_)<12?stableTones_+1:1;
        candidateTone_=f;
        if(stableTones_>=3){
            toneLocked_=true;lastToneTick_=ticks_;
            stats_.tone=std::abs(f-stats_.tone)>40?f:stats_.tone*.6f+f*.4f;
        }
    }else stableTones_=0;
    if(ticks_-lastToneTick_>3000)toneLocked_=false;
}
void Decoder::sample(float x) {
    audio_[audioPos_]=x;audioPos_=(audioPos_+1)%1024;
    if(++samples_%512==0 && samples_>=1024)findTone();
    float cutoff=timing_.calibrated()?std::clamp(timing_.wpm()*4.f,90.f,500.f):200.f;
    float alpha=1-std::exp(-2*pi*cutoff/4000);
    phase_+=2*pi*stats_.tone/4000;if(phase_>2*pi)phase_-=2*pi;
    iqI_+=alpha*(x*std::cos(phase_)-iqI_);iqQ_+=alpha*(x*std::sin(phase_)-iqQ_);
    i2_+=alpha*(iqI_-i2_);q2_+=alpha*(iqQ_-q2_);
    sum_+=std::hypot(i2_,q2_);
    if(samples_%4==0){envelope(sum_/4);sum_=0;}
}
void Decoder::envelope(float x) {
    levels_[levelPos_]=x;levelPos_=(levelPos_+1)%levels_.size();
    if(++ticks_%32==0) {
        size_t count=std::min(ticks_,levels_.size());
        std::vector<float> sorted(levels_.begin(),levels_.begin()+count);
        std::sort(sorted.begin(),sorted.end());
        noise_=sorted[count/5];
    }
    peak_=std::max(x,peak_*.997f);
    float range=std::max(peak_-noise_,1e-9f);
    float threshold=noise_+range*(key_?.22f:.38f);
    bool candidate=x>std::max(threshold,noise_*5.f) && peak_>noise_*6.f && peak_>1e-7f && toneLocked_;
    int debounceTicks=timing_.calibrated()?std::clamp(int(144/timing_.wpm()),2,12):2;
    if(candidate!=key_) {if(++debounce_>=debounceTicks){key_=candidate;debounce_=0;}} else debounce_=0;
    // Delay acquisition until the first tone estimate, avoiding startup filter clicks.
    if(ticks_>=300)timing_.tick(key_);
    stats_.level=x;stats_.threshold=threshold;stats_.keyed=key_;
    stats_.wpm=timing_.wpm();stats_.calibrated=timing_.calibrated();
    if(trace)trace(key_,x,threshold);
}
}
