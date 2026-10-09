// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc,char** argv){
    if(argc<3){std::cerr<<"Usage: cw_decode float32-mono.raw sample-rate [wpm|0] [tone|0] [recovery:1|0]\n";return 2;}
    std::ifstream in(argv[1],std::ios::binary);if(!in)return 2;
    cw::Decoder decoder;decoder.reset(argc>3?std::stof(argv[3]):0,argc>4?std::stof(argv[4]):0);
    decoder.recovery(argc<=5||std::string(argv[5])!="0");
    decoder.output=[](const std::string&s){std::cout<<s<<std::flush;};
    std::vector<float> buffer(4096);
    while(in.read(reinterpret_cast<char*>(buffer.data()),buffer.size()*sizeof(float)) || in.gcount()>0)
        decoder.process(buffer.data(),in.gcount()/sizeof(float),std::stoi(argv[2]));
    std::vector<float> silence(std::stoi(argv[2])*4);decoder.process(silence.data(),silence.size(),std::stoi(argv[2]));
    auto s=decoder.stats();std::cerr<<"\nTone "<<s.tone<<" Hz, "<<s.wpm<<" WPM\n";
}
