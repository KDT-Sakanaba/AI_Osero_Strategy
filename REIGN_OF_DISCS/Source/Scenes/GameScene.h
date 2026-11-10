#pragma once
// The game screen: board, panels, mouse operation, strategy menus, pause menu.

#include <deque>
#include <future>
#include <vector>
#include "Core/SceneManager.h"
#include "Player/AIPlayer.h"
#include "Systems/GameSession.h"
#include "UI/BoardAnimator.h"
#include "UI/BoardRenderer.h"
#include "UI/GameUI.h"
#include "UI/HelpOverlay.h"

class GameScene : public SceneBase
{
public:
    GameScene(AppContext& context, const GameConfig& config);
    void OnEnter() override;
    void Update(double deltaSec) override;
    void Draw() const override;

private:
    enum class InputMode
    {
        Normal,
        AbilityMenu,
        AbilityTarget,
        BuildMenu,
        BuildTarget
    };

    struct MenuRow
    {
        Button button;
        std::string detail;   // cost / effect / remaining uses
        std::string reason;   // why it is unavailable (empty = available)
    };

    void ConsumeEvents();
    double AnimationScale() const;
    void UpdateAi(double deltaSec);
    void FinishAiTurnSafely();
    void HandleHumanInput();
    void HandleMenuInput();
    void HandleTargetInput();
    void UpdatePause();
    void OpenAbilityMenu();
    void OpenBuildMenu();
    void BackToNormal();
    bool IsHumanTurn() const;
    std::vector<Pos> CurrentTargets() const;
    std::string StatusLine() const;
    std::string HoverInfo() const;
    void DrawMenu(int mouseX, int mouseY, bool pressed) const;

    GameConfig config_;
    GameSession session_;
    GameUI ui_;
    BoardRenderer renderer_;
    BoardAnimator animator_;
    HelpOverlay help_;
    Pos hover_{ -1, -1 };
    bool paused_ = false;
    double endDelay_ = 0.0;

    InputMode mode_ = InputMode::Normal;
    AbilityType selectedAbility_ = AbilityType::MagicReinforce;
    FacilityType selectedFacility_ = FacilityType::None;
    std::vector<MenuRow> menu_;
    Button menuBack_;   // "戻る" inside the ability / build menu

    // AI (F003): thinking runs on another thread; actions are shown one at a time.
    std::future<std::vector<AiAction>> aiFuture_;
    bool aiThinking_ = false;
    std::deque<AiAction> aiQueue_;
    double aiTimer_ = 0.0;
    int aiTurnNumber_ = 0;
    std::uint32_t aiSeed_ = 0;
};
