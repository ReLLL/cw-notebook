// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <string>
#include <vector>

// GUI-thread-only view. Holding a selection never pauses decoding or logging.
class TranscriptView {
public:
    void draw(const std::string& text, float height, float scale, bool& follow);
    void clear();
    void copyAll(const std::string& text);
    void copyReport(double frequency, float wpm, float carrierOffset, bool manualSpeed);
    bool popupOpen() const { return popupOpen_; }
private:
    static int selectionCallback(ImGuiInputTextCallbackData* data);
    void update(const std::string& text, float width);
    std::string source_, selection_;
    std::vector<char> buffer_{'\0'};
    float width_=0,fontSize_=0;
    bool wasFollowing_=true;
    bool popupOpen_=false;
    std::string copyNotice_;
    double copiedAt_=-10;
    void copied(const std::string& text, const char* kind);
};
