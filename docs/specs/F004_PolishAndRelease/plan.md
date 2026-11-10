# F004: 演出・調整・最終テスト 設計

- 状態: 確定（2026-11-10）
- 仕様: `spec.md`

## 対象環境
F001 の plan.md と同じ。音声は DXライブラリの `LoadSoundMem`・`PlaySoundMem`・`ChangeVolumeSoundMem`・`DeleteSoundMem` を使う。

## 既存構造（F003 完了時点）
- `GameScene` はイベント（Placed は反転した石の順序付き一覧、AbilityUsed は変化したマス、ResourceGained は増えた量）を受け取ってメッセージに流している。演出はこのイベントから作れる。
- `BoardRenderer::DrawStone(cx, cy, owner, widthScale, scale)` は横幅・大きさを変えて描ける。`BoardView` に描画の上書き情報を足せる。
- `GameUI` は資源の浮き上がり表示（ShowGain）を持つ。`Button::Draw(..., pressed)` は押下時に沈む表示に対応済み。
- `AppContext` に `audio`・`settings` のポインタ枠がある（未使用）。`Features::SETTINGS_SCREEN` は false。
- 検証用モード `--script` で画面を保存できる。`InputManager::LeftDown()` あり。

## 設計
| 対象 | 内容 |
|------|------|
| `Core/Settings.h` | `Settings { int bgmVolume = 70; int seVolume = 80; bool fastAnimation = false; }`（0〜100、10刻み。保存しない） |
| `Core/AudioManager.*` | 起動時に `<exe のフォルダ>/Resources/Sounds/` から決まった名前（拡張子 .wav／.ogg／.mp3 の順に探す）を読み込む。ないファイルは無視。`PlaySe(id)`・`PlayBgm(id)`（ループ、同じ曲なら継続）・`ApplyVolumes(settings)`・`Release()` |
| ビルド後処理 | ゲーム本体のビルド後に `Resources` を出力フォルダへコピー（`xcopy`）。exe の隣の `Resources` を読むので、どこから起動しても同じ |
| `UI/BoardAnimator.*` | 石の演出の時間管理。`OnPlaced(pos, owner, flips)`・`OnChanged(pos, owner)`・`Update(dt)`・`Busy()`・`FillView(BoardView&)`。配置: 0.18 秒で 0.3→1.0 倍に拡大し、置いたマスを 0.6 秒光らせる。反転: 石ごとに 0.05 秒ずつ遅らせ、0.2 秒で横に縮んで色が変わり広がる。設定の「速い」では時間を半分 |
| `BoardView` の拡張 | `overrides`（マス・表示する持ち主・横幅・倍率）、`highlight`（マス・強さ） |
| `GameUI` | 資源の数値を実際の値まで少しずつ増やして表示（0.06 秒ごとに1）。`Update(dt, state)` |
| `GameScene` | 演出中（`BoardAnimator::Busy()`）は人の入力と AI の行動の実行を止める。イベントごとに効果音（配置・反転・資源・エラー） |
| `Scenes/SettingsScene.*` | BGM 音量・効果音音量（−／＋で 10 刻み）、アニメーション速度（通常／速い）、「戻る」。変更はすぐ反映。`Features::SETTINGS_SCREEN = true` |
| 各シーンのボタン | 押している間は沈む表示（`LeftDown()`）、クリックで効果音 |
| `ResultScene` | 勝利・敗北で効果音、BGM はタイトル曲に戻す |

## 変更範囲と依存関係
- 追加: `Core/Settings.h`、`Core/AudioManager.*`、`UI/BoardAnimator.*`、`Scenes/SettingsScene.*`、`Resources/Sounds/README.md`、ルートの `README.md`・`PROJECT_STATUS.md`・`IMPLEMENTATION_LOG.md`・`KNOWN_ISSUES.md`。
- 変更: `Core/Game.*`（音声・設定の初期化と解放）、`UI/BoardRenderer.*`、`UI/GameUI.*`、`Scenes/*`、`REIGN_OF_DISCS.vcxproj`（ビルド後のコピー）、`Core/Features.h`。
- ロジック層（Board・Systems・Player）は変更しない（演出は描画側だけ、R-01）。

## 検証方針
| 受け入れ条件 | 確認方法 |
|--------------|----------|
| AC-01 | 検証用モードで配置直後・反転途中の画面を保存。演出中のクリックで盤面が変わらないこと（演出中にクリック→画面保存） |
| AC-02 | 検証用モード: 勝ち（2人対戦）・負け（AI）・引き分け（ロジックテストで判定、画面はリトライ確認） |
| AC-03 | 検証用モード: 設定画面で値を変えた画面を保存。演出速度の切り替えは BoardAnimator の時間で確認 |
| AC-04 | 音声ファイルなしで全シーンを通す。テスト用に一時的に生成した wav を出力フォルダの `Resources/Sounds` に置いて起動・通しプレイし、終了後に削除 |
| AC-05 | ReignTests（T001〜T014 相当）と、T015（シーン遷移）は検証用モードの画面保存。対応表を tasks.md に記録 |
| AC-06 | 検証用モードで連続 10 局（リトライを含む）を行い、プロセスのメモリ使用量を一定間隔で記録し増え続けないことを確認。全モードの通しプレイはユーザーの手動確認を依頼 |
| AC-07 | 原典 18章の11項目ごとに根拠を `IMPLEMENTATION_LOG.md` と tasks.md に記録 |
| AC-08 | README と進捗3ファイルを作成・更新 |

## リスク・代替案
- 音声ファイルの形式によっては読み込めない → 読み込み失敗は無視し、KNOWN_ISSUES に対応形式（wav／ogg／mp3）を書く。
