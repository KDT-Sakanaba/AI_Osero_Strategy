# F003: AI 対戦 設計

- 状態: 確定（2026-11-10）
- 仕様: `spec.md`

## 対象環境
F001 の plan.md と同じ。スレッドは C++ 標準の `std::async`／`std::future` を使う（外部ライブラリなし）。

## 既存構造（F002 完了時点）
- `GameState`（値型、コピーして探索できる）、`GameRules`（ApplyPlacement・BeginTurn・EndTurn・CanPass・MustAutoPass）、`GameSession`（TryPlace・TryPass・TryUseAbility・TryBuild）。
- `AbilityManager`（CheckUse・ValidTargets・Use）、`FacilityManager`（CheckBuild・BuildableCells・Build）、`ResourceManager`（TurnIncome）。
- `GameScene::IsHumanTurn()` は AI 側の手番で人の入力を止める形になっている。`Features::AI_OPPONENT` で選択を有効化する。
- 検証用モード `--script`（`Input/InputScript`）で画面を保存できる。

## 設計

### `Player/AIPlayer`（ロジック層、DxLib なし）
- `struct AiAction { Kind {UseAbility, Build, Place, Pass}; AbilityType ability; FacilityType facility; Pos pos; }`
- `struct AiSettings { OpponentType level; int maxDepth = 4; double timeLimitSec = 2.0; std::uint32_t seed; }`
- `std::vector<AiAction> AIPlayer::DecideTurn(const GameState&, const AiSettings&)`: 手番1回分の行動列（建設・能力は0〜1回ずつ、最後は Place か Pass）。作業用コピーに順に適用しながら決めるので、能力で合法手が変わっても正しい。
- EASY: 合法手からランダム。なければ Pass。建設・能力は使わない。
- NORMAL:
  - 配置: 1手先評価。角 +100、辺 +20、資源マス（マナ鉱脈・鉱石鉱脈・祭壇マス）+30、拠点 +40（資源・拠点はストラテジーのみ）、置いた後に相手が新たに角を取れるようになる手 -80、空きマスが 20 を超える間は反転数 ×2 を減点。同点はランダム。
  - 建設・能力（ストラテジー）: ルールベース。能力の優先順は 強制反転（角・資源マス・拠点・施設のある敵石）→ 破城（敵の砦、なければ収入のある施設）→ 戦術的配置（角・資源マス・拠点が対象のとき）→ 魔力強化（相手の次の手で反転されうる資源マス・拠点・施設の自石）→ 資源採掘（鉱石 11 以下）。建設は「魔力炉（マナ 10 未満）→ 採掘所 → 祭壇（施設）→ 砦（防御できる自石が2つ以上）」の順で最初に建てられるものを、相手の次の手で反転されにくいマス（角 > 辺 > 相手の合法手で反転対象にならないマス）に建てる。
- HARD: 反復深化の Minimax + Alpha-Beta（深さ 1 から最大 `maxDepth`、制限時間を超えたら直前に完了した深さの最善手）。
  - 根: 「NORMAL のルールで選ぶ能力候補の上位 2 件＋建設候補 1 件＋何もしない」の組み合わせ（最大 6 通り）× 配置。2 層目以降は配置とパスだけ。
  - 局面の評価（AI 側から見た値）: 石数差（終盤ほど重い）、角 ×25、辺 ×5、合法手の数の差 ×4、資源の合計 ×1、施設 ×10、拠点 ×15、資源マス ×8、強制反転が使える状態 +6。終局は勝ち +10000＋石数差、負け −10000＋石数差。
  - 探索中の手番の進行は `GameRules`（ApplyPlacement → EndTurn → BeginTurn）をそのまま使う（R-02）。
  - 手の並べ替え: 角 → 辺 → その他（枝刈りを効かせるため）。
- 乱数: `std::mt19937`（`AiSettings::seed`）。テストでは固定シード。
- 評価の重みは `AIPlayer.cpp` 冒頭の定数表にまとめる（R-07）。

### 画面（GameScene）
- AI の手番が始まったら、状態をコピーして `std::async` で `DecideTurn` を実行（R-08: 描画を止めない）。結果が出るまで「敵AIが考えています…」を表示。
- 結果の行動を 0.5 秒間隔で1つずつ `GameSession` に渡す。行動が拒否された場合（通常は起きない）は、残りを捨てて、合法手があればその先頭、なければパスで手番を終える。
- ポーズ中は AI の行動の実行を止める（思考は続く）。タイトルへ戻るときは思考の終了を待ってからシーンを破棄する（future の破棄で待つ）。
- `Features::AI_OPPONENT = true`。

## 変更範囲と依存関係
- 追加: `Player/AIPlayer.*`、`Tests/TestAI.cpp`。
- 変更: `Scenes/GameScene.*`、`Core/Features.h`、`Tests/TestMain.cpp`（名前が `SLOW_` で始まる重いテストは、名前を指定したときだけ実行）。

## 検証方針
| 受け入れ条件 | 確認方法 |
|--------------|----------|
| AC-01 | ReignTests: 3難易度 × 2ルールの自己対戦（相手はランダム）で、AI の行動がすべて受理される（拒否 0 件） |
| AC-02 | ReignTests: 合法手がない局面で Pass を返す。AI 同士の対局が両者パス・満杯で終わる |
| AC-03 | ReignTests: 角が取れる指定局面で NORMAL が角を選ぶ |
| AC-04 | ReignTests（Release）: 中盤の局面 20 件で HARD の思考時間を計測、最大値 ≤ 2.0 秒 |
| AC-05 | ReignTests `SLOW_AC05`（Release）: NORMAL 対 EASY、HARD 対 NORMAL を先後入れ替えて各 50 局（計 100 局）、勝率 60% 以上 |
| AC-06 | AC-01 の自己対戦で、建設・能力の拒否 0 件 |
| AC-07 | 検証用モードで AI 対戦（両ルール）を結果画面まで進め、画面保存で確認。ユーザーの手動確認は F004 でまとめて依頼 |

## リスク・代替案
- Debug ビルドでは探索が遅く 2 秒に達しやすい → 時間制限で打ち切るので応答は保たれる。AC-04 は Release で判定する。
- HARD が NORMAL に勝ち越せない場合は、評価の重みを調整する（ルールは変えない）。
