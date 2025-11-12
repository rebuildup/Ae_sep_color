# ヘッダーファイルをLFSから通常のGitに移行する手順

## 現在の状況

- 14個のヘッダーファイル（.h）がLFSで管理されている（合計約1.8KB）
- これらのファイルは小さなファイルなので、LFSで管理する必要はない
- ビルドには必要だが、LFSストレージの予算を消費している

## 手順

### 1. .gitattributesを更新（完了）

`.gitattributes`を更新して、ヘッダーファイルをLFSから除外しました。

### 2. 既存のLFSファイルを通常のGitに移行

既存のLFSファイルを通常のGitに移行するには、履歴を書き換える必要があります。

#### 方法A: git lfs migrateを使用（推奨）

```bash
# ヘッダーファイルをLFSから通常のGitに移行
git lfs migrate export --include="third_party/halide/include/*.h" --everything

# 変更を確認
git log --oneline -5

# リモートにプッシュ（force pushが必要）
git push origin main --force
```

⚠️ **注意**: `--force`プッシュは履歴を書き換えるため、他のコラボレーターと連携が必要です。

#### 方法B: git filter-repoを使用（上級者向け）

```bash
# git-filter-repoをインストール（未インストールの場合）
pip install git-filter-repo

# ヘッダーファイルをLFSから通常のGitに移行
git filter-repo --path third_party/halide/include --invert-paths
```

### 3. 変更をコミット

```bash
git add .gitattributes
git commit -m "chore: migrate header files from LFS to regular Git"
git push origin main
```

## 期待される効果

- LFSストレージの使用量が約1.8KB削減される
- ヘッダーファイルは通常のGitで管理されるため、フェッチが高速になる
- LFS予算の超過問題が解決される可能性がある

## 注意事項

- 履歴を書き換える操作は、他のコラボレーターに影響を与える可能性があります
- 作業前にリポジトリのバックアップを取ることをお勧めします
- 現在のLFSファイル（Halide.lib、Halide.dll、libHalide.21.0.0.dylib）は削除されません

