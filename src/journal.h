// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <string>
#include <ctime>

namespace cw {
std::string utc(const char* format = "%Y-%m-%d %H:%M:%S");
class Journal {
public:
    explicit Journal(std::filesystem::path folder);
    ~Journal();
    void create(double frequency);
    void resume(const std::string& filename);
    void append(const std::string& text, double frequency, float wpm);
    void rename(const std::string& filename);
    std::filesystem::path clear();
    void sync();
    std::filesystem::path path() const { return path_; }
    static std::string validName(const std::string& filename);
private:
    void open(bool exclusive);
    void write(const std::string& text);
    void close();
    std::filesystem::path folder_, path_;
    int fd_ = -1;
    double frequency_ = -1;
    std::time_t lastWrite_ = 0;
    size_t column_ = 0;
};
}
