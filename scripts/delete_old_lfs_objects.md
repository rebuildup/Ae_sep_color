# GitHub LFSストレージから古いオブジェクトを削除する手順

## 問題

GitHubのLFSストレージ予算が超過しており、必要なバイナリファイル（Halide.lib、Halide.dll、libHalide.21.0.0.dylib）が取得できません。

## 解決方法

### 方法1: GitHubのWebインターフェースから削除（推奨）

1. GitHubのリポジトリページにアクセス
2. **Settings** → **Billing** → **Git LFS Data**
3. 使用状況を確認し、不要なオブジェクトを削除

### 方法2: リポジトリの履歴を浅くする

リポジトリの履歴を浅くすることで、古いLFSオブジェクトへの参照を削除できます。

```bash
# 過去1ヶ月のみ保持
git fetch --shallow-since=2025-11-01
git gc --prune=now --aggressive
git push origin main --force
```

⚠️ **注意**: `--force`プッシュは履歴を書き換えるため、他のコラボレーターと連携が必要です。

### 方法3: git lfs migrateを使用

```bash
# すべてのLFSファイルを確認
git lfs ls-files

# ヘッダーファイルをLFSから通常のGitに移行（既に完了）
# バイナリファイルは保持

# 履歴を最適化
git gc --prune=now --aggressive
```

### 方法4: LFSストレージの予算を増やす

1. GitHubのリポジトリページにアクセス
2. **Settings** → **Billing** → **Git LFS Data**
3. 予算を増やす

## 現在のLFSファイル

以下のファイルは**削除しないでください**（ビルドに必要）：

- `third_party/halide/win/lib/Halide.lib` (約96MB)
- `third_party/halide/win/bin/Halide.dll` (約139MB)
- `third_party/halide/mac/lib/libHalide.21.0.0.dylib` (約206MB)

合計: 約441MB

## 推奨される手順

1. **GitHubのLFSストレージの使用状況を確認**
   - Settings → Billing → Git LFS Data

2. **不要なLFSオブジェクトを削除**
   - 他のリポジトリで使用されていない古いオブジェクトを削除

3. **または、LFSストレージの予算を増やす**
   - 現在のバイナリファイル（約441MB）を保持するために予算を増やす

4. **変更をプッシュ**
   ```bash
   git add .gitattributes
   git commit -m "chore: exclude header files from LFS"
   git push origin main
   ```

## 注意事項

- 現在のLFSファイル（Halide.lib、Halide.dll、libHalide.21.0.0.dylib）は削除しないでください
- 履歴を書き換える操作は、他のコラボレーターに影響を与える可能性があります
- 作業前にリポジトリのバックアップを取ることをお勧めします

