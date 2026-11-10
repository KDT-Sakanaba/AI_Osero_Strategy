# F003: AI 対戦 タスク

- 状態: 実装完了（ユーザーの手動確認待ち: AC-07）
- Analyze: 合格（2026-11-10。AC-01〜AC-07 すべてにタスクと確認方法あり、依存は前方参照なし、`[要確認]` なし、対象環境は F001 で確認済み）
- 仕様: `spec.md` ／ 設計: `plan.md`

## タスク

- [x] T001: EASY・NORMAL
  - 目的: 合法な行動だけを選ぶ AI の土台と、EASY・NORMAL を作る。
  - 作業: `Player/AIPlayer.*`（AiAction・AiSettings・DecideTurn、NORMAL の配置評価と建設・能力のルール）。
  - 関連ファイル: 上記、`Tests/TestAI.cpp`
  - 依存: なし（F002 完了）
  - 対応AC: AC-01、AC-02、AC-03、AC-06
  - 完了条件: 自己対戦で拒否 0 件、パス・角のテストが成功。
  - 結果: 2026-11-10 AIPlayer（EASY・NORMAL、建設・能力のルール）を実装。ReignTests: T014_AllLevelsOnlyLegalActions（3難易度×2ルール、拒否0件）・F003_PassWhenNoMove・F003_NormalTakesCorner・F003_EasyNeverUsesStrategyActions・F003_NormalUsesStrategyWhenRich 成功。

- [x] T002: HARD
  - 目的: 反復深化 Minimax + Alpha-Beta と評価関数を作る。
  - 作業: `AIPlayer.cpp` に探索・評価・根の候補を追加。
  - 関連ファイル: 上記
  - 依存: T001
  - 対応AC: AC-01、AC-04、AC-06
  - 完了条件: 自己対戦で拒否 0 件、Release で思考時間 ≤ 2 秒。
  - 結果: 2026-11-10 反復深化 Minimax + Alpha-Beta と評価関数を実装。Release で中盤20局面の最大思考時間 0.078 秒、深さ4まで完了（F003_HardThinkingTimeLimit）。

- [x] T003: 強さの確認
  - 目的: 難易度の差を確認する。
  - 作業: `SLOW_AC05` テスト、必要なら重みの調整。
  - 関連ファイル: `Tests/TestAI.cpp`、`Player/AIPlayer.cpp`
  - 依存: T002
  - 対応AC: AC-05
  - 完了条件: HARD > NORMAL > EASY をそれぞれ勝率 60% 以上で確認。
  - 結果: 2026-11-10 SLOW_AC05（Release、各50局・先後入れ替え）: NORMAL 対 EASY 88%（通常）・96%（ストラテジー）、HARD 対 NORMAL 76%（通常）・88%（ストラテジー）。拒否0件。重みの調整は不要だった。

- [x] T004: 画面への組み込み
  - 目的: AI の手番を非同期で考え、行動を順に見せる。
  - 作業: `Scenes/GameScene.*`、`Core/Features.h`。
  - 関連ファイル: 上記
  - 依存: T001
  - 対応AC: AC-07
  - 完了条件: 検証用モードで AI 対戦（両ルール）が結果画面まで進む。
  - 結果: 2026-11-10 GameScene に非同期思考（std::async）と0.5秒間隔の行動表示、行動が拒否されたときの安全な手番終了を追加。検証用モードで NORMAL（通常オセロ）と HARD（ストラテジー）の対局を結果画面まで確認。

- [x] T005: 検証と記録
  - 目的: F003 の受け入れ条件をまとめて確認する。
  - 作業: Debug/Release ビルド、ReignTests（SLOW を含む）、記録。
  - 関連ファイル: tasks.md
  - 依存: T003、T004
  - 対応AC: AC-01〜AC-07
  - 完了条件: すべての AC に確認結果がある。
  - 結果: 2026-11-10 Debug/Release ビルド成功（警告0）、ReignTests 33件＋SLOW 1件成功。ユーザーの手動確認（AC-07）は F004 でまとめて依頼。

## 再開メモ
- 最終更新日: 2026-11-10
- 完了した作業: T001〜T005。
- 次に行う作業: F004（演出・設定・音・最終テスト）。
