// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_skimmer.h"
#include <ctime>

namespace rtty {
std::string receptionReport(const Station& station,const Status& capture,bool compact,std::time_t now);
std::string analysisPrompt(const Station& station,const Status& capture,bool compact,std::time_t now,const std::string& grid="");
}
