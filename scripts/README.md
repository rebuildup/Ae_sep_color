# Git LFS クリーンアップスクリプト

## 概要

このスクリプトは、GitHubのLFSストレージ予算を超過している問題を解決するために、古いLFSオブジェクトを削除します。

## 使用方法

### Windows (PowerShell)

```powershell
.\scripts\cleanup_lfs.ps1
```

### Linux/macOS (Bash)

```bash
chmod +x scripts/cleanup_lfs.sh
./scripts/cleanup_lfs.sh
```

## 重要な注意事項

⚠️ **現在のLFSファイル（Halide.lib、Halide.dll、libHalide.21.0.0.dylib）は削除されません**。これらはビルドに必要です。

## 手順

### 1. ローカルのクリーンアップ

スクリプトを実行すると、以下の処理が行われます：

1. 現在のLFSファイルの状況を確認
2. ローカルの不要なLFSオブジェクトを確認（ドライラン）
3. 過去30日間に参照されていないLFSオブジェクトを削除
4. リポジトリの履歴を最適化

### 2. GitHubのLFSストレージから古いオブジェクトを削除

ローカルのクリーンアップだけでは、GitHubのLFSストレージの使用量は減りません。以下の方法で削除できます：

#### 方法A: リポジトリの履歴を浅くする（推奨）

```bash
# 浅い履歴に変更（過去1ヶ月のみ保持）
git fetch --shallow-since=2025-11-01
git gc --prune=now
git push origin main --force
```

⚠️ **注意**: `--force`プッシュは履歴を書き換えるため、他のコラボレーターと連携が必要です。

#### 方法B: GitHubのWebインターフェースから削除

1. GitHubのリポジトリページにアクセス
2. Settings → Billing → Git LFS Data
3. 使用状況を確認し、不要なオブジェクトを削除

#### 方法C: GitHub APIを使用

```bash
# LFSオブジェクトの一覧を取得
curl -H "Authorization: token YOUR_TOKEN" \
  https://github.com/rebuildup/Ae_sep_color.git/info/lfs/objects
```

## 現在のLFSファイル

以下のファイルは**削除しないでください**：

- `third_party/halide/win/lib/Halide.lib` (約96MB)
- `third_party/halide/win/bin/Halide.dll` (約139MB)
- `third_party/halide/mac/lib/libHalide.21.0.0.dylib` (約206MB)

合計: 約441MB

## トラブルシューティング

### LFS予算がまだ超過している場合

1. GitHubのLFSストレージの使用状況を確認
2. 古いリポジトリや不要なLFSファイルを削除
3. LFSストレージの予算を増やす（GitHubの設定から）

### ビルドが失敗する場合

LFSファイルが取得できない場合、ビルドは失敗します。以下のいずれかを実行してください：

1. LFSストレージの予算を増やす
2. LFSファイルを別の方法で管理（例：GitHub Releases）

## 参考資料

- [Git LFS ドキュメント](https://git-lfs.github.com/)
- [GitHub LFS ドキュメント](https://docs.github.com/ja/repositories/working-with-files/managing-large-files)

