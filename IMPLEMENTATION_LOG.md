# IMPLEMENTATION_LOG

## 2026-11-10 F001 基盤・通常オセロ・基本UI
- DXライブラリ 3.25a（公式 DxLib_VC3_25a.zip）を `ThirdParty/DxLib/` に配置。
- `REIGN_OF_DISCS.sln`（ゲーム本体・ReignTests の2プロジェクト、x64、v143、C++17、/W4、/utf-8、/MT・/MTd、マルチバイト）、`Common.props`、`build.ps1`、`.gitignore`。
- ロジック層: `Utils/Types.h`・`Constants.h`、`Board/Cell`・`Board`・`MoveValidator`（FlipRule）、`Systems/TurnManager`・`VictoryManager`・`GameState`・`GameRules`・`GameSession`、`Player/Player`。
- 表示層: `Main.cpp`、`Core/Game`・`SceneManager`・`Features.h`、`Input/InputManager`、`UI/UiConstants.h`・`FontSet`・`DrawUtil`・`BoardRenderer`・`GameUI`、`Scenes/Title`・`Select`・`Game`・`ResultScene`。
- テスト: `Tests/TestFramework.h`・`TestMain.cpp`・`TestClassic.cpp`（11件）。

## 2026-11-10 F002 ストラテジーモード
- 追加: `Systems/ResourceManager`・`FacilityManager`・`AbilityManager`。変更: `GameRules`（収入・防御再計算・期限切れ・自動パス判定）、`GameSession`（TryUseAbility・TryBuild）、`GameScene`（能力・建設メニュー、対象選択、マス情報ツールチップ）、`GameUI`。
- 検証用モード: `Input/InputScript`、`Game`（`--script`・`--shots`）。
- テスト: `Tests/TestStrategy.cpp`（16件）。

## 2026-11-10 F003 AI 対戦
- 追加: `Player/AIPlayer`（EASY・NORMAL・HARD）。変更: `GameScene`（std::async による非同期思考、0.5秒間隔の行動表示、安全な手番終了）、`TestMain.cpp`（SLOW_ テスト）。
- テスト: `Tests/TestAI.cpp`（6件＋SLOW 1件）。

## 2026-11-10 F004 演出・調整・最終テスト
- 追加: `Core/Settings.h`・`AudioManager`、`UI/BoardAnimator`、`Scenes/SettingsScene`、`Resources/Sounds/README.md`、ビルド後の Resources コピー。
- 変更: `BoardRenderer`（演出の上書き描画・配置マスの発光）、`GameUI`（資源のカウントアップ）、`GameScene`（演出中の入力停止・効果音）、各シーン（ボタンの押下表示・効果音・BGM）、`GameSession`（イベントに能力・施設の種類）。
- ドキュメント: `README.md`、本ファイル、`PROJECT_STATUS.md`、`KNOWN_ISSUES.md`。

## 最終テスト結果（2026-11-10）
- Debug・Release の再ビルド: エラー0・警告0。
- ReignTests: Debug 33件・Release 33件すべて成功。SLOW_AC05（Release）成功。

### 原典 17章 テスト仕様
| ID | 内容 | 結果・根拠 |
|----|------|-----------|
| T001 | 初期盤面生成 | 成功（T001_InitialBoard） |
| T002 | 合法手判定 | 成功（T002_InitialLegalMoves、Validator_EdgeEmptyAndAdjacentOwnCases） |
| T003 | 不正な配置 | 成功（T003_IllegalPlacementChangesNothing） |
| T004 | 1方向反転 | 成功（T004_SingleDirectionFlip） |
| T005 | 複数方向反転 | 成功（T005_AllEightDirections：8方向16個） |
| T006 | パス処理 | 成功（T006_AutomaticPass、F002_PassRulesInStrategy） |
| T007 | 両者パス | 成功（T007_BothPassEndsGame） |
| T008 | 全マス占有 | 成功（T008_FullBoardJudgement：勝ち・負け・引き分け） |
| T009 | 資源獲得 | 成功（T009_ResourceIncome、T009_IncomeOnlyAtOwnTurnStartViaSession） |
| T010 | 資源不足 | 成功（T010_ShortageRefusedAndNothingChanges） |
| T011 | 施設建設 | 成功（T011_BuildAllFacilitiesAndEffects） |
| T012 | 施設奪取 | 成功（T012_FacilityChangesOwnerWithStone） |
| T013 | 特殊能力 | 成功（T013_MagicReinforce・ForcedFlip・ResourceMining・SiegeBreak・TacticalPlacement） |
| T014 | AI対戦 | 成功（T014_AllLevelsOnlyLegalActions：3難易度×2ルール、拒否0件） |
| T015 | シーン遷移 | 検証用モードで確認（タイトル→モード選択→ゲーム→結果→もう一度／タイトル、設定、ポーズ→タイトル、遷移直後のクリックが次画面に届かない） |

### 原典 18章 初期バージョンの完成条件
| 項目 | 判定 | 根拠 |
|------|------|------|
| 通常オセロが遊べる | 確認済み（手動確認待ち） | ロジックテスト、検証用モードで2人対戦・AI対戦を終局まで |
| ストラテジーモードが遊べる | 確認済み（手動確認待ち） | ロジックテスト、検証用モードで2人対戦・HARD 対戦を終局まで |
| ローカル2人対戦ができる | 確認済み（手動確認待ち） | 検証用モード |
| AI対戦ができる | 確認済み（手動確認待ち） | T014、SLOW_AC05、検証用モード |
| 資源管理が機能する | 確認済み | T009・T010 |
| 施設建設が機能する | 確認済み | T011・T012、F002_FortDefenceRangeOverlapAndCapture |
| 特殊能力が機能する | 確認済み | T013 |
| 勝敗判定が正しい | 確認済み | T007・T008 |
| タイトルから結果画面まで正常に遷移する | 確認済み | T015（検証用モード） |
| Visual Studioでビルドできる | 確認済み | VS2022 の devenv（IDE）と MSBuild の両方で Debug・Release とも成功、警告0 |
| DXライブラリ以外の不要な外部依存を増やさない | 確認済み | 依存は DXライブラリと C++ 標準ライブラリのみ（テストも自作ランナー） |

### 長時間プレイ
- 検証用モードで AI（EASY）と連続10局（「もう一度」で再戦）を Debug・Release で実行。クラッシュなし、10局とも結果画面に到達。メモリは Release で 約350MB、Debug で 約430MB に達した後は一定（KNOWN_ISSUES 5）。
- テスト用に生成した効果音・BGM（wav）を出力フォルダに一時的に置いた状態で実行し、エラーなく完了（実行後に削除）。

## 2026-11-10 最終テストの追加確認
- VS2022 IDE（devenv）で再構築: Debug・Release とも 2 正常終了、0 失敗、警告0（ワイルドカードを明示的なファイル一覧に変更、`build.ps1` に一覧漏れの検出を追加）。
- 音声: 素材なし→7件すべて missing でも正常動作。テスト用 wav あり→7件読み込み、タイトル・設定・結果で bgm_title、ゲームで bgm_game が再生中、音量設定の変更が即時反映（`audio` レポート）。テスト用 wav は削除済み。
- 引き分けの結果画面を検証用モード（`drawresult`）で表示確認。
- ReignTests 33件成功（再実行）。

## 2026-11-10 分かりやすさの改善（ユーザー要望）
- ゲーム内ヘルプ `UI/HelpOverlay`（基本ルール・盤面の記号の凡例・資源と能力・操作の4ページ）。ゲーム画面の「ヘルプ」ボタン／H キー、ポーズメニュー、タイトルの「遊び方」から開く。
- 画面下に「今やること」のガイド文を常に表示。資源の欄・ボタンにマウスを乗せると用途の説明を表示。
- 盤上の特殊マスの記号を1文字（魔・鉱・祭・拠）から2文字（マナ・鉱石・祭壇・拠点）に変更。ルールブックも同じ表記に更新。
- 確認: Debug・Release ビルド成功、ReignTests 33件成功、検証用モードでヘルプ4ページ・ツールチップ・ガイド文を画面保存で確認。


## 2026-11-10 ランダム配置と戻るボタン（ユーザー要望）
- `Systems/MapGenerator`: 点対称のランダム配置（角・中央4マス・初手のマス8つを除外、上下左右で隣接させない、シードで再現可能）。`GameConfig` に `layout`・`mapSeed`。モード選択に「マップ：ランダム配置／標準配置」（初期値ランダム）。ゲームごとに新しい配置。
- 「戻る」ボタン: 能力・建設メニューの右上と、画面下（メニュー・対象選択中のみ表示）。右クリックでも従来どおり戻れる。
- テスト: F002_RandomLayoutRules（500シードで規則・再現性・多様性）、F002_RandomLayoutGamesPlayable（100局）。ReignTests 35件成功（Debug・Release）。検証用モードで画面確認。


## 2026-11-10 砦のルール変更（ユーザー要望）
- 砦は持ち主の相手のターンが1回終わると消える。砦を取る（通常反転・強制反転）と取った側に追加ターン、砦は取った側のものになり相手の1ターン分守る。
- 魔力強化・破城の期間を、ターン番号ではなく『どちらのターンが終わったか』で数える方式に変更（追加ターン対応）。`TurnManager::EndTurn(passed, extraTurn)`、`GameRules::EndTurn` が `TurnEndInfo` を返す、イベント `FortExpired`・`ExtraTurn`。
- AI: NORMAL は砦を取る手・強制反転を高く評価。HARD は探索で追加ターンを自然に扱う。
- テスト: F002_FortCaptureGivesExtraTurn・F002_FortCaptureByForcedFlip を追加、T011・T013_SiegeBreakDuration を新ルールに更新。ReignTests 37件成功（Debug・Release）。SLOW_AC05: NORMAL 対 EASY 88%/94%、HARD 対 NORMAL 76%/92%。

