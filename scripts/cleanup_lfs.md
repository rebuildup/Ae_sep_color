# Git LFS クリーンアップ手順

## 現在の状況
- リポジトリには3つのLFSファイルが管理されています：
  - `third_party/halide/win/lib/Halide.lib` (約96MB)
  - `third_party/halide/win/bin/Halide.dll` (約139MB)
  - `third_party/halide/mac/lib/libHalide.21.0.0.dylib` (約206MB)
- 合計: 約441MB

## 手順1: ローカルの不要なLFSオブジェクトを削除

```bash
# 現在のLFSオブジェクトの使用状況を確認
git lfs ls-files

# 過去30日間に参照されていないLFSオブジェクトを削除（ドライラン）
git lfs prune --dry-run --verbose

# 実際に削除（注意：これはローカルのみ）
git lfs prune --verbose
```

## 手順2: リポジトリの履歴から古いLFSファイルを削除

### オプションA: 浅い履歴に変更（推奨）

```bash
# 現在のブランチを浅い履歴に変更
git fetch --shallow-since=2025-11-01
git gc --prune=now
```

### オプションB: git filter-repoを使用（上級者向け）

```bash
# git-filter-repoをインストール（未インストールの場合）
# pip install git-filter-repo

# 古いLFSファイルを履歴から削除
# 注意：これは履歴を書き換えます
git filter-repo --path third_party/halide/ --invert-paths
```

## 手順3: GitHubのLFSストレージから古いオブジェクトを削除

GitHubのLFSストレージから古いオブジェクトを削除するには、以下の方法があります：

1. **GitHubのWebインターフェースから**:
   - リポジトリの Settings → Billing → Git LFS Data
   - 使用状況を確認し、不要なオブジェクトを削除

2. **GitHub APIを使用**:
   ```bash
   # LFSオブジェクトの一覧を取得
   curl -H "Authorization: token YOUR_TOKEN" \
     https://github.com/rebuildup/Ae_sep_color.git/info/lfs/objects
   ```

## 手順4: リモートに反映

```bash
# 履歴を書き換えた場合は、force pushが必要
git push origin main --force
```

## 注意事項

- **現在のLFSファイルは削除しないでください**。これらはビルドに必要です。
- 履歴を書き換える操作は、他のコラボレーターに影響を与える可能性があります。
- 作業前にリポジトリのバックアップを取ることをお勧めします。

## 代替案

LFSストレージの予算を増やすことも検討してください：
- GitHubの設定 → Billing → Git LFS Data
- 予算を増やすことで、現在のLFSファイルを保持できます

