#!/bin/bash
# Git LFS クリーンアップスクリプト
# 古いLFSオブジェクトを削除して、GitHubのLFSストレージ使用量を減らします

set -e

echo "=== Git LFS クリーンアップスクリプト ==="
echo ""

# 現在のLFSファイルの状況を確認
echo "1. 現在のLFSファイルの状況を確認中..."
git lfs ls-files
echo ""

# ローカルの不要なLFSオブジェクトを削除（ドライラン）
echo "2. ローカルの不要なLFSオブジェクトを確認中（ドライラン）..."
git lfs prune --dry-run --verbose || echo "警告: pruneコマンドが失敗しました"
echo ""

# 過去30日間に参照されていないLFSオブジェクトを削除
echo "3. 過去30日間に参照されていないLFSオブジェクトを削除中..."
echo "   注意: これはローカルのみに影響します"
read -p "続行しますか？ (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git lfs prune --verbose || echo "警告: pruneコマンドが失敗しました"
else
    echo "スキップしました"
fi
echo ""

# リポジトリの履歴を最適化
echo "4. リポジトリの履歴を最適化中..."
git gc --prune=now --aggressive || echo "警告: gcコマンドが失敗しました"
echo ""

echo "=== クリーンアップ完了 ==="
echo ""
echo "次のステップ:"
echo "1. GitHubのリポジトリ設定でLFSストレージの使用状況を確認"
echo "2. 必要に応じて、GitHubのLFSストレージから古いオブジェクトを削除"
echo "3. 変更をリモートにプッシュ: git push origin main"

