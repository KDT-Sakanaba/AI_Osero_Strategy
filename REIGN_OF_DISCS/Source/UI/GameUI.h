#pragma once
// Side panels, bottom bar (turn, side to move, message log, buttons) and the pause menu.

#include <array>
#include <deque>
#include <string>
#include "Systems/GameState.h"
#include "UI/DrawUtil.h"

class FontSet;

class GameUI
{
public:
    GameUI();

    void AddMessage(const std::string& text, bool isError = false);
    // Advances timers; resource numbers count up towards the real values (original 9.3).
    void Update(double deltaSec, const GameState& state);

    void DrawPanels(const GameState& state, const FontSet& fonts) const;
    void DrawBottom(const GameState& state, const FontSet& fonts, int mouseX, int mouseY, bool pressed,
        const std::string& statusLine, const std::string& hoverInfo) const;
    void DrawPauseMenu(const FontSet& fonts, int mouseX, int mouseY, bool pressed) const;

    Button& PassButton() { return pass_; }
    Button& AbilityButton() { return ability_; }
    Button& BuildButton() { return build_; }
    Button& HelpButton() { return help_; }
    Button& BackButton() { return back_; }
    // The bottom "戻る" button is shown only while a menu / target selection is open.
    void SetBackVisible(bool visible) const { backVisible_ = visible; }
    Button& PauseHelpButton() { return pauseHelp_; }
    Button& ResumeButton() { return resume_; }
    Button& ToTitleButton() { return toTitle_; }

    // Explanation for the side-panel row / bottom button under the mouse (empty = none).
    std::string Tooltip(const GameState& state, int mouseX, int mouseY) const;

    // Resource gain pop-up (F004 R 9.3): text floating near a side panel.
    void ShowGain(CellOwner side, const Resources& gained);

private:
    struct Message
    {
        std::string text;
        bool isError = false;
        double age = 0.0;
    };
    struct Gain
    {
        CellOwner side = CellOwner::None;
        std::string text;
        double age = 0.0;
    };

    void DrawSidePanel(const GameState& state, CellOwner side, int x, const FontSet& fonts) const;

    struct ShownResources
    {
        double mana = 0.0;
        double ore = 0.0;
        double influence = 0.0;
    };
    std::array<ShownResources, 2> shown_{};
    bool shownInitialized_ = false;

    std::deque<Message> log_;
    std::deque<Gain> gains_;
    Button pass_;
    Button ability_;
    Button build_;
    Button help_;
    Button back_;
    mutable bool backVisible_ = false;
    Button pauseHelp_;
    Button resume_;
    Button toTitle_;
};
