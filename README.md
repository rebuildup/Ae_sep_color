# sep_color

sep_color は After Effects SDK (AeSDK) をベースにした色分離エフェクトプラグインです。直線または円形の境界を解析的に計算し、滑らかなアンチエイリアス付きで指定色へ遷移させます。AeSDK の Template ディレクトリにそのまま配置して使うことを前提に構成しています。

## 目次
- [sep\_color](#sep_color)
  - [目次](#目次)
  - [概要](#概要)
  - [主な機能](#主な機能)
  - [動作環境](#動作環境)
  - [AeSDK への組み込み手順](#aesdk-への組み込み手順)
  - [ビルド](#ビルド)
    - [Windows (Visual Studio)](#windows-visual-studio)
    - [macOS (Xcode)](#macos-xcode)
    - [生成物](#生成物)
    - [GitHub Actions と AeSDK](#github-actions-と-aesdk)
  - [After Effects での利用方法](#after-effects-での利用方法)
  - [エフェクトパラメータ](#エフェクトパラメータ)
  - [実装メモ](#実装メモ)
  - [ライセンス](#ライセンス)
  - [バージョン情報](#バージョン情報)

---

## 概要
- 直線 (Line) / 円 (Circle) の境界で画面を2色に分離
- 距離ベースの解析的アンチエイリアスで滑らかな境界を生成
- 8-bit / 16-bit ワークフローをテンプレートベースで共通処理
- マルチスレッド並列化を行いフルHDで数 ms レベルの処理時間を目標
- 無料配布中（Windows/macOSビルド、旧版含む）で、商用利用は自由・クレジット不要のMITライセンスに準拠します。[参考](https://361do.booth.pm/items/6520211)

## 主な機能
- **Line Mode**: 任意角度の直線で画面を分割し、指定領域のみ色変換
- **Circle Mode**: 任意中心・半径の円形領域に色変換を適用
- **Edge Width**: 境界幅を制御し、0〜広いグラデーションまで調整
- **Blend Amount**: 元の映像と変換色のブレンド量を調整
- **解析的 AA**: サンプリングレスの距離計算でAE標準と同等の品質を実現

## 動作環境
| 項目 | 最低要件 | 推奨 |
| ---- | -------- | ---- |
| After Effects | 2022以降 | 2025 (開発/検証環境) |
| OS | Windows 10 / macOS 12 | Windows 11 (開発環境) |
| 開発ツール | Visual Studio 2022 / Xcode 14 | Cursor + Codex を併用 |
| CPU | SSE4.1 対応 | AVX2 対応マルチコア |

> 実際の開発・検証は `After Effects 2025 + Windows 11 + Cursor / Codex` の構成で行っています。macOS 版はビルド済みですが実機検証をまだ完了できていません（検証予定）。[参考](https://361do.booth.pm/items/6520211)

## AeSDK への組み込み手順
sep_color は AeSDK に同梱されるテンプレートと同じフォルダ構成を前提にしています。必ず **AeSDK の `Examples/Effect/Template/` ディレクトリ直下**に配置してください。

1. Adobe 公式サイトから対象バージョンの AeSDK を取得・展開します。
2. 展開先の `Examples/Effect/Template/` を開きます。
3. リポジトリの `sep_color` フォルダをテンプレート直下へ丸ごとコピーします。
   - 例: `C:\Adobe\AfterEffectsSDK\Examples\Effect\Template\sep_color\`
4. Visual Studio / Xcode から既存テンプレートと同様にプロジェクト/ソリューションを開きます。

> ⚠️ テンプレートディレクトリ以外に置くと `../../../Headers` など AeSDK 既定パスが解決できず、ビルドスクリプトも失敗します。

## ビルド

### Windows (Visual Studio)
```powershell
cd Examples\Effect\Template\sep_color\Win
msbuild sep_color.sln /p:Configuration=Release /p:Platform=x64
```
推奨オプション: `/O2 /GL /arch:AVX2 /fp:fast /MP`

### macOS (Xcode)
```bash
cd Examples/Effect/Template/sep_color/Mac
xcodebuild -project sep_color.xcodeproj -configuration Release
```
推奨フラグ: `-O3 -ffast-math -march=native`

### 生成物
- Windows: `Win/x64/Release/sep_color.aex`
- macOS: `Mac/build/Release/sep_color.plugin`

### GitHub Actions と AeSDK
CI では AeSDK をパブリックに配布できないため、**プライベートリポジトリにミラーした AeSDK** を使用し、GitHub Actions から以下のように取得しています。

1. プライベートリポジトリに AeSDK を配置
2. GitHub リポジトリの Secrets（例: `AESDK_REPO`, `AESDK_TOKEN`）を設定
3. ワークフロー内で `actions/checkout` や `git clone https://$AESDK_TOKEN@github.com/...` の形で取得
4. `Examples/Effect/Template/` へ展開してからビルドステップを実行

> Secrets には Personal Access Token など権限付きの値を格納し、ログに出力しないよう注意してください。

## After Effects での利用方法
1. 上記ビルドで生成されたプラグインを After Effects の `Plug-ins` フォルダへコピーします。
   - Windows: `C:\Program Files\Adobe\Adobe After Effects <version>\Support Files\Plug-ins\`
   - macOS: `/Applications/Adobe After Effects <version>/Plug-ins/`
2. After Effects を再起動すると、`エフェクト > Stylize` など指定カテゴリに `sep_color` が表示されます。
3. 対象レイヤーに適用し、パラメータを調整して利用します。

## エフェクトパラメータ
| パラメータ | 説明 |
| ---------- | ---- |
| Anchor Point | Line モードでは境界が通る基準点、Circle モードでは中心座標として利用。 |
| Mode | `Line` / `Circle` を選択。デフォルトは Line。 |
| Angle | Line モード時の境界角度 (度数法)。Circle モードでは無視されます。 |
| Radius | Circle モード時の半径（AEピクセル単位）。Line モードでは無視されます。 |
| Color | 変換先の RGB 色。アルファは入力値を維持しつつカラーのみ置換/ブレンド。 |

> アンチエイリアスやブレンド幅は常時有効のためパラメータ化していません。README 旧版に記載の `Edge Width` や `Blend Amount` は存在しません。

## 実装メモ
- **解析的アンチエイリアス**: FXAA 研究をベースに、境界からの符号付き距離を用いたカバレッジ計算でサンプリングを完全排除。
- **ディープカラー対応**: `PixelTraits<T>` テンプレートで 8/16-bit を同一ロジックで処理し、`PF_WORLD_IS_DEEP` で実行時切替。
- **マルチスレッド**: 行単位タイル分割と SDK 標準の iterate API を組み合わせ、8 コアで 6〜7 倍、16 コアで 10 倍近い高速化を想定。
- **メモリアクセス最適化**: プレマルチ値・半径逆数などを行/フレーム単位で事前計算し、帯域と除算コストを削減。

## ライセンス
- 本リポジトリは **MIT License** で提供します。
- After Effects SDK の利用規約・配布条件に従い、Adobe のライセンス条項を順守してください。

## バージョン情報
| 日付 | 内容 |
| ---- | ---- |
| 2025-01-23 | sep_color 公開 |
| 2025-11-14 | アンチエイリアス対応 / Mac 版追加 / 処理速度改善 |

ダウンロードアーカイブ（Win/Mac/旧版）は BOOTH ページで随時更新されます。[参考](https://361do.booth.pm/items/6520211)
