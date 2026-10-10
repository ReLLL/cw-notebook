// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdio>
#include <deque>
#include <stdexcept>
#include <vector>

namespace cw {
struct KeyRun { bool key; double seconds; };

// Exact measured durations, not guessed dots/dashes. Long ambiguous receptions
// spill to an anonymous temporary file on the decoding worker, never the UI.
class RunBuffer {
public:
    RunBuffer() = default;
    RunBuffer(const RunBuffer&) = delete;
    RunBuffer& operator=(const RunBuffer&) = delete;
    ~RunBuffer(){clear();}
    size_t size() const {return count_;}
    const std::deque<KeyRun>& recent() const {return recent_;}
    void clear(){if(file_)std::fclose(file_);file_=nullptr;runs_.clear();recent_.clear();count_=0;}
    void push_back(KeyRun run){
        if(!file_&&runs_.size()>=4096){
            auto* candidate=std::tmpfile();
            if(!candidate)throw std::runtime_error("Cannot create CW learning buffer; check temporary disk space");
            if(std::fwrite(runs_.data(),sizeof(KeyRun),runs_.size(),candidate)!=runs_.size()){
                std::fclose(candidate);throw std::runtime_error("Cannot preserve CW learning buffer; check disk space");
            }
            file_=candidate;runs_.clear();
        }
        if(file_){
            if(std::fwrite(&run,sizeof(run),1,file_)!=1)throw std::runtime_error("Cannot append CW learning buffer; check disk space");
        }else runs_.push_back(run);
        ++count_;
        recent_.push_back(run);if(recent_.size()>512)recent_.pop_front();
    }
    template<class Visitor> void each(Visitor visit){
        if(!file_){for(const auto& run:runs_)visit(run);return;}
        if(std::fflush(file_)!=0||std::fseek(file_,0,SEEK_SET)!=0)
            throw std::runtime_error("Cannot rewind CW learning buffer");
        KeyRun block[256];size_t remaining=count_;
        while(remaining){
            size_t n=remaining<256?remaining:256;
            if(std::fread(block,sizeof(KeyRun),n,file_)!=n)throw std::runtime_error("Cannot read CW learning buffer");
            for(size_t i=0;i<n;++i)visit(block[i]);
            remaining-=n;
        }
    }
private:
    std::vector<KeyRun> runs_;
    std::deque<KeyRun> recent_;
    std::FILE* file_=nullptr;
    size_t count_=0;
};
}
