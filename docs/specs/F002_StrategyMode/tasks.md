# F002: ストラテジーモード タスク

- 状態: 実装完了（ユーザーの手動確認待ち: AC-10）
- Analyze: 合格（2026-11-10。`analysis.md`）
- 仕様: `spec.md` ／ 設計: `plan.md`

## タスク

### 段階1: ロジック

- [x] T001: 資源と収入
  - 目的: 資源の加算・上限・収入計算と、ターン開始時の獲得を作る。
  - 作業: `Systems/ResourceManager.*`、`GameRules::BeginTurn`。
  - 関連ファイル: 上記、`Tests/TestStrategy.cpp`
  - 依存: なし（F001 完了）
  - 対応AC: AC-01、AC-02
  - 完了条件: 配置表・収入・上限のテストが成功。
  - 結果: 2026-11-10 ResourceManager と BeginTurn を実装。ReignTests: F002_LayoutIsFixedAndSymmetric・T009_ResourceIncome・T009_IncomeOnlyAtOwnTurnStartViaSession 成功。

- [x] T002: 施設と防御
  - 目的: 建設・施設の効果・防御の計算を作る。
  - 作業: `Systems/FacilityManager.*`、`GameRules::ApplyPlacement` 後の防御再計算。
  - 関連ファイル: 上記
  - 依存: T001
  - 対応AC: AC-03、AC-04、AC-05、AC-06
  - 完了条件: 建設・奪取・防御・R-16 のテストが成功。
  - 結果: 2026-11-10 FacilityManager（建設・防御の再計算）を実装。ReignTests: T010・T011・T012・F002_FortDefenceRangeOverlapAndCapture・F002_R16_ProtectedStonesInsideLines 成功。

- [x] T003: 特殊能力と期限
  - 目的: 5能力と、強化・無効化の期限切れを作る。
  - 作業: `Systems/AbilityManager.*`、`GameRules::EndTurn`・`MustAutoPass`。
  - 関連ファイル: 上記
  - 依存: T002
  - 対応AC: AC-03、AC-07、AC-08
  - 完了条件: 能力ごとのテストと、合法手なしのときの手動パス・自動パスのテストが成功。
  - 結果: 2026-11-10 AbilityManager・期限切れ・MustAutoPass を実装。ReignTests: T013（5能力）・F002_PassRulesInStrategy 成功。

- [x] T004: GameSession の操作とイベント
  - 目的: 画面と AI が使う操作（能力・建設）を GameSession に追加する。
  - 作業: `TryUseAbility`・`TryBuild`、拒否理由の文言、盤面が埋まったときの終了。ストラテジーのランダム対局テスト。
  - 関連ファイル: `Systems/GameSession.*`、`Tests/TestStrategy.cpp`
  - 依存: T003
  - 対応AC: AC-03、AC-08、AC-09、AC-11
  - 完了条件: ReignTests 全件成功（F001 の回帰を含む）。
  - 結果: 2026-11-10 TryUseAbility・TryBuild を追加。ReignTests 27件すべて成功（F001 の回帰を含む）。ランダム対局200局で不整合なし、平均約60手で終局（膠着なし）。

### 段階2: 画面

- [x] T005: ストラテジーの操作画面
  - 目的: 能力・建設のメニュー、対象選択、マス情報を作り、モード選択で有効にする。
  - 作業: `Scenes/GameScene.*`、`UI/GameUI.*`、`Core/Features.h`。
  - 関連ファイル: 上記
  - 依存: T004
  - 対応AC: AC-01、AC-03、AC-10
  - 完了条件: 自動 UI 操作で、特殊マスの表示・能力の使用・建設・キャンセル・拒否メッセージをスクリーンショットで確認。
  - 結果: 2026-11-10 能力・建設メニュー、対象選択、マス情報のツールチップ、ストラテジーの有効化。検証用モード（`--script`）で、特殊マスの表示・資源採掘・採掘所の建設・メニューの拒否理由・キャンセルを画面保存で確認。設計補足: 検証のため `Input/InputScript`（OS の入力を使わない台本再生と画面保存）を追加した。

- [x] T006: 検証と記録
  - 目的: F002 の受け入れ条件をまとめて確認する。
  - 作業: Debug/Release ビルド、ReignTests、自動 UI 操作でストラテジーの1局、記録。
  - 関連ファイル: tasks.md、`IMPLEMENTATION_LOG.md`
  - 依存: T005
  - 対応AC: AC-01〜AC-11
  - 完了条件: すべての AC に確認結果がある（ユーザーの手動確認は F004 でまとめて依頼）。
  - 結果: 2026-11-10 Debug・Release ビルド成功（警告0）、ReignTests 27件成功。検証用モードでストラテジーの1局を結果画面（残り資源の表示）まで確認。ユーザーの手動確認（AC-10）は F004 でまとめて依頼。

## 再開メモ
- 最終更新日: 2026-11-10
- 完了した作業: T001〜T006。
- 未解決: ユーザーの手動確認（AC-10）。
- 次に行う作業: F003（AI）。
