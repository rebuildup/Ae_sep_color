#!/bin/bash
# GitHubのLFSストレージから古いオブジェクトを削除するスクリプト
# リポジトリの履歴を浅くして、古いLFSオブジェクトへの参照を削除します

set -e

echo "=== GitHub LFSストレージ クリーンアップスクリプト ==="
echo ""
echo "⚠️  警告: このスクリプトはリポジトリの履歴を書き換えます"
echo "⚠️  他のコラボレーターと連携してから実行してください"
echo ""

# 現在のブランチを確認
CURRENT_BRANCH=$(git branch --show-current)
echo "現在のブランチ: $CURRENT_BRANCH"
echo ""

# バックアップの作成を推奨
echo "1. バックアップの作成を推奨します"
read -p "続行しますか？ (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "キャンセルしました"
    exit 0
fi
echo ""

# 浅い履歴に変更する日付を指定
echo "2. 浅い履歴に変更する日付を指定してください"
echo "   例: 2025-11-01 (過去1ヶ月のみ保持)"
read -p "日付を入力 (YYYY-MM-DD): " SHALLOW_DATE
echo ""

# 浅い履歴に変更
echo "3. 浅い履歴に変更中..."
git fetch --shallow-since=$SHALLOW_DATE
if [ $? -ne 0 ]; then
    echo "エラー: 浅い履歴への変更に失敗しました"
    exit 1
fi
echo ""

# ガベージコレクション
echo "4. リポジトリの履歴を最適化中..."
git gc --prune=now --aggressive
echo ""

# リモートに反映
echo "5. リモートに反映しますか？"
echo "   注意: これはforce pushを実行します"
read -p "続行しますか？ (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "リモートにプッシュ中..."
    git push origin $CURRENT_BRANCH --force
    if [ $? -ne 0 ]; then
        echo "エラー: プッシュに失敗しました"
        exit 1
    fi
    echo "完了しました"
else
    echo "スキップしました。後で手動でプッシュしてください:"
    echo "  git push origin $CURRENT_BRANCH --force"
fi
echo ""

echo "=== クリーンアップ完了 ==="
echo ""
echo "次のステップ:"
echo "1. GitHubのリポジトリ設定でLFSストレージの使用状況を確認"
echo "2. 必要に応じて、GitHubのLFSストレージから古いオブジェクトを削除"

