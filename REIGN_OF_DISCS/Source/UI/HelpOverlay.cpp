#include "UI/HelpOverlay.h"
#include "DxLib.h"
#include "Input/InputManager.h"
#include "UI/BoardRenderer.h"
#include "UI/FontSet.h"
#include "UI/UiConstants.h"

namespace
{
    constexpr int BOX_X = 80;
    constexpr int BOX_Y = 32;
    constexpr int BOX_W = Ui::SCREEN_W - 160;
    constexpr int BOX_H = Ui::SCREEN_H - 64;
    constexpr int TEXT_X = BOX_X + 40;
    constexpr int CONTENT_Y = BOX_Y + 96;
    constexpr int LINE_H = 30;

    const char* TAB_NAMES[HelpOverlay::PAGE_COUNT] = { "基本ルール", "盤面の記号", "資源と能力", "操作" };

    void Line(int& y, const std::string& text, const Ui::Rgb& color, int font, int x = TEXT_X)
    {
        DrawUtil::Text(x, y, text, color, font);
        y += LINE_H;
    }

    void Heading(int& y, const std::string& text, int font)
    {
        y += 6;
        DrawUtil::Text(TEXT_X, y, text, Ui::TEXT_ACCENT, font);
        y += LINE_H + 4;
    }

    void CellSwatch(int x, int y, const Ui::Rgb& color, const char* mark, int font)
    {
        DrawUtil::FillRect(x, y, 44, 44, color);
        DrawUtil::FrameRect(x, y, 44, 44, Ui::GRID);
        DrawUtil::Text(x + 3, y + 2, mark, Ui::TEXT, font);
    }
}

HelpOverlay::HelpOverlay()
{
    const int tabW = 200;
    const int tabsX = BOX_X + (BOX_W - (tabW * PAGE_COUNT + 12 * (PAGE_COUNT - 1))) / 2;
    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        tabs_[i] = Button{ tabsX + i * (tabW + 12), BOX_Y + 24, tabW, 48, TAB_NAMES[i] };
    }
    prev_ = Button{ BOX_X + 40, BOX_Y + BOX_H - 84, 160, 52, "← 前へ" };
    next_ = Button{ BOX_X + BOX_W - 200, BOX_Y + BOX_H - 84, 160, 52, "次へ →" };
    close_ = Button{ BOX_X + (BOX_W - 200) / 2, BOX_Y + BOX_H - 84, 200, 52, "閉じる" };
}

void HelpOverlay::Open(int page)
{
    open_ = true;
    page_ = (page >= 0 && page < PAGE_COUNT) ? page : 0;
}

bool HelpOverlay::Update(const InputManager& in)
{
    if (!open_)
    {
        return false;
    }
    const int mx = in.MouseX();
    const int my = in.MouseY();
    const bool click = in.LeftClicked();
    prev_.enabled = page_ > 0;
    next_.enabled = page_ < PAGE_COUNT - 1;
    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        tabs_[i].selected = (i == page_);
        if (tabs_[i].Clicked(mx, my, click)) { page_ = i; return true; }
    }
    if (prev_.Clicked(mx, my, click)) { --page_; return true; }
    if (next_.Clicked(mx, my, click)) { ++page_; return true; }
    if (close_.Clicked(mx, my, click) || in.RightClicked() || in.KeyPressed(KEY_INPUT_ESCAPE) || in.KeyPressed(KEY_INPUT_H))
    {
        open_ = false;
        return true;
    }
    return false;
}

void HelpOverlay::Draw(const FontSet& fonts, int mouseX, int mouseY, bool pressed) const
{
    if (!open_)
    {
        return;
    }
    DrawUtil::Overlay(190);
    DrawUtil::FillRect(BOX_X, BOX_Y, BOX_W, BOX_H, Ui::PANEL_BG);
    DrawUtil::FrameRect(BOX_X, BOX_Y, BOX_W, BOX_H, Ui::HOVER_FRAME, 2);
    for (const Button& t : tabs_) t.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    DrawPage(fonts);
    prev_.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    next_.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    close_.Draw(fonts.Medium(), mouseX, mouseY, pressed);
    DrawUtil::TextCentered(Ui::SCREEN_W / 2, BOX_Y + BOX_H - 26,
        std::to_string(page_ + 1) + " / " + std::to_string(PAGE_COUNT) + "　（右クリック・ESC・H でも閉じます）", Ui::TEXT_DIM, fonts.Small());
}

void HelpOverlay::DrawPage(const FontSet& f) const
{
    int y = CONTENT_Y;
    const int m = f.Medium();
    const int s = f.Small();

    switch (page_)
    {
    case 0:
        Heading(y, "共通のルール", m);
        Line(y, "・相手の石を縦・横・斜めで挟めるマスにだけ置けます。挟んだ石は自分の色になります。", Ui::TEXT, m);
        Line(y, "・置けるマスには小さな点が出ます。置ける場所がなければ自動でパスします。", Ui::TEXT, m);
        Line(y, "・盤面が埋まるか、両者が続けてパスすると終了。石の多い方の勝ちです。", Ui::TEXT, m);
        Heading(y, "ストラテジーモードの1ターン", m);
        Line(y, "① ターン開始時に、自分の石がある資源マス・施設から資源が入ります。", Ui::TEXT, m);
        Line(y, "② 「能力」と「建設」をそれぞれ1回まで、好きな順で使えます（使わなくてもよい）。", Ui::TEXT, m);
        Line(y, "③ 石を1つ置くとターン終了です。", Ui::TEXT, m);
        Line(y, "・置ける場所がなくても能力・建設が使える間は自動でパスしません。終えるときは「パス」。", Ui::TEXT, m);
        Line(y, "・砦に守られた石（青い枠）と強化された石（ピンクの輪）は、挟まれても裏返りません。", Ui::TEXT, m);
        Line(y, "・砦が守るのは相手の次の1ターンだけで、その後は消えます。砦を奪うと、もう1ターン行えます。", Ui::TEXT_ACCENT, m);
        break;

    case 1:
    {
        Heading(y, "特殊マス（ターン開始時の収入）", m);
        const int col2 = TEXT_X + 520;
        struct Item { Ui::Rgb color; const char* mark; const char* text; };
        const Item cells[4] = {
            { Ui::MANA_CELL, "マナ", "マナ鉱脈：マナ +2" },
            { Ui::ORE_CELL, "鉱石", "鉱石鉱脈：鉱石 +2" },
            { Ui::ALTAR_CELL, "祭壇", "祭壇マス：マナ +1・影響力 +1" },
            { Ui::BASE_CELL, "拠点", "中立拠点：影響力 +1" },
        };
        for (int i = 0; i < 4; ++i)
        {
            const int x = (i % 2 == 0) ? TEXT_X : col2;
            const int yy = y + (i / 2) * 54;
            CellSwatch(x, yy, cells[i].color, cells[i].mark, s);
            DrawUtil::Text(x + 58, yy + 12, cells[i].text, Ui::TEXT, m);
        }
        y += 2 * 54 + 4;

        Heading(y, "施設（マス右下の金色の札）", m);
        const char* facilities[4][2] = {
            { "採", "採掘所：ターン開始時に鉱石 +1" }, { "炉", "魔力炉：ターン開始時にマナ +1" },
            { "砦", "砦：相手の1ターンだけ上下左右を守る" }, { "壇", "祭壇（施設）：ターン開始時に影響力 +1" },
        };
        for (int i = 0; i < 4; ++i)
        {
            const int x = (i % 2 == 0) ? TEXT_X : col2;
            const int yy = y + (i / 2) * 40;
            DrawUtil::FillRect(x + 10, yy + 2, 24, 24, Ui::TEXT_ACCENT);
            DrawUtil::Text(x + 14, yy + 5, facilities[i][0], Ui::BLACK_STONE, s);
            DrawUtil::Text(x + 58, yy + 3, facilities[i][1], Ui::TEXT, m);
        }
        y += 2 * 40 + 4;

        Heading(y, "石とマスの印", m);
        // defended
        DrawUtil::FillRect(TEXT_X, y, 44, 44, Ui::BOARD_BG);
        BoardRenderer::DrawStone(TEXT_X + 22, y + 22, CellOwner::Player, 1.0, 0.65);
        DrawUtil::FrameRect(TEXT_X + 2, y + 2, 40, 40, Ui::DEFENSE_MARK, 3);
        DrawUtil::Text(TEXT_X + 58, y + 12, "青い枠：防御中（砦の効果）", Ui::TEXT, m);
        // enhanced
        DrawUtil::FillRect(col2, y, 44, 44, Ui::BOARD_BG);
        BoardRenderer::DrawStone(col2 + 22, y + 22, CellOwner::Enemy, 1.0, 0.65);
        DrawCircleAA(static_cast<float>(col2 + 22), static_cast<float>(y + 22), 21.0f, 32, DrawUtil::Color(Ui::ENHANCE_MARK), FALSE, 3.0f);
        DrawUtil::Text(col2 + 58, y + 12, "ピンクの輪：強化中（魔力強化）", Ui::TEXT, m);
        y += 54;
        // hint / target
        DrawUtil::FillRect(TEXT_X, y, 44, 44, Ui::BOARD_BG);
        DrawCircleAA(static_cast<float>(TEXT_X + 22), static_cast<float>(y + 22), 6.0f, 16, DrawUtil::Color(Ui::BLACK_STONE), TRUE);
        DrawUtil::Text(TEXT_X + 58, y + 12, "小さな点：石を置けるマス", Ui::TEXT, m);
        DrawUtil::FillRect(col2, y, 44, 44, Ui::BOARD_BG);
        DrawUtil::FrameRect(col2 + 1, y + 1, 42, 42, Ui::TARGET_MARK, 2);
        DrawUtil::Text(col2 + 58, y + 12, "オレンジの枠：能力・建設の対象にできるマス", Ui::TEXT, m);
        y += 54;
        DrawUtil::FillRect(TEXT_X + 10, y + 2, 24, 24, Ui::BUTTON_DISABLED);
        DrawUtil::Text(TEXT_X + 14, y + 5, "砦", Ui::BLACK_STONE, s);
        DrawUtil::Text(TEXT_X + 58, y + 3, "灰色の札：破城で無効化中の施設", Ui::TEXT, m);
        break;
    }

    case 2:
        Heading(y, "資源の使い道（上限：マナ15・鉱石15・影響力10）", m);
        Line(y, "マナ　　：魔力強化（3）・強制反転（5）・魔力炉の建設（2）", Ui::TEXT, m);
        Line(y, "鉱石　　：すべての施設の建設（採掘所3・魔力炉2・砦4・祭壇3）", Ui::TEXT, m);
        Line(y, "影響力　：破城（3）・戦術的配置（5）・祭壇（施設）の建設（1）", Ui::TEXT, m);
        Heading(y, "特殊能力（1ターンに1回）", m);
        Line(y, "魔力強化（マナ3）：自分の石1つを、相手の次のターン終了まで裏返らなくする", Ui::TEXT, m);
        Line(y, "強制反転（マナ5）：敵の石1つを自分の石にする（防御中・強化中は不可）", Ui::TEXT, m);
        Line(y, "資源採掘（なし）　：鉱石 +2（1ゲームに3回まで）", Ui::TEXT, m);
        Line(y, "破城（影響力3）　：敵の施設1つを、相手のターン2回分だけ無効にする", Ui::TEXT, m);
        Line(y, "戦術的配置（影響力5）：自分の石の隣の空きマスに石を追加で置く（裏返さない・1回まで）", Ui::TEXT, m);
        Line(y, "※ 施設は石と一緒に奪われます。砦を奪った側は、もう1ターン行えます。", Ui::TEXT_DIM, m);
        break;

    default:
        Heading(y, "マウス・キー", m);
        Line(y, "左クリック　　：石を置く／ボタンを押す／能力・建設の対象マスを選ぶ", Ui::TEXT, m);
        Line(y, "右クリック　　：選択の解除・1段階戻る", Ui::TEXT, m);
        Line(y, "マウスを乗せる：マスの情報、資源やボタンの説明を表示", Ui::TEXT, m);
        Line(y, "ESC　　　　　：メニューを閉じる／ポーズ", Ui::TEXT, m);
        Line(y, "H　　　　　　：このヘルプを開く・閉じる", Ui::TEXT, m);
        Heading(y, "能力・建設の使い方（ストラテジー）", m);
        Line(y, "① 画面右下の「能力」または「建設」を押す", Ui::TEXT, m);
        Line(y, "② 一覧から使うものを選ぶ（灰色は今は使えない。理由が赤字で出ます）", Ui::TEXT, m);
        Line(y, "③ オレンジの枠のマスをクリックして実行（資源採掘は選んだ時点で実行）", Ui::TEXT, m);
        break;
    }
}
