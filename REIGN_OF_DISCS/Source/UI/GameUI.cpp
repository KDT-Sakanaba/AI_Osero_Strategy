#include "UI/GameUI.h"
#include <algorithm>
#include "DxLib.h"
#include "UI/BoardRenderer.h"
#include "UI/FontSet.h"
#include "UI/UiConstants.h"

namespace
{
    constexpr double GAIN_POPUP_SEC = 1.6;
    constexpr double COUNT_UP_SEC_PER_UNIT = 0.06;
    constexpr int PANEL_ROWS_Y = 16 + 56 + 48 - 6; // first resource row (matches DrawSidePanel)
    constexpr int PANEL_ROW_H = 34;

    int CountOwned(const Board& board, CellOwner side, bool facilities)
    {
        int count = 0;
        for (int y = 0; y < Board::SIZE; ++y)
        {
            for (int x = 0; x < Board::SIZE; ++x)
            {
                const Cell& c = board.GetCell(x, y);
                if (c.owner != side) continue;
                if (facilities ? c.HasFacility() : c.type == CellType::NeutralBase) ++count;
            }
        }
        return count;
    }

    std::string Gauge(int value, int max)
    {
        return std::to_string(value) + " / " + std::to_string(max);
    }
}

GameUI::GameUI()
{
    pass_ = Button{ 1104, Ui::BOTTOM_Y + 10, 136, 48, "パス" };
    ability_ = Button{ 1104, Ui::BOTTOM_Y + 64, 64, 44, "能力" };
    build_ = Button{ 1176, Ui::BOTTOM_Y + 64, 64, 44, "建設" };
    help_ = Button{ 1008, Ui::BOTTOM_Y + 10, 88, 48, "ヘルプ" };
    back_ = Button{ 1008, Ui::BOTTOM_Y + 64, 88, 44, "戻る" };
    resume_ = Button{ (Ui::SCREEN_W - 260) / 2, 300, 260, 56, "続ける" };
    pauseHelp_ = Button{ (Ui::SCREEN_W - 260) / 2, 376, 260, 56, "ヘルプ（遊び方）" };
    toTitle_ = Button{ (Ui::SCREEN_W - 260) / 2, 452, 260, 56, "タイトルへ戻る" };
}

void GameUI::AddMessage(const std::string& text, bool isError)
{
    log_.push_front(Message{ text, isError, 0.0 });
    while (log_.size() > Ui::LOG_LINES)
    {
        log_.pop_back();
    }
}

void GameUI::ShowGain(CellOwner side, const Resources& gained)
{
    std::string text;
    if (gained.mana > 0) text += "マナ+" + std::to_string(gained.mana) + " ";
    if (gained.ore > 0) text += "鉱石+" + std::to_string(gained.ore) + " ";
    if (gained.influence > 0) text += "影響力+" + std::to_string(gained.influence);
    if (!text.empty())
    {
        gains_.push_back(Gain{ side, text, 0.0 });
    }
}

void GameUI::Update(double deltaSec, const GameState& state)
{
    auto approach = [deltaSec](double& shown, int actual) {
        if (shown > actual)
        {
            shown = actual; // spending is shown at once
        }
        else if (shown < actual)
        {
            shown = std::min<double>(actual, shown + deltaSec / COUNT_UP_SEC_PER_UNIT);
        }
    };
    for (int i = 0; i < 2; ++i)
    {
        const Resources& r = state.players[i].resources;
        if (!shownInitialized_)
        {
            shown_[i] = ShownResources{ static_cast<double>(r.mana), static_cast<double>(r.ore), static_cast<double>(r.influence) };
            continue;
        }
        approach(shown_[i].mana, r.mana);
        approach(shown_[i].ore, r.ore);
        approach(shown_[i].influence, r.influence);
    }
    shownInitialized_ = true;

    for (auto& m : log_) m.age += deltaSec;
    for (auto& g : gains_) g.age += deltaSec;
    while (!gains_.empty() && gains_.front().age > GAIN_POPUP_SEC)
    {
        gains_.pop_front();
    }
}

void GameUI::DrawSidePanel(const GameState& state, CellOwner side, int x, const FontSet& fonts) const
{
    const bool toMove = !state.over && state.turn.Current() == side;
    DrawUtil::Panel(x, Ui::PANEL_Y, Ui::PANEL_W, Ui::PANEL_H);
    if (toMove)
    {
        DrawUtil::FrameRect(x, Ui::PANEL_Y, Ui::PANEL_W, Ui::PANEL_H, Ui::HOVER_FRAME, 3);
    }
    const Player& p = state.Side(side);
    int y = Ui::PANEL_Y + 16;
    BoardRenderer::DrawStone(x + 36, y + 20, side, 1.0, 0.7);
    DrawUtil::Text(x + 70, y + 6, p.name, Ui::TEXT, fonts.Medium());
    y += 56;
    DrawUtil::Text(x + 20, y, "石の数", Ui::TEXT_DIM, fonts.Medium());
    DrawUtil::Text(x + 180, y - 6, std::to_string(state.board.CountStones(side)), Ui::TEXT_ACCENT, fonts.Large());
    y += 48;

    if (state.IsStrategy())
    {
        const ShownResources& sr = shown_[SideIndex(side)];
        const Resources r{ static_cast<int>(sr.mana), static_cast<int>(sr.ore), static_cast<int>(sr.influence) };
        DrawUtil::Text(x + 20, y, "マナ", Ui::TEXT_DIM, fonts.Medium());
        DrawUtil::Text(x + 180, y, Gauge(r.mana, Const::MANA_MAX), Ui::TEXT, fonts.Medium());
        y += 34;
        DrawUtil::Text(x + 20, y, "鉱石", Ui::TEXT_DIM, fonts.Medium());
        DrawUtil::Text(x + 180, y, Gauge(r.ore, Const::ORE_MAX), Ui::TEXT, fonts.Medium());
        y += 34;
        DrawUtil::Text(x + 20, y, "影響力", Ui::TEXT_DIM, fonts.Medium());
        DrawUtil::Text(x + 180, y, Gauge(r.influence, Const::INFLUENCE_MAX), Ui::TEXT, fonts.Medium());
        y += 34;
        DrawUtil::Text(x + 20, y, "所有拠点数", Ui::TEXT_DIM, fonts.Medium());
        DrawUtil::Text(x + 180, y, std::to_string(CountOwned(state.board, side, false)), Ui::TEXT, fonts.Medium());
        y += 34;
        DrawUtil::Text(x + 20, y, "所有施設数", Ui::TEXT_DIM, fonts.Medium());
        DrawUtil::Text(x + 180, y, std::to_string(CountOwned(state.board, side, true)), Ui::TEXT, fonts.Medium());
        y += 34;
        DrawUtil::Text(x + 20, y, "資源採掘 残り", Ui::TEXT_DIM, fonts.Small());
        DrawUtil::Text(x + 180, y, std::to_string(Const::MINING_ABILITY_LIMIT - p.abilityUses[AbilityIndex(AbilityType::ResourceMining)]) + " 回", Ui::TEXT, fonts.Small());
        y += 26;
        DrawUtil::Text(x + 20, y, "戦術的配置 残り", Ui::TEXT_DIM, fonts.Small());
        DrawUtil::Text(x + 180, y, std::to_string(Const::TACTICAL_PLACEMENT_LIMIT - p.abilityUses[AbilityIndex(AbilityType::TacticalPlacement)]) + " 回", Ui::TEXT, fonts.Small());
        y += 26;
        if (toMove)
        {
            DrawUtil::Text(x + 20, y + 8, state.actions.abilityUsed ? "能力: 使用済み" : "能力: 使用可", Ui::TEXT_DIM, fonts.Small());
            DrawUtil::Text(x + 170, y + 8, state.actions.built ? "建設: 使用済み" : "建設: 使用可", Ui::TEXT_DIM, fonts.Small());
        }
    }

    for (const auto& g : gains_)
    {
        if (g.side != side) continue;
        const int rise = static_cast<int>(g.age * 30.0);
        const int alpha = static_cast<int>(255.0 * (1.0 - g.age / GAIN_POPUP_SEC));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha < 0 ? 0 : alpha);
        DrawUtil::Text(x + 20, Ui::PANEL_Y + Ui::PANEL_H - 40 - rise, g.text, Ui::TEXT_ACCENT, fonts.Medium());
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

std::string GameUI::Tooltip(const GameState& state, int mx, int my) const
{
    if (backVisible_ && back_.Contains(mx, my)) return "1つ前の画面に戻ります（右クリックと同じ）";
    if (help_.Contains(mx, my)) return "ルール・記号・資源の使い道を表示します（H キー）";
    if (pass_.Contains(mx, my)) return pass_.enabled ? "このターンを終えます" : "置ける場所がある間はパスできません";
    if (state.IsStrategy())
    {
        if (ability_.Contains(mx, my)) return "特殊能力を使います（1ターンに1回）";
        if (build_.Contains(mx, my)) return "自分の石のマスに施設を建てます（1ターンに1回）";
        for (int panelX : { Ui::LEFT_PANEL_X, Ui::RIGHT_PANEL_X })
        {
            if (mx < panelX || mx >= panelX + Ui::PANEL_W) continue;
            const int row = (my - (Ui::PANEL_Y + PANEL_ROWS_Y)) / PANEL_ROW_H;
            if (my < Ui::PANEL_Y + PANEL_ROWS_Y || row > 4) continue;
            switch (row)
            {
            case 0: return "マナ：魔力強化（3）・強制反転（5）・魔力炉（2）に使う";
            case 1: return "鉱石：施設の建設に使う（採掘所3・魔力炉2・砦4・祭壇3）";
            case 2: return "影響力：破城（3）・戦術的配置（5）・祭壇（施設）（1）に使う";
            case 3: return "所有拠点数：自分の石がある中立拠点の数（1つにつき影響力+1）";
            default: return "所有施設数：自分の石の上にある施設の数（石と一緒に奪われる）";
            }
        }
    }
    return "";
}

void GameUI::DrawPanels(const GameState& state, const FontSet& fonts) const
{
    DrawSidePanel(state, CellOwner::Player, Ui::LEFT_PANEL_X, fonts);
    DrawSidePanel(state, CellOwner::Enemy, Ui::RIGHT_PANEL_X, fonts);
}

void GameUI::DrawBottom(const GameState& state, const FontSet& fonts, int mouseX, int mouseY, bool pressed,
    const std::string& statusLine, const std::string& hoverInfo) const
{
    DrawUtil::Panel(Ui::LEFT_PANEL_X, Ui::BOTTOM_Y, Ui::SCREEN_W - Ui::LEFT_PANEL_X * 2, Ui::BOTTOM_H);

    // Left: turn, side to move, hover info
    DrawUtil::Text(Ui::LEFT_PANEL_X + 16, Ui::BOTTOM_Y + 12, "ターン " + std::to_string(state.turn.TurnNumber()), Ui::TEXT_ACCENT, fonts.Medium());
    const std::string toMove = state.over ? "ゲーム終了" : "手番: " + state.Current().name;
    DrawUtil::Text(Ui::LEFT_PANEL_X + 16, Ui::BOTTOM_Y + 42, toMove, Ui::TEXT, fonts.Medium());
    DrawUtil::Text(Ui::LEFT_PANEL_X + 16, Ui::BOTTOM_Y + 76, hoverInfo, Ui::TEXT_DIM, fonts.Small());

    // Centre: message log (newest first; newest highlighted for a while)
    int y = Ui::BOTTOM_Y + 10;
    bool first = true;
    for (const auto& m : log_)
    {
        Ui::Rgb color = Ui::TEXT_DIM;
        if (first && m.age < Ui::MESSAGE_HIGHLIGHT_SEC)
        {
            color = m.isError ? Ui::TEXT_ERROR : Ui::TEXT_ACCENT;
        }
        DrawUtil::Text(Ui::LOG_X, y, m.text, color, fonts.Medium());
        y += Ui::LOG_LINE_H;
        first = false;
    }
    if (!statusLine.empty())
    {
        DrawUtil::Text(Ui::LOG_X, Ui::BOTTOM_Y + 10 + Ui::LOG_LINES * Ui::LOG_LINE_H + 4, statusLine, Ui::HOVER_FRAME, fonts.Small());
    }

    help_.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    if (backVisible_)
    {
        back_.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    }
    pass_.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    if (state.IsStrategy())
    {
        ability_.Draw(fonts.Small(), mouseX, mouseY, pressed);
        build_.Draw(fonts.Small(), mouseX, mouseY, pressed);
    }
}

void GameUI::DrawPauseMenu(const FontSet& fonts, int mouseX, int mouseY, bool pressed) const
{
    DrawUtil::Overlay(170);
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, 200, "ポーズ", Ui::TEXT_ACCENT, fonts.Title());
    resume_.Draw(fonts.Large(), mouseX, mouseY, pressed);
    pauseHelp_.Draw(fonts.Large(), mouseX, mouseY, pressed);
    toTitle_.Draw(fonts.Large(), mouseX, mouseY, pressed);
}
