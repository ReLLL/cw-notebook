// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_receiver.h"
#include <signal_path/signal_path.h>
#include <cstddef>
#include <type_traits>

struct RttyReceiver::Stream {
    dsp::stream<dsp::complex_t> iq;
    dsp::sink::Handler<dsp::complex_t> drain;
};
RttyReceiver::RttyReceiver()=default;
RttyReceiver::~RttyReceiver(){enabled(false);}
void RttyReceiver::enabled(bool enable){
    requested_=enable;sync();
}
void RttyReceiver::sync(){
    bool enable=requested_&&rate_>=8000&&rate_<=rtty::Skimmer::maxRate;
    if(enable==bool(stream_))return;
    if(enable){
        stream_=std::make_unique<Stream>();
        stream_->drain.init(&stream_->iq,input,this);
        // Start the drain before attaching to the host's synchronous splitter.
        stream_->drain.start();sigpath::iqFrontEnd.bindIQStream(&stream_->iq);
        engine_.configure(rate_,center_,playing_);
    }else{
        engine_.configure(rate_,center_,false);
        stream_->drain.stop();sigpath::iqFrontEnd.unbindIQStream(&stream_->iq);stream_.reset();
    }
}
void RttyReceiver::configure(double rate,double center,bool playing){
    // Stable GUI metadata polls must not contend with the raw input callback.
    if(rate==rate_&&center==center_&&playing==playing_)return;
    rate_=rate;center_=center;playing_=playing;
    sync();
    engine_.configure(rate,center,playing&&bool(stream_));
}
void RttyReceiver::input(dsp::complex_t* data,int count,void* context){
    // Public host types and std::complex<float> both store adjacent real/imag floats.
    static_assert(std::is_standard_layout<dsp::complex_t>::value);
    static_assert(offsetof(dsp::complex_t,im)==sizeof(float));
    static_assert(sizeof(dsp::complex_t)==sizeof(rtty::IQ));
    if(count>0)static_cast<RttyReceiver*>(context)->engine_.inputBytes(data,size_t(count));
}
