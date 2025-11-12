# ヘッダーファイルをLFSから通常のGitに移行するスクリプト (PowerShell版)

Write-Host "=== ヘッダーファイルをLFSから通常のGitに移行 ===" -ForegroundColor Cyan
Write-Host ""

# 現在のLFSファイルの状況を確認
Write-Host "1. 現在のLFSファイルの状況を確認中..." -ForegroundColor Yellow
$headerFiles = git lfs ls-files | Select-String "\.h$"
if ($headerFiles) {
    Write-Host "  見つかったヘッダーファイル:" -ForegroundColor Gray
    $headerFiles | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
} else {
    Write-Host "  ヘッダーファイルが見つかりません" -ForegroundColor Yellow
}
Write-Host ""

# ヘッダーファイルをLFSから削除
Write-Host "2. ヘッダーファイルをLFSから削除中..." -ForegroundColor Yellow
if ($headerFiles) {
    $headerFiles | ForEach-Object {
        $file = ($_ -split '\s+')[2]
        if ($file) {
            Write-Host "  Removing from LFS: $file" -ForegroundColor Gray
            git lfs untrack $file
        }
    }
} else {
    Write-Host "  ヘッダーファイルが見つかりません" -ForegroundColor Yellow
}
Write-Host ""

# .gitattributesを更新（既に更新済み）
Write-Host "3. .gitattributesを確認中..." -ForegroundColor Yellow
if (Select-String -Path .gitattributes -Pattern "third_party/halide/include/\*.h -filter") {
    Write-Host "  .gitattributesは既に更新されています" -ForegroundColor Green
} else {
    Write-Host "  警告: .gitattributesが更新されていません" -ForegroundColor Yellow
}
Write-Host ""

# 変更をコミット
Write-Host "4. 変更をコミットしますか？" -ForegroundColor Yellow
$response = Read-Host "続行しますか？ (y/N)"
if ($response -eq "y" -or $response -eq "Y") {
    git add .gitattributes
    git commit -m "chore: migrate header files from LFS to regular Git"
    Write-Host "コミット完了" -ForegroundColor Green
} else {
    Write-Host "スキップしました。手動でコミットしてください:" -ForegroundColor Yellow
    Write-Host "  git add .gitattributes" -ForegroundColor Gray
    Write-Host "  git commit -m 'chore: migrate header files from LFS to regular Git'" -ForegroundColor Gray
}
Write-Host ""

Write-Host "=== 移行完了 ===" -ForegroundColor Green
Write-Host ""
Write-Host "次のステップ:" -ForegroundColor Cyan
Write-Host "1. 変更をリモートにプッシュ: git push origin main"
Write-Host "2. GitHubのLFSストレージの使用状況を確認"

