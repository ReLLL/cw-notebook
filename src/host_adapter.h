// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <string>
namespace cw::host {
#if CW_HOST_BROWN
inline constexpr bool brown=true;
inline constexpr const char* name="SDR++Brown";
#else
inline constexpr bool brown=false;
inline constexpr const char* name="SDR++";
#endif
std::filesystem::path logFolder();
std::string mode(const std::string& receiver);
void useCW(const std::string& receiver);
std::string grid(const std::string& instance);
bool setGrid(std::string value,const std::string& instance);
}
