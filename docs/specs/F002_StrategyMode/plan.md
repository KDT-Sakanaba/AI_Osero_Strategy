# F002: ストラテジーモード 設計

- 状態: 確定（2026-11-10）
- 仕様: `spec.md`

## 対象環境
F001 の plan.md「対象環境」と同じ（VS2022 v143、C++17、DxLib 3.25a、`build.ps1` でビルドとロジックテスト）。

## 既存構造（F001 完了時点の調査結果）
- `Source/Utils/Types.h`: CellOwner・CellType・FacilityType・AbilityType・Pos・GameConfig。`Constants.h`: 盤面・初期配置・特殊マス配置（`SPECIAL_CELLS`）・資源の初期値と最大値・能力の回数制限。
- `Source/Board/Cell.h`: `isDefended`・`enhancedUntil`・`disabledUntil` と `IsProtected()`。`Board`: 値型。`ApplyStrategyLayout()` 実装済み。
- `Source/Board/MoveValidator`: `FlipRule.respectProtection` で防御石を「列に含めるが反転しない」（F002 R-16）を実装済み。通常オセロは既定値。
- `Source/Systems/GameState.h`: 盤面・ターン・両者の Player・`TurnActions`（abilityUsed・built）。`GameRules`: Setup・ApplyPlacement・BeginTurn（空）・CanPass・MustAutoPass・EndTurn。`GameSession`: TryPlace・TryPass・イベント（Placed・Passed・ResourceGained・Built・AbilityUsed・Rejected・GameOver）。
- `Source/UI/GameUI`: 資源・拠点数・施設数・回数の表示、能力・建設ボタン、資源の浮き上がり表示は実装済み（ストラテジー時のみ表示）。`BoardRenderer`: 特殊マスの色・文字、施設の印、防御枠、強化の輪、対象の枠を描ける。

## 設計

### 追加するクラス（ロジック層、DxLib なし）
| クラス | 責務 |
|--------|------|
| `ResourceManager` | `Clamp`、`AddCapped(Player&, Resources) -> 実際に増えた量`、`Shortage(have, cost) -> 不足の文言`、`TurnIncome(const GameState&, CellOwner) -> Resources`（資源マス＋無効でない施設、マスごと1回） |
| `FacilityManager` | `Cost(type)`・`Effect(type)` の表、`CheckBuild(state, pos, type) -> 失敗理由（空なら可）`、`Build(state, pos, type)`、`BuildableCells(state, type)`、`RecomputeDefense(Board&)`（全マスの isDefended を砦の位置・所有者・無効化から計算し直す。R-14・R-15・R-17） |
| `AbilityManager` | 能力の定義表（名前・コスト・対象の要否・回数上限）、`CheckUse(state, type) -> 失敗理由`、`ValidTargets(state, type)`、`IsValidTarget`、`Use(state, type, pos, outResult)` |

### ルールの詳細
- 資源の収入（R-08）: `GameRules::BeginTurn` で、手番側の `TurnIncome` を `AddCapped` する。マナ鉱脈 マナ+2、鉱石鉱脈 鉱石+2、祭壇マス マナ+1 影響力+1、中立拠点 影響力+1、採掘所 鉱石+1、魔力炉 マナ+1、祭壇（施設） 影響力+1、砦 なし。
- 防御（R-14〜R-17）: 盤面が変わるたび（配置・強制反転・戦術的配置・破城・期限切れ・建設）に `RecomputeDefense` を呼ぶ。
- 期限（R-19・R-22）: ターン番号 N で使ったとき、魔力強化は `enhancedUntil = N+1`、破城は `disabledUntil = N+3`。`EndTurn` でターン T を終える前に、`<= T` のものを 0 に戻す（その後 `RecomputeDefense`）。
- 合法手がないとき（R-26）: `MustAutoPass` = 合法手なし かつ 使える能力も建てられる施設もない。`CanPass` = 合法手なし。
- 戦術的配置で盤面が埋まった場合: その時点でターンを終え、終了判定に進む。

### GameSession への追加
- `TryUseAbility(AbilityType, Pos)`、`TryBuild(FacilityType, Pos)`。失敗時は Rejected イベント（原典 8.5 の文言: 「マナが不足しています」「このマスには建設できません」など）を出し、状態を変えない。
- 成功時は AbilityUsed／Built イベント（反転した石・対象のマス付き）。

### 画面（GameScene）
- 操作モード: 通常 → 能力メニュー → 対象選択、通常 → 建設メニュー → 対象選択。右クリック・ESC で1段階戻る（通常モードの ESC はポーズ）。
- メニューは盤面の上に重ねるパネル。各行に名前・コスト・効果・残り回数を表示し、使えない行は灰色にして理由を下に出す。
- 対象選択中は、有効な対象マスをオレンジの枠で示し、合法手の目印は消す。
- マウスを乗せたマスの情報: 座標、マスの種類、所有者、施設（無効化中）、防御中・強化中、そのマスからの収入。
- モード選択でストラテジーを有効化（`Features::STRATEGY_MODE = true`）。

## 変更範囲と依存関係
- 追加: `Systems/ResourceManager.*`、`Systems/FacilityManager.*`、`Systems/AbilityManager.*`、`Tests/TestStrategy.cpp`。
- 変更: `GameRules.cpp`（BeginTurn・ApplyPlacement 後の防御再計算・MustAutoPass・EndTurn の期限処理）、`GameSession.*`、`GameScene.*`、`GameUI.*`（メニュー）、`Features.h`。
- 通常オセロでは `state.IsStrategy()` が false のため、追加した処理はすべて呼ばれない（R-28）。F001 のテストを回帰テストとしてそのまま使う。

## 検証方針
| 受け入れ条件 | 確認方法 |
|--------------|----------|
| AC-01 | ReignTests: 配置表どおりの CellType・点対称。通常オセロで全マス Normal。自動 UI 操作のスクリーンショット |
| AC-02 | ReignTests: 各資源マス・施設の収入、ターン開始時だけ、最大値で切り捨て、無効化中の施設は収入なし（T009） |
| AC-03 | ReignTests: 資源不足・使用済み・回数上限・不正な対象で状態が変わらず理由が返る（T010） |
| AC-04 | ReignTests: 4施設の建設でコストが減り、次のターン開始時から効果（T011） |
| AC-05 | ReignTests: 施設マスの石が通常反転・強制反転されると所有と収入が移る（T012） |
| AC-06 | ReignTests: 砦の上下左右だけ防御、重なり、所有者変更、破城で解除、R-16 の挟み方（列に含むが反転しない、防御石だけの方向は不成立） |
| AC-07 | ReignTests: 5能力の効果・コスト・回数・対象・期間（T013） |
| AC-08 | ReignTests: 能力・建設が2回目で拒否、配置でターン終了 |
| AC-09 | ReignTests の F001 テストがすべて成功（回帰） |
| AC-10 | 自動 UI 操作で能力・建設の操作とストラテジーモードの1局を確認。ユーザーの手動確認は F004 でまとめて依頼 |
| AC-11 | ReignTests 全件成功、ストラテジーのランダム対局（能力・建設も乱択）多数で不整合なし |

## リスク・代替案
- 防御が強すぎて膠着する恐れ（原典 21章）→ 自己対戦で終局までの手数を記録し、極端に長い・パスが続く場合は KNOWN_ISSUES に記録してユーザーに調整案を示す（独断でルールを変えない）。
