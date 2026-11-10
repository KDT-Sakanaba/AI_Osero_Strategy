# F001: 基盤・通常オセロ・基本UI 設計

- 状態: 確定（2026-11-10）
- 仕様: `spec.md`

## 対象環境
- エンジン／フレームワーク: なし（一般）。C++ ＋ DXライブラリ。手順は `.claude/myspec/engines/generic.md`（判定根拠: `*.uproject`・`project.godot`・Unity の目印なし。ソリューションはこれから作る）
- IDE・ツールセット: Visual Studio Community 2022 17.14（`C:\Program Files\Microsoft Visual Studio\2022\Community`）、プラットフォームツールセット v143、Windows SDK 10.0.26100.0（19041 もあり）
- 言語: C++17（`/std:c++17`）。ソースは UTF-8（BOM なし）、コンパイラオプション `/utf-8`
- DXライブラリ: 未導入（2026-11-10 調査）。T001 で公式サイトの Visual C++ 用パッケージを入手し、「プロジェクトに追加すべきファイル_VC用」の中身を `ThirdParty/DxLib/` に置く。バージョンは入手時点の最新版を使い、`ThirdParty/DxLib/VERSION.txt` に記録する
- ビルド方法: `"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" REIGN_OF_DISCS.sln /p:Configuration=<Debug|Release> /p:Platform=x64 /m /v:minimal`
- テスト方法: ロジックテスト `x64\<構成>\ReignTests.exe`（終了コード 0 で全成功）。画面・操作は手動確認（手順は「検証方針」）
- git: 2026-11-10 に GitHub（KDT-Sakanaba/AI_Osero_Strategy）へ公開

## 既存構造（調査結果）
- ソースコードはまだない（新規作成）。`docs/` に原典 docx・テキスト抽出版・`specs/` のみ。

## 設計

### ソリューション構成
原典 10.2 に従い、テスト用プロジェクトだけを追加する。

```text
REIGN_OF_DISCS/
├─ REIGN_OF_DISCS.sln            # 2プロジェクト（ゲーム・テスト）、x64 の Debug/Release のみ
├─ REIGN_OF_DISCS/
│  ├─ REIGN_OF_DISCS.vcxproj     # ゲーム本体（Windows アプリ、DxLib をリンク）
│  ├─ Source/                    # 原典 10.2 のフォルダ構成
│  ├─ Resources/{Images,Fonts,Sounds,Data}/
│  └─ README.md
├─ Tests/
│  ├─ ReignTests.vcxproj         # コンソールアプリ。ロジック層の .cpp を直接コンパイルする（DxLib をリンクしない）
│  ├─ TestMain.cpp               # 簡易テストランナー（CHECK マクロ、失敗数を終了コードにする）
│  └─ Test*.cpp
├─ ThirdParty/DxLib/             # DxLib.h・*.lib・VERSION.txt
├─ docs/                         # 原典・specs
├─ PROJECT_STATUS.md / IMPLEMENTATION_LOG.md / KNOWN_ISSUES.md
└─ .gitignore                    # x64/、.vs/、*.user など
```

- 原典 10.2 の `docs/GAME_SPECIFICATION.md` などの3ファイルは作らず、`docs/specs/` を設計文書とする（原典 19.6 の進捗3ファイルはルートに作る。constitution 5章）。
- プロジェクト設定（両プロジェクト共通）: x64 のみ、`/W4`、`/utf-8`、`/std:c++17`、ランタイム Debug=`/MTd`・Release=`/MT`（DXライブラリの指定に合わせる）、文字セットは「マルチバイト」。
- ゲーム本体: サブシステム Windows、インクルード・ライブラリのディレクトリに `$(SolutionDir)ThirdParty\DxLib`。

### 層の分け方（DXライブラリへの依存）
| 層 | フォルダ | DxLib | テスト |
|----|----------|-------|--------|
| ロジック | `Board/`、`Systems/`、`Player/`、`Utils/` | 使わない（`#include "DxLib.h"` 禁止） | ReignTests で自動テスト |
| 表示・入力・進行 | `Core/`、`Scenes/`、`UI/`、`Input/`、`Main.cpp` | 使う | 手動確認 |

### 主なデータ型（`Utils/Types.h`）
- `enum class CellOwner { None, Player, Enemy }`（原典 11.1。Player=黒・先手、Enemy=白・後手）、`Opponent(CellOwner)` 関数。
- `struct Pos { int x; int y; }`。
- `enum class CellType`・`enum class FacilityType`（原典 11.2・11.3）は F002 で使うが、`Cell` のフィールドとして F001 で定義しておく（値は常に Normal／None）。
- `enum class RuleMode { Classic, Strategy }`、`enum class OpponentType { Local, AiEasy, AiNormal, AiHard }`、`struct GameConfig { RuleMode rule; OpponentType opponent; }`。
- `struct Cell { CellOwner owner; CellType type; FacilityType facility; bool isDefended; }`
  - **原典 11.4 との差異**: `isSelected` は UI の状態なので `Cell` に持たず、`GameUI` が持つ（原典 19.3「UI とゲームロジックを分離」を優先）。`isDefended` は F002 で毎回再計算する値（F002 spec R-17）。

### クラス（F001 で作るもの）
| クラス | 層 | 責務・主な関数 |
|--------|----|----------------|
| `Board` | ロジック | 8×8 の `Cell` を保持。`Initialize()`（R-03 の初期配置）、`IsInside`、`GetCell(x,y) const`（前提条件は `IsInside`。範囲外なら Debug では assert、Release では配列に触れず空の `Cell` の定数を返す）、`TryGetCell(x,y) -> const Cell*`（範囲外は nullptr）、`SetOwner`、`FlipStones(const std::vector<Pos>&)`、`CountStones`、`CountEmpty`。値型（コピー可能、ポインタを持たない。F003 の探索でコピーする） |
| `MoveValidator` | ロジック | 状態を持たない。`FindFlips(const Board&, Pos, CellOwner, const FlipRule&) -> std::vector<Pos>`（反転対象。空なら不正な手）、`IsLegalMove`、`GetLegalMoves(...) -> std::vector<Pos>`、`HasLegalMove`。探索順は原典 12.1 の配列 |
| `FlipRule` | ロジック | 反転のルール差を外から渡すための小さな構造体。F001 では `isProtected` 判定なし（通常オセロ）。F002 で「防御・強化石は列に含めるが反転しない」（F002 spec R-16）を追加する。通常オセロでは常に既定値を渡すので結果は変わらない |
| `TurnManager` | ロジック | 現在の手番、通算手番数（R-08）、各プレイヤーのターン数、連続パス数。`BeginTurn`、`EndTurnByPlacement`、`Pass`、`IsBothPassed` |
| `VictoryManager` | ロジック | `IsGameOver(const Board&, const TurnManager&)`、`Judge(const Board&) -> Result{winner, blackCount, whiteCount}`。将来の勝利条件を追加できるよう、判定を関数として分ける |
| `Player`（struct） | ロジック | 原典 11.6。F001 は `owner`・`turnCount`・表示名を使う |
| `GameSession` | ロジック | 1ゲーム分の状態と進行をまとめる（`Board`・`TurnManager`・`Player[2]`・`GameConfig`・メッセージ出力キュー）。`TryPlace(Pos) -> PlaceResult`、`ProcessAutoPass()`、`IsOver()`、`GetResult()`。GameScene はこれだけを操作する。値型 |
| `Game` | 表示 | DxLib の初期化（`ChangeWindowMode(TRUE)`、`SetGraphMode(1280,720,32)`、`SetMainWindowText`、`SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8)`、`DxLib_Init`、`SetDrawScreen(DX_SCREEN_BACK)`）、メインループ（`ProcessMessage`・入力更新・シーン更新・描画・`ScreenFlip`）、終了処理（`DxLib_End`） |
| `SceneManager`／`SceneBase` | 表示 | シーンの保持と切り替え。切り替え要求は次のフレームの先頭で適用し、適用時に `InputManager::Flush()`（R-09・AC-07） |
| `TitleScene`・`SelectScene`・`GameScene`・`ResultScene` | 表示 | spec「ユーザー視点の動作」1〜9。`GameConfig` と結果をシーン間で受け渡す |
| `InputManager` | 表示 | マウス位置・左右クリック・キー（ESC）の押下中／押した瞬間。`Flush()` で次のフレームまで押した瞬間を無効化 |
| `BoardRenderer` | 表示 | 盤面・石・合法手の目印・マウスを乗せたマスの枠。画面座標 ↔ マス座標の変換 |
| `GameUI` | 表示 | 左右の情報欄、下部のターン・手番・パスボタン・メッセージログ（最新3件、最新を3秒強調）、ESC メニュー。ボタンは簡易の `Button` 構造体（矩形・ラベル・有効／無効） |
| `FontSet` | 表示 | `CreateFontToHandle` で作ったフォント（大・中・小）を保持し、終了時に `DeleteFontToHandle` |

### 画面レイアウト（`Utils/Constants.h` に定数として置く）
- 画面 1280×720。盤面: 1マス 68px、盤 544×544、左上 (368, 24)。
- 左情報欄 x=24〜344、右情報欄 x=936〜1256、y=24〜568。
- 下部パネル y=584〜704: 左にターン・手番、中央にメッセージログ3行、右にパスボタン（F002 で能力・建設ボタンを追加する領域を空けておく）。
- 色: 盤面の背景は暗い緑、格子線は黒、黒石 (20,20,20)、白石 (235,235,235)、合法手の目印は手番の色の半透明の小円、マウスを乗せたマスは黄色の枠。

### 1フレームの流れ（GameScene）
1. ESC → ポーズメニューを開く／閉じる。メニュー中は盤面入力を無視。
2. ターン開始時（1回だけ）: `GameSession::ProcessAutoPass()`（合法手がなければパスし、メッセージを出す）。両者パスなら終了。
3. 左クリック → `BoardRenderer` でマス座標に変換 → `GameSession::TryPlace`。不正ならメッセージだけ出し、状態は変えない。
4. 終了したら `ResultScene` へ（結果を渡す）。

### 安全性（原典 15章への対応）
- 盤面外: `Board`・`MoveValidator` の公開関数はすべて `IsInside` を確認してから配列に触れる。
- 不正な配置: `TryPlace` は `FindFlips` が空なら何も変えずに失敗を返す（原典 12.4「配置前に盤面を変更しない」）。
- 入力残留: シーン切り替え時の `Flush`。
- リソース: フォントは `FontSet` が所有し、`Game` の終了処理で解放。画像・音は F001 では使わない。

## 変更範囲と依存関係
- すべて新規作成。依存の向き: 表示層 → `GameSession` → `TurnManager`・`VictoryManager`・`MoveValidator` → `Board` → `Types`・`Constants`。ロジック層から表示層への依存はない。
- F002 は `FlipRule`・`Cell` の追加フィールド・`GameSession` の拡張で載せる。F003 は `GameSession` のコピーと `GetLegalMoves` を使う。

## 検証方針
| 受け入れ条件 | 確認方法 |
|--------------|----------|
| AC-01 | MSBuild で Debug・Release (x64) をビルドし、エラー0・新規警告0を確認。起動してウィンドウのタイトル・サイズを確認し、「終了」と×で終わることをユーザーが手動確認 |
| AC-02 | ReignTests: 初期盤面の4石と残り60マスが空 |
| AC-03 | ReignTests: 初期局面の合法手が (3,2)(2,3)(5,4)(4,5) の4つだけ。不正な座標（空でない・反転0・盤面外）で `TryPlace` しても盤面・手番が変わらない。手動: 目印が合法手だけに出る |
| AC-04 | ReignTests: 1方向・複数方向（8方向すべてを含む局面）・端で途切れる・空白で途切れる・自石隣接の各ケース |
| AC-05 | ReignTests: 片方だけ合法手がない局面で自動パス、両者合法手なしで終了。手動: パスのメッセージ |
| AC-06 | ReignTests: 64マス埋まり（黒勝ち・白勝ち・32対32 の引き分け）と、両者パスで空きマスが残る終了の判定 |
| AC-07 | 手動: タイトル→選択→ゲーム→結果→もう一度／タイトルへ、ESC→タイトルへ。結果画面の「もう一度」をクリックした直後に盤面へ石が置かれないこと |
| AC-08 | 手動: ローカル2人対戦で1局を最後まで遊ぶ |
| AC-09 | ReignTests.exe の終了コード 0 と、失敗0件の出力 |
| AC-10 | 手動: タイトル・メッセージ・結果の日本語が文字化けしない |

## リスク・代替案
- DXライブラリの公式サイトの構成やファイル名が変わっている可能性がある → T001 で実際のページを確認してから取得する。取得できなければユーザーに手動での配置を依頼する。
- DXライブラリの文字コード設定: `SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8)` と `/utf-8` の組み合わせで文字化けする場合は、文字セットを Unicode にして `DX_CHARCODEFORMAT_UTF8` を見直すか、文字列リテラルの扱いを変える（T003 の動作確認で判断）。
- ロジック層のファイルを2つのプロジェクトでコンパイルするため、ファイル追加時に両方の vcxproj に追加し忘れる恐れ → ロジック層の .cpp を `Source/Logic.props`（共有の項目リスト）にまとめ、両プロジェクトから読み込む。
- 原典 11.4 から `isSelected` を外す差異 → ユーザーのレビュー対象として報告する。
