# Git LFS クリーンアップスクリプト (PowerShell版)
# 古いLFSオブジェクトを削除して、GitHubのLFSストレージ使用量を減らします

Write-Host "=== Git LFS クリーンアップスクリプト ===" -ForegroundColor Cyan
Write-Host ""

# 現在のLFSファイルの状況を確認
Write-Host "1. 現在のLFSファイルの状況を確認中..." -ForegroundColor Yellow
git lfs ls-files
Write-Host ""

# ローカルの不要なLFSオブジェクトを削除（ドライラン）
Write-Host "2. ローカルの不要なLFSオブジェクトを確認中（ドライラン）..." -ForegroundColor Yellow
git lfs prune --dry-run --verbose
if ($LASTEXITCODE -ne 0) {
    Write-Host "警告: pruneコマンドが失敗しました" -ForegroundColor Yellow
}
Write-Host ""

# 過去30日間に参照されていないLFSオブジェクトを削除
Write-Host "3. 過去30日間に参照されていないLFSオブジェクトを削除中..." -ForegroundColor Yellow
Write-Host "   注意: これはローカルのみに影響します" -ForegroundColor Yellow
$response = Read-Host "続行しますか？ (y/N)"
if ($response -eq "y" -or $response -eq "Y") {
    git lfs prune --verbose
    if ($LASTEXITCODE -ne 0) {
        Write-Host "警告: pruneコマンドが失敗しました" -ForegroundColor Yellow
    }
} else {
    Write-Host "スキップしました" -ForegroundColor Yellow
}
Write-Host ""

# リポジトリの履歴を最適化
Write-Host "4. リポジトリの履歴を最適化中..." -ForegroundColor Yellow
git gc --prune=now --aggressive
if ($LASTEXITCODE -ne 0) {
    Write-Host "警告: gcコマンドが失敗しました" -ForegroundColor Yellow
}
Write-Host ""

Write-Host "=== クリーンアップ完了 ===" -ForegroundColor Green
Write-Host ""
Write-Host "次のステップ:" -ForegroundColor Cyan
Write-Host "1. GitHubのリポジトリ設定でLFSストレージの使用状況を確認"
Write-Host "2. 必要に応じて、GitHubのLFSストレージから古いオブジェクトを削除"
Write-Host "3. 変更をリモートにプッシュ: git push origin main"

