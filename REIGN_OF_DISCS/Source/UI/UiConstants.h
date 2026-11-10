#pragma once
// Screen layout, colours and timing (no DxLib types here; colours are RGB triples).

namespace Ui
{
    constexpr int SCREEN_W = 1280;
    constexpr int SCREEN_H = 720;
    constexpr int COLOR_BIT = 32;
    constexpr const char* WINDOW_TITLE = "REIGN OF DISCS";

    // Board
    constexpr int CELL = 68;
    constexpr int BOARD_PIXELS = CELL * 8;
    constexpr int BOARD_X = (SCREEN_W - BOARD_PIXELS) / 2; // 368
    constexpr int BOARD_Y = 24;
    constexpr int STONE_RADIUS = 28;
    constexpr int HINT_RADIUS = 7;

    // Side panels
    constexpr int PANEL_Y = 24;
    constexpr int PANEL_H = 544;
    constexpr int LEFT_PANEL_X = 24;
    constexpr int RIGHT_PANEL_X = 936;
    constexpr int PANEL_W = 320;

    // Bottom area
    constexpr int BOTTOM_Y = 584;
    constexpr int BOTTOM_H = 120;
    constexpr int LOG_X = 368;
    constexpr int LOG_LINES = 3;
    constexpr int LOG_LINE_H = 26;

    // Timing (seconds)
    constexpr double MESSAGE_HIGHLIGHT_SEC = 3.0;
    constexpr double AI_MIN_THINK_SEC = 0.4;       // "thinking" is visible at least this long
    constexpr double AI_ACTION_INTERVAL_SEC = 0.5; // AI actions shown one by one (F003 spec 4)

    struct Rgb { int r; int g; int b; };
    constexpr Rgb BG            { 24, 28, 38 };
    constexpr Rgb PANEL_BG      { 36, 42, 56 };
    constexpr Rgb PANEL_FRAME   { 90, 100, 130 };
    constexpr Rgb BOARD_BG      { 30, 96, 60 };
    constexpr Rgb GRID          { 10, 30, 18 };
    constexpr Rgb BLACK_STONE   { 20, 20, 20 };
    constexpr Rgb WHITE_STONE   { 235, 235, 235 };
    constexpr Rgb STONE_EDGE    { 90, 90, 90 };
    constexpr Rgb HOVER_FRAME   { 255, 220, 60 };
    constexpr Rgb TEXT          { 230, 232, 240 };
    constexpr Rgb TEXT_DIM      { 140, 146, 160 };
    constexpr Rgb TEXT_ACCENT   { 255, 214, 102 };
    constexpr Rgb TEXT_ERROR    { 255, 120, 110 };
    constexpr Rgb BUTTON        { 64, 76, 110 };
    constexpr Rgb BUTTON_HOVER  { 92, 110, 160 };
    constexpr Rgb BUTTON_DISABLED { 50, 52, 60 };
    constexpr Rgb BUTTON_SELECTED { 150, 110, 40 };
    constexpr Rgb OVERLAY       { 0, 0, 0 };

    // Special squares (strategy)
    constexpr Rgb MANA_CELL     { 50, 80, 150 };
    constexpr Rgb ORE_CELL      { 120, 90, 60 };
    constexpr Rgb ALTAR_CELL    { 120, 60, 130 };
    constexpr Rgb BASE_CELL     { 150, 130, 40 };
    constexpr Rgb DEFENSE_MARK  { 80, 200, 255 };
    constexpr Rgb ENHANCE_MARK  { 255, 120, 220 };
    constexpr Rgb TARGET_MARK   { 255, 150, 60 };
}
