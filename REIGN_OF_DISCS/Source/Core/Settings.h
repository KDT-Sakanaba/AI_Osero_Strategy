#pragma once
// User settings (F004 spec 6). Kept only while the program runs (not saved).

struct Settings
{
    static constexpr int VOLUME_STEP = 10;
    static constexpr int VOLUME_MAX = 100;

    int bgmVolume = 70;          // 0..100 %
    int seVolume = 80;           // 0..100 %
    bool fastAnimation = false;  // "速い": animations take half the time

    double AnimationScale() const { return fastAnimation ? 0.5 : 1.0; }
};
