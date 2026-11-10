#include "Scenes/GameScene.h"
#include <memory>
#include "Core/Settings.h"
#include "DxLib.h"
#include "Input/InputManager.h"
#include "Scenes/ResultScene.h"
#include "Scenes/TitleScene.h"
#include "Systems/AbilityManager.h"
#include "Systems/FacilityManager.h"
#include "Systems/ResourceManager.h"
#include "UI/FontSet.h"

namespace
{
    constexpr double RESULT_DELAY_SEC = 1.2; // let the last move be seen before the result screen

    constexpr int MENU_W = 800;
    constexpr int MENU_X = (Ui::SCREEN_W - MENU_W) / 2;
    constexpr int MENU_Y = Ui::BOARD_Y + 24;
    constexpr int MENU_ROW_H = 76;
    constexpr int MENU_BUTTON_W = 190;

    std::string CostText(const Resources& cost)
    {
        std::string text;
        if (cost.mana > 0) text += "マナ" + std::to_string(cost.mana) + " ";
        if (cost.ore > 0) text += "鉱石" + std::to_string(cost.ore) + " ";
        if (cost.influence > 0) text += "影響力" + std::to_string(cost.influence);
        return text.empty() ? "コストなし" : text;
    }

    std::string OwnerText(CellOwner owner, const GameState& state)
    {
        return owner == CellOwner::None ? "なし" : state.Side(owner).name;
    }
}

GameScene::GameScene(AppContext& context, const GameConfig& config)
    : SceneBase(context), config_(config)
{
    menuBack_ = Button{ MENU_X + MENU_W - 136, MENU_Y + 10, 120, 40, "戻る" };
}

void GameScene::OnEnter()
{
    PlayBgm(BgmId::Game);
    if (config_.layout == MapLayout::Random)
    {
        // A new random map for every game, including "もう一度".
        config_.mapSeed = static_cast<std::uint32_t>(GetNowHiPerformanceCount()) | 1u;
    }
    session_.Start(config_);
    ConsumeEvents();
}

double GameScene::AnimationScale() const
{
    return ctx_.settings != nullptr ? ctx_.settings->AnimationScale() : 1.0;
}

bool GameScene::IsHumanTurn() const
{
    const GameState& s = session_.State();
    if (s.over)
    {
        return false;
    }
    return !(IsAiOpponent(config_.opponent) && s.turn.Current() == CellOwner::Enemy);
}

void GameScene::ConsumeEvents()
{
    for (const GameEvent& e : session_.TakeEvents())
    {
        switch (e.type)
        {
        case GameEventType::Rejected:
            ui_.AddMessage(e.message, true);
            PlaySe(SoundId::Error);
            break;
        case GameEventType::ResourceGained:
            ui_.AddMessage(e.message);
            ui_.ShowGain(e.actor, e.gained);
            PlaySe(SoundId::Resource);
            break;
        case GameEventType::Placed:
            ui_.AddMessage(e.message);
            animator_.OnPlaced(e.pos, e.actor, e.flips, AnimationScale());
            PlaySe(SoundId::Place);
            if (!e.flips.empty())
            {
                PlaySe(SoundId::Flip);
            }
            break;
        case GameEventType::AbilityUsed:
            ui_.AddMessage(e.message);
            if (e.ability == AbilityType::ForcedFlip && !e.flips.empty())
            {
                animator_.OnFlipped(e.flips.front(), e.actor, AnimationScale());
                PlaySe(SoundId::Flip);
            }
            else if (e.ability == AbilityType::TacticalPlacement && !e.flips.empty())
            {
                animator_.OnAppeared(e.flips.front(), e.actor, AnimationScale());
                PlaySe(SoundId::Place);
            }
            else
            {
                PlaySe(SoundId::Button);
            }
            if (!e.gained.IsZero())
            {
                ui_.ShowGain(e.actor, e.gained);
                PlaySe(SoundId::Resource);
            }
            break;
        case GameEventType::Built:
            ui_.AddMessage(e.message);
            PlaySe(SoundId::Button);
            break;
        default:
            ui_.AddMessage(e.message);
            break;
        }
    }
}

void GameScene::Update(double deltaSec)
{
    ui_.Update(deltaSec, session_.State());
    if (!paused_)
    {
        animator_.Update(deltaSec);
    }
    const InputManager& in = *ctx_.input;

    // Help screen has priority over everything (the game waits while it is open).
    if (help_.IsOpen())
    {
        if (help_.Update(in)) PlaySe(SoundId::Button);
        return;
    }
    if (in.KeyPressed(KEY_INPUT_H) ||
        (!paused_ && ui_.HelpButton().Clicked(in.MouseX(), in.MouseY(), in.LeftClicked())))
    {
        PlaySe(SoundId::Button);
        help_.Open(0);
        return;
    }

    if (in.KeyPressed(KEY_INPUT_ESCAPE))
    {
        if (mode_ != InputMode::Normal && !paused_)
        {
            BackToNormal();
        }
        else
        {
            paused_ = !paused_;
        }
        return;
    }
    if (paused_)
    {
        UpdatePause();
        return;
    }

    if (animator_.Busy())
    {
        return; // no board operation and no AI action while stones are animating (original 9.2)
    }

    if (session_.IsOver())
    {
        mode_ = InputMode::Normal;
        endDelay_ += deltaSec;
        if (endDelay_ >= RESULT_DELAY_SEC)
        {
            ctx_.scenes->Change(std::make_unique<ResultScene>(ctx_, config_, session_.State()));
        }
        return;
    }

    hover_ = Pos{ -1, -1 };
    BoardRenderer::ScreenToCell(in.MouseX(), in.MouseY(), hover_);
    if (IsHumanTurn())
    {
        HandleHumanInput();
    }
    else
    {
        ui_.PassButton().enabled = false;
        ui_.AbilityButton().enabled = false;
        ui_.BuildButton().enabled = false;
        UpdateAi(deltaSec);
    }
    ConsumeEvents();
}

void GameScene::UpdateAi(double deltaSec)
{
    const GameState& state = session_.State();
    aiTimer_ += deltaSec;

    if (!aiThinking_ && aiQueue_.empty())
    {
        // New AI turn: think on a copy of the state (the window keeps running, F003 R-08).
        AiSettings settings;
        settings.level = config_.opponent;
        settings.seed = static_cast<std::uint32_t>(GetNowCount()) + (++aiSeed_);
        const GameState snapshot = state;
        aiFuture_ = std::async(std::launch::async, [snapshot, settings]() {
            return AIPlayer::DecideTurn(snapshot, settings);
        });
        aiThinking_ = true;
        aiTimer_ = 0.0;
        aiTurnNumber_ = state.turn.TurnNumber();
        return;
    }

    if (aiThinking_)
    {
        if (aiTimer_ < Ui::AI_MIN_THINK_SEC ||
            aiFuture_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            return;
        }
        const std::vector<AiAction> plan = aiFuture_.get();
        aiQueue_.assign(plan.begin(), plan.end());
        aiThinking_ = false;
        aiTimer_ = Ui::AI_ACTION_INTERVAL_SEC; // first action right away
        if (aiQueue_.empty())
        {
            FinishAiTurnSafely();
            return;
        }
    }

    if (aiTimer_ < Ui::AI_ACTION_INTERVAL_SEC)
    {
        return;
    }
    aiTimer_ = 0.0;

    const AiAction action = aiQueue_.front();
    aiQueue_.pop_front();
    bool ok = false;
    switch (action.kind)
    {
    case AiAction::Kind::UseAbility: ok = session_.TryUseAbility(action.ability, action.pos); break;
    case AiAction::Kind::Build:      ok = session_.TryBuild(action.facility, action.pos); break;
    case AiAction::Kind::Place:      ok = session_.TryPlace(action.pos); break;
    case AiAction::Kind::Pass:       ok = session_.TryPass(); break;
    }

    const bool turnEnded = session_.IsOver() || session_.State().turn.TurnNumber() != aiTurnNumber_;
    if (!ok || (aiQueue_.empty() && !turnEnded))
    {
        FinishAiTurnSafely();
    }
    if (session_.IsOver() || session_.State().turn.TurnNumber() != aiTurnNumber_)
    {
        aiQueue_.clear();
    }
}

void GameScene::FinishAiTurnSafely()
{
    // A refused or incomplete plan must never stall the game: place the first legal move or pass.
    aiQueue_.clear();
    if (session_.IsOver() || session_.State().turn.TurnNumber() != aiTurnNumber_)
    {
        return;
    }
    const std::vector<Pos> moves = GameRules::LegalMoves(session_.State());
    if (!moves.empty())
    {
        session_.TryPlace(moves.front());
    }
    else
    {
        session_.TryPass();
    }
}

void GameScene::HandleHumanInput()
{
    switch (mode_)
    {
    case InputMode::AbilityMenu:
    case InputMode::BuildMenu:
        HandleMenuInput();
        return;
    case InputMode::AbilityTarget:
    case InputMode::BuildTarget:
        HandleTargetInput();
        return;
    default:
        break;
    }

    const InputManager& in = *ctx_.input;
    const GameState& state = session_.State();
    const int mx = in.MouseX();
    const int my = in.MouseY();
    const bool click = in.LeftClicked();

    Button& pass = ui_.PassButton();
    pass.enabled = GameRules::CanPass(state);
    Button& ability = ui_.AbilityButton();
    ability.enabled = state.IsStrategy() && !state.actions.abilityUsed;
    Button& build = ui_.BuildButton();
    build.enabled = state.IsStrategy() && !state.actions.built;

    if (pass.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        session_.TryPass();
        return;
    }
    if (state.IsStrategy() && ability.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        OpenAbilityMenu();
        return;
    }
    if (state.IsStrategy() && build.Clicked(mx, my, click))
    {
        PlaySe(SoundId::Button);
        OpenBuildMenu();
        return;
    }
    if (click && Board::IsInside(hover_))
    {
        session_.TryPlace(hover_);
    }
}

void GameScene::OpenAbilityMenu()
{
    const GameState& state = session_.State();
    menu_.clear();
    int i = 0;
    for (const AbilityInfo& info : AbilityManager::All())
    {
        MenuRow row;
        row.button = Button{ MENU_X + 16, MENU_Y + 60 + i * MENU_ROW_H, MENU_BUTTON_W, 52, info.name };
        row.detail = CostText(info.cost);
        const int remaining = AbilityManager::RemainingUses(state.Current(), info.type);
        if (remaining >= 0)
        {
            row.detail += "  残り" + std::to_string(remaining) + "回";
        }
        row.detail += "  " + std::string(info.effect);
        row.reason = AbilityManager::CheckUse(state, info.type);
        row.button.enabled = row.reason.empty();
        menu_.push_back(row);
        ++i;
    }
    mode_ = InputMode::AbilityMenu;
}

void GameScene::OpenBuildMenu()
{
    const GameState& state = session_.State();
    menu_.clear();
    int i = 0;
    for (const FacilityInfo& info : FacilityManager::All())
    {
        MenuRow row;
        row.button = Button{ MENU_X + 16, MENU_Y + 60 + i * MENU_ROW_H, MENU_BUTTON_W, 52, info.name };
        row.detail = CostText(info.cost) + "  " + info.effect;
        row.reason = FacilityManager::CheckTurn(state);
        if (row.reason.empty())
        {
            row.reason = ResourceManager::Shortage(state.Current(), info.cost);
        }
        if (row.reason.empty() && FacilityManager::BuildableCells(state, info.type).empty())
        {
            row.reason = "建設できるマスがありません";
        }
        row.button.enabled = row.reason.empty();
        menu_.push_back(row);
        ++i;
    }
    mode_ = InputMode::BuildMenu;
}

void GameScene::HandleMenuInput()
{
    const InputManager& in = *ctx_.input;
    const bool backClicked = ui_.BackButton().Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()) ||
        menuBack_.Clicked(in.MouseX(), in.MouseY(), in.LeftClicked());
    if (in.RightClicked() || backClicked)
    {
        if (backClicked) PlaySe(SoundId::Button);
        BackToNormal();
        return;
    }
    const bool abilities = (mode_ == InputMode::AbilityMenu);
    for (size_t i = 0; i < menu_.size(); ++i)
    {
        if (!menu_[i].button.Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()))
        {
            continue;
        }
        PlaySe(SoundId::Button);
        if (abilities)
        {
            selectedAbility_ = AbilityManager::All()[i].type;
            if (!AbilityManager::Info(selectedAbility_).needsTarget)
            {
                session_.TryUseAbility(selectedAbility_, Pos{ -1, -1 });
                BackToNormal();
                return;
            }
            mode_ = InputMode::AbilityTarget;
        }
        else
        {
            selectedFacility_ = FacilityManager::All()[i].type;
            mode_ = InputMode::BuildTarget;
        }
        return;
    }
}

void GameScene::HandleTargetInput()
{
    const InputManager& in = *ctx_.input;
    const bool backClicked = ui_.BackButton().Clicked(in.MouseX(), in.MouseY(), in.LeftClicked());
    if (in.RightClicked() || backClicked)
    {
        if (backClicked) PlaySe(SoundId::Button);
        // Back one step: to the menu the target mode came from.
        if (mode_ == InputMode::AbilityTarget) OpenAbilityMenu(); else OpenBuildMenu();
        return;
    }
    if (!in.LeftClicked() || !Board::IsInside(hover_))
    {
        return;
    }
    const bool ok = (mode_ == InputMode::AbilityTarget)
        ? session_.TryUseAbility(selectedAbility_, hover_)
        : session_.TryBuild(selectedFacility_, hover_);
    if (ok)
    {
        BackToNormal();
    }
}

void GameScene::BackToNormal()
{
    mode_ = InputMode::Normal;
    menu_.clear();
}

void GameScene::UpdatePause()
{
    const InputManager& in = *ctx_.input;
    if (ui_.ResumeButton().Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()) || in.RightClicked())
    {
        paused_ = false;
    }
    else if (ui_.PauseHelpButton().Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()))
    {
        PlaySe(SoundId::Button);
        paused_ = false;
        help_.Open(0);
    }
    else if (ui_.ToTitleButton().Clicked(in.MouseX(), in.MouseY(), in.LeftClicked()))
    {
        PlaySe(SoundId::Button);
        ctx_.scenes->Change(std::make_unique<TitleScene>(ctx_));
    }
}

std::vector<Pos> GameScene::CurrentTargets() const
{
    const GameState& s = session_.State();
    if (mode_ == InputMode::AbilityTarget)
    {
        return AbilityManager::ValidTargets(s, selectedAbility_);
    }
    if (mode_ == InputMode::BuildTarget)
    {
        return FacilityManager::BuildableCells(s, selectedFacility_);
    }
    return {};
}

std::string GameScene::StatusLine() const
{
    switch (mode_)
    {
    case InputMode::AbilityMenu:
    case InputMode::BuildMenu:
        return "使うものを選んでください（「戻る」・右クリック・ESC で閉じる）";
    case InputMode::AbilityTarget:
        return std::string("「") + AbilityManager::Info(selectedAbility_).name + "」の対象マスを選んでください（「戻る」・右クリックで一覧へ）";
    case InputMode::BuildTarget:
        return std::string("「") + FacilityManager::Info(selectedFacility_).name + "」を建てるマスを選んでください（「戻る」・右クリックで一覧へ）";
    default:
        break;
    }
    const GameState& s = session_.State();
    if (!s.over && !IsHumanTurn())
    {
        return "敵AIが考えています…";
    }
    if (!s.over && IsHumanTurn() && GameRules::CanPass(s))
    {
        return "置ける場所がありません。建設・能力を使うか、パスしてください";
    }
    if (!s.over && IsHumanTurn())
    {
        // What to do now (always visible guide).
        if (s.IsStrategy())
        {
            return "石を置くとターン終了。その前に「能力」「建設」を各1回使えます（H：ヘルプ）";
        }
        return "点のあるマスをクリックして石を置いてください（H：ヘルプ）";
    }
    return "";
}

std::string GameScene::HoverInfo() const
{
    if (!Board::IsInside(hover_))
    {
        return "";
    }
    const GameState& s = session_.State();
    if (!s.IsStrategy())
    {
        return "";
    }
    const Cell& c = s.board.GetCell(hover_);
    std::string info = std::string(CellTypeName(c.type)) + " / 所有: " + OwnerText(c.owner, s);
    if (c.HasFacility())
    {
        info += " / " + std::string(FacilityName(c.facility)) + (c.IsFacilityDisabled() ? "（無効化中）" : "");
    }
    if (c.isDefended) info += " / 防御中";
    if (c.IsEnhanced()) info += " / 強化中";
    const Resources inc = ResourceManager::CellIncome(c);
    if (!inc.IsZero())
    {
        info += " / 収入:";
        if (inc.mana) info += " マナ+" + std::to_string(inc.mana);
        if (inc.ore) info += " 鉱石+" + std::to_string(inc.ore);
        if (inc.influence) info += " 影響力+" + std::to_string(inc.influence);
    }
    return info;
}

void GameScene::DrawMenu(int mouseX, int mouseY, bool pressed) const
{
    const FontSet& f = *ctx_.fonts;
    const int h = 70 + static_cast<int>(menu_.size()) * MENU_ROW_H;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 235);
    DrawUtil::FillRect(MENU_X, MENU_Y, MENU_W, h, Ui::PANEL_BG);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawUtil::FrameRect(MENU_X, MENU_Y, MENU_W, h, Ui::HOVER_FRAME, 2);
    const char* title = (mode_ == InputMode::AbilityMenu) ? "特殊能力（1ターンに1回）" : "施設建設（1ターンに1回）";
    DrawUtil::Text(MENU_X + 16, MENU_Y + 16, title, Ui::TEXT_ACCENT, f.Medium());
    menuBack_.Draw(f.Medium(), mouseX, mouseY, pressed);
    for (const MenuRow& row : menu_)
    {
        row.button.Draw(f.Medium(), mouseX, mouseY, pressed);
        const int tx = row.button.x + MENU_BUTTON_W + 12;
        DrawUtil::Text(tx, row.button.y + 2, row.detail, Ui::TEXT, f.Small());
        if (!row.reason.empty())
        {
            DrawUtil::Text(tx, row.button.y + 28, row.reason, Ui::TEXT_ERROR, f.Small());
        }
    }
}

void GameScene::Draw() const
{
    const FontSet& f = *ctx_.fonts;
    const InputManager& in = *ctx_.input;
    const GameState& state = session_.State();
    const bool pressed = in.LeftDown();

    DrawUtil::FillRect(0, 0, Ui::SCREEN_W, Ui::SCREEN_H, Ui::BG);

    BoardView view;
    view.hover = (paused_ || mode_ == InputMode::AbilityMenu || mode_ == InputMode::BuildMenu) ? Pos{ -1, -1 } : hover_;
    view.showSpecialCells = state.IsStrategy();
    if (mode_ == InputMode::AbilityTarget || mode_ == InputMode::BuildTarget)
    {
        view.targets = CurrentTargets();
    }
    else if (!state.over && IsHumanTurn() && !animator_.Busy())
    {
        view.hints = GameRules::LegalMoves(state);
        view.hintOwner = state.turn.Current();
    }
    animator_.FillView(view);
    renderer_.Draw(state.board, view, f.Small());

    ui_.DrawPanels(state, f);
    const std::string coords = Board::IsInside(view.hover)
        ? "マス (" + std::to_string(view.hover.x) + ", " + std::to_string(view.hover.y) + ")" : "";
    ui_.SetBackVisible(mode_ != InputMode::Normal);
    ui_.DrawBottom(state, f, in.MouseX(), in.MouseY(), pressed, StatusLine(), coords);

    if (mode_ == InputMode::AbilityMenu || mode_ == InputMode::BuildMenu)
    {
        DrawMenu(in.MouseX(), in.MouseY(), pressed);
    }
    else if (!paused_)
    {
        // Tooltip: square info on the board (original 8.4), otherwise panel rows / buttons.
        const std::string info = Board::IsInside(view.hover) ? HoverInfo() : ui_.Tooltip(state, in.MouseX(), in.MouseY());
        if (!info.empty())
        {
            const int w = DrawUtil::TextWidth(info, f.Small()) + 16;
            int tx = in.MouseX() + 18;
            int ty = in.MouseY() + 18;
            if (tx + w > Ui::SCREEN_W - 8) tx = Ui::SCREEN_W - 8 - w;
            if (ty + 28 > Ui::SCREEN_H - 8) ty = in.MouseY() - 40;
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
            DrawUtil::FillRect(tx, ty, w, 28, Ui::PANEL_BG);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawUtil::FrameRect(tx, ty, w, 28, Ui::PANEL_FRAME);
            DrawUtil::Text(tx + 8, ty + 5, info, Ui::TEXT, f.Small());
        }
    }
    if (paused_)
    {
        ui_.DrawPauseMenu(f, in.MouseX(), in.MouseY(), pressed);
    }
    help_.Draw(f, in.MouseX(), in.MouseY(), pressed);
}
