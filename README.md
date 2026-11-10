# REIGN OF DISCS — 盤上の覇権

オセロ × 領土戦略のターン制ボードゲーム（C++ / DXライブラリ / Windows）。
仕様の原典は `docs/REIGN_OF_DISCS_ゲーム仕様書.docx`、確定した仕様・設計・タスクは `docs/specs/`（MySpecKit 形式）にあります。

## ルールブック
- オンライン版: https://claude.ai/artifact/N84ZZAVQjUD1CrjRXVyW2M （ルートの `3 ルールブック（オンライン）.url` をダブルクリックでも開けます）
- ローカル版: `docs/ルールブック.html`（ブラウザで開けます）

## 動作環境
- Windows 10 / 11
- Visual Studio 2022（「C++ によるデスクトップ開発」ワークロード、ツールセット v143）
- DXライブラリ 3.25a（`ThirdParty/DxLib/` に、このプロジェクトが使うヘッダーと x64 用ライブラリだけを同梱。別途インストール不要）

## ビルドと起動
- Visual Studio 2022 で `REIGN_OF_DISCS.sln` を開き、構成 `Debug` または `Release`・プラットフォーム `x64` でビルドして、`REIGN_OF_DISCS` をスタートアップにして実行します。
- コマンドラインの場合（ビルドとロジックテスト）:
  ```powershell
  powershell -ExecutionPolicy Bypass -File build.ps1                         # Debug + テスト
  powershell -ExecutionPolicy Bypass -File build.ps1 -Configuration Release  # Release + テスト
  ```
- 実行ファイル: `x64\<構成>\REIGN_OF_DISCS.exe`（ビルド時に `Resources` が隣にコピーされます）

## 遊び方
1. タイトルの「ゲーム開始」→ ルール（通常オセロ／ストラテジー）と対戦相手（ローカル2人対戦／AI EASY・NORMAL・HARD）を選んで「ゲーム開始」。
2. 黒（プレイヤー）が先手。合法手のマスに小さな点が出ます。左クリックで石を置きます。
3. 置ける場所がなければ自動でパスします（ストラテジーでは、建設・能力が使える間は自動パスせず「パス」ボタンで終えます）。
4. 盤面が埋まるか両者が続けてパスすると終了し、石の多い方が勝ちです。

| 操作 | 内容 |
|------|------|
| 左クリック | マスの選択・石の配置、ボタン |
| 右クリック／「戻る」ボタン | 選択の解除・1段階戻る（能力・建設のメニューや対象選択中は「戻る」ボタンも表示） |
| マウスを乗せる | マスの座標（ストラテジーではマスの種類・所有者・施設・防御・収入） |
| ESC | メニューを閉じる／ポーズ（続ける・ヘルプ・タイトルへ戻る） |
| H／ヘルプボタン | ゲーム内ヘルプ（基本ルール・盤面の記号・資源と能力・操作）。タイトルの「遊び方」からも開ける |

### ストラテジーモード
- 特殊マス（モード選択の「マップ」で、ゲームごとに変わる「ランダム配置」か、固定の「標準配置」を選べます。ランダムでも点対称で、角・中央付近には置かれません）: マナ鉱脈（マナ+2）×4、鉱石鉱脈（鉱石+2）×4、祭壇マス（マナ+1・影響力+1）×2、中立拠点（影響力+1）×4。自分の石があるマスから、**自分のターン開始時**に資源を得ます（上限: マナ15・鉱石15・影響力10）。
- 建設（1ターン1回）: 採掘所（鉱石3）／魔力炉（鉱石2・マナ2）／砦（鉱石4）／祭壇（鉱石3・影響力1）。施設は石と一緒に持ち主が変わります。
- 砦: 建てた直後の相手の1ターンだけ、上下左右の自分の石を「防御状態」にし、その後は消えます。砦のマスの石を取ると、取った側がもう1ターン行えます（砦も取った側のものになり、相手の1ターン分守ります）。防御・強化中の石は挟んでも裏返りません（挟む列には含められます）。
- 特殊能力（1ターン1回）: 魔力強化（マナ3）・強制反転（マナ5）・資源採掘（鉱石+2、3回まで）・破城（影響力3）・戦術的配置（影響力5、1回まで）。
- 能力・建設のあとでも石を置けます。石を置くとターンが終わります。

## 効果音・BGM
素材は同梱していません。`REIGN_OF_DISCS/Resources/Sounds/` に決まった名前のファイルを置くと鳴ります（詳細は同フォルダの `README.md`）。音量とアニメーション速度はタイトルの「設定」で変えられます（保存はしません）。

## テスト
- ロジックテスト: `x64\<構成>\ReignTests.exe`（DXライブラリに依存しないテスト。終了コード 0 で全成功）
- 重いテスト（AI の強さの自己対戦）: `x64\Release\ReignTests.exe SLOW_`
- 検証用モード（画面の自動確認用。OS のマウス・キーボードは使わず、ウィンドウも表示しない）:
  `REIGN_OF_DISCS.exe --script <台本ファイル> --shots <画像の出力フォルダ>`（台本の書式は `Source/Input/InputScript.h`）

## フォルダ構成
```text
REIGN_OF_DISCS.sln / Common.props / build.ps1
REIGN_OF_DISCS/            ゲーム本体（Source/、Resources/）
  Source/Board, Systems, Player, Utils   ロジック層（DXライブラリ非依存）
  Source/Core, Scenes, UI, Input         表示・入力・シーン（DXライブラリ使用）
Tests/                     ロジックテスト
ThirdParty/DxLib/          DXライブラリ
docs/specs/                仕様・設計・タスク（F001〜F004）
PROJECT_STATUS.md / IMPLEMENTATION_LOG.md / KNOWN_ISSUES.md
```

## ライセンス表記
DX Library Copyright (C) 2001-2025 Takumi Yamada.
同梱ライブラリ（libjpeg・libpng・zlib・Ogg Vorbis・Opus・Bullet など）の著作権表示は `ThirdParty/DxLib/DxLib.txt` を参照してください。
