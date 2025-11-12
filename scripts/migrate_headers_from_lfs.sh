#!/bin/bash
# ヘッダーファイルをLFSから通常のGitに移行するスクリプト

set -e

echo "=== ヘッダーファイルをLFSから通常のGitに移行 ==="
echo ""

# 現在のLFSファイルの状況を確認
echo "1. 現在のLFSファイルの状況を確認中..."
git lfs ls-files | grep "\.h$" || echo "ヘッダーファイルが見つかりません"
echo ""

# ヘッダーファイルをLFSから削除
echo "2. ヘッダーファイルをLFSから削除中..."
HEADER_FILES=$(git lfs ls-files | grep "\.h$" | awk '{print $3}')
if [ -z "$HEADER_FILES" ]; then
    echo "ヘッダーファイルが見つかりません"
else
    for file in $HEADER_FILES; do
        echo "  Removing from LFS: $file"
        git lfs untrack "$file"
    done
fi
echo ""

# .gitattributesを更新（既に更新済み）
echo "3. .gitattributesを確認中..."
if grep -q "third_party/halide/include/\*.h -filter" .gitattributes; then
    echo "  .gitattributesは既に更新されています"
else
    echo "  警告: .gitattributesが更新されていません"
fi
echo ""

# 変更をコミット
echo "4. 変更をコミットしますか？"
read -p "続行しますか？ (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git add .gitattributes
    git commit -m "chore: migrate header files from LFS to regular Git"
    echo "コミット完了"
else
    echo "スキップしました。手動でコミットしてください:"
    echo "  git add .gitattributes"
    echo "  git commit -m 'chore: migrate header files from LFS to regular Git'"
fi
echo ""

echo "=== 移行完了 ==="
echo ""
echo "次のステップ:"
echo "1. 変更をリモートにプッシュ: git push origin main"
echo "2. GitHubのLFSストレージの使用状況を確認"

