# GitHubのLFSストレージから古いオブジェクトを削除するスクリプト (PowerShell版)
# リポジトリの履歴を浅くして、古いLFSオブジェクトへの参照を削除します

Write-Host "=== GitHub LFSストレージ クリーンアップスクリプト ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "⚠️  警告: このスクリプトはリポジトリの履歴を書き換えます" -ForegroundColor Yellow
Write-Host "⚠️  他のコラボレーターと連携してから実行してください" -ForegroundColor Yellow
Write-Host ""

# 現在のブランチを確認
$CURRENT_BRANCH = git branch --show-current
Write-Host "現在のブランチ: $CURRENT_BRANCH" -ForegroundColor Cyan
Write-Host ""

# バックアップの作成を推奨
Write-Host "1. バックアップの作成を推奨します" -ForegroundColor Yellow
$response = Read-Host "続行しますか？ (y/N)"
if ($response -ne "y" -and $response -ne "Y") {
    Write-Host "キャンセルしました" -ForegroundColor Yellow
    exit 0
}
Write-Host ""

# 浅い履歴に変更する日付を指定
Write-Host "2. 浅い履歴に変更する日付を指定してください" -ForegroundColor Yellow
Write-Host "   例: 2025-11-01 (過去1ヶ月のみ保持)" -ForegroundColor Gray
$SHALLOW_DATE = Read-Host "日付を入力 (YYYY-MM-DD)"
Write-Host ""

# 浅い履歴に変更
Write-Host "3. 浅い履歴に変更中..." -ForegroundColor Yellow
git fetch --shallow-since=$SHALLOW_DATE
if ($LASTEXITCODE -ne 0) {
    Write-Host "エラー: 浅い履歴への変更に失敗しました" -ForegroundColor Red
    exit 1
}
Write-Host ""

# ガベージコレクション
Write-Host "4. リポジトリの履歴を最適化中..." -ForegroundColor Yellow
git gc --prune=now --aggressive
Write-Host ""

# リモートに反映
Write-Host "5. リモートに反映しますか？" -ForegroundColor Yellow
Write-Host "   注意: これはforce pushを実行します" -ForegroundColor Yellow
$response = Read-Host "続行しますか？ (y/N)"
if ($response -eq "y" -or $response -eq "Y") {
    Write-Host "リモートにプッシュ中..." -ForegroundColor Yellow
    git push origin $CURRENT_BRANCH --force
    if ($LASTEXITCODE -ne 0) {
        Write-Host "エラー: プッシュに失敗しました" -ForegroundColor Red
        exit 1
    }
    Write-Host "完了しました" -ForegroundColor Green
} else {
    Write-Host "スキップしました。後で手動でプッシュしてください:" -ForegroundColor Yellow
    Write-Host "  git push origin $CURRENT_BRANCH --force" -ForegroundColor Gray
}
Write-Host ""

Write-Host "=== クリーンアップ完了 ===" -ForegroundColor Green
Write-Host ""
Write-Host "次のステップ:" -ForegroundColor Cyan
Write-Host "1. GitHubのリポジトリ設定でLFSストレージの使用状況を確認"
Write-Host "2. 必要に応じて、GitHubのLFSストレージから古いオブジェクトを削除"

