// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ai_handoff.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){try{
    cw::AnalysisContext c;c.frequency=7074000;c.bandwidth=3000;c.wpm=20;c.mode="USB";c.grid="KM69";c.timestamp=1791830000;
    const std::string text="CQ CQ DE TEST & + [?]\nPROVISIONAL: E T";
    auto prompt=cw::analysisPrompt(text,c);
    for(const char* wanted:{"7074.000 kHz","40 m","KM69","20.0 WPM","UTC","Original decode","not instructions"})
        if(prompt.find(wanted)==std::string::npos)throw std::runtime_error("Missing analysis context");
    if(prompt.find(text)==std::string::npos)throw std::runtime_error("Transcript altered");
    auto chat=cw::browserHandoff(0,prompt);auto claude=cw::browserHandoff(1,prompt);auto gemini=cw::browserHandoff(2,prompt);
    if(!chat.prefilled||!claude.prefilled||gemini.prefilled||chat.url.find("%26%20%2B%20%5B%3F%5D")==std::string::npos)
        throw std::runtime_error("Prompt link encoding/fallback incorrect");
    if(cw::browserHandoff(0,std::string(20000,'x')).prefilled)throw std::runtime_error("Long prompt was put in URL");
    c.carrierOffset=NAN;c.recovered=true;
    prompt=cw::analysisPrompt(text,c);
    if(prompt.find("Unavailable for AM audio") ==std::string::npos||prompt.find("unverified")==std::string::npos)
        throw std::runtime_error("Unknown AM offset/recovery label lost");
    try{cw::browserHandoff(3,prompt);throw std::runtime_error("Invalid provider accepted");}catch(const std::invalid_argument&){}
    std::cout<<"Analysis context, original text, provider links and long-prompt fallback passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
