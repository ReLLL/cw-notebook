// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <complex>
#include <vector>
#include <stdexcept>
#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif

namespace rtty {
using IQ=std::complex<float>;
// Plans and scratch are worker-owned; no allocation in transform().
class FFT {
public:
    explicit FFT(size_t n):n_(n),real_(n),imag_(n){
        if(n<2||(n&(n-1)))throw std::invalid_argument("FFT size must be a power of two");
        while((size_t(1)<<log_)<n)++log_;
#ifdef __APPLE__
        setup_=vDSP_create_fftsetup(log_,kFFTRadix2);
        if(!setup_)throw std::bad_alloc();
#else
        roots_.resize(n/2);
        for(size_t i=0;i<n/2;++i)roots_[i]=std::polar(1.f,float(-2*3.141592653589793*i/n));
#endif
    }
    ~FFT(){
#ifdef __APPLE__
        vDSP_destroy_fftsetup(setup_);
#endif
    }
    FFT(const FFT&)=delete;
    FFT& operator=(const FFT&)=delete;
    void transform(std::vector<IQ>& x,bool inverse=false){
#ifdef __APPLE__
        for(size_t i=0;i<n_;++i){real_[i]=x[i].real();imag_[i]=x[i].imag();}
        DSPSplitComplex split{real_.data(),imag_.data()};
        vDSP_fft_zip(setup_,&split,1,log_,inverse?FFT_INVERSE:FFT_FORWARD);
        const float scale=inverse?1.f/n_:1.f;
        for(size_t i=0;i<n_;++i)x[i]={real_[i]*scale,imag_[i]*scale};
#else
        for(size_t i=1,j=0;i<n_;++i){
            size_t bit=n_>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;
            if(i<j)std::swap(x[i],x[j]);
        }
        for(size_t len=2;len<=n_;len*=2)for(size_t i=0;i<n_;i+=len){
            for(size_t j=0;j<len/2;++j){
                auto w=roots_[j*n_/len];if(inverse)w=std::conj(w);
                auto a=x[i+j],b=x[i+j+len/2]*w;x[i+j]=a+b;x[i+j+len/2]=a-b;
            }
        }
        if(inverse)for(auto& v:x)v/=float(n_);
#endif
    }
private:
    size_t n_;unsigned log_=0;
    std::vector<float> real_,imag_;
#ifdef __APPLE__
    FFTSetup setup_=nullptr;
#else
    std::vector<IQ> roots_;
#endif
};
}
