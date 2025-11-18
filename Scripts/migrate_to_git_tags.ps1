# 迁移脚本：从 VERSION 文件迁移到 Git Tag 版本管理 (PowerShell版本)

Write-Host "=========================================="
Write-Host "迁移到 Git Tag 版本管理"
Write-Host "=========================================="
Write-Host ""

# 检查 VERSION 文件是否存在
if (Test-Path "VERSION") {
    $VERSION_FROM_FILE = (Get-Content "VERSION" -Raw).Trim()
    Write-Host "当前 VERSION 文件内容: $VERSION_FROM_FILE"
    
    # 检查版本格式
    if ($VERSION_FROM_FILE -match '^\d+\.\d+\.\d+$') {
        # 获取最新的 tag
        $tags = git tag -l --sort=-version:refname
        $LATEST_TAG = if ($tags) { $tags[0] } else { $null }
        
        if ($LATEST_TAG) {
            # 移除 v 前缀
            $LATEST_TAG_VERSION = $LATEST_TAG -replace '^v', ''
            Write-Host "当前最新 tag: $LATEST_TAG ($LATEST_TAG_VERSION)"
            
            # 比较版本
            if ($VERSION_FROM_FILE -ne $LATEST_TAG_VERSION) {
                Write-Host ""
                Write-Host "警告: VERSION 文件 ($VERSION_FROM_FILE) 与最新 tag ($LATEST_TAG_VERSION) 不一致"
                Write-Host ""
                $response = Read-Host "是否创建新 tag v$VERSION_FROM_FILE 来同步版本? (y/n)"
                if ($response -eq 'y' -or $response -eq 'Y') {
                    $NEW_TAG = "v$VERSION_FROM_FILE"
                    $tagExists = git rev-parse "$NEW_TAG" 2>$null
                    if ($LASTEXITCODE -eq 0) {
                        Write-Host "Tag $NEW_TAG 已存在，跳过创建"
                    } else {
                        git tag "$NEW_TAG" HEAD
                        Write-Host "已创建 tag: $NEW_TAG"
                    }
                }
            } else {
                Write-Host "版本已同步，无需操作"
            }
        } else {
            # 没有 tag，创建初始 tag
            Write-Host "未找到 tag，创建初始 tag: v$VERSION_FROM_FILE"
            git tag "v$VERSION_FROM_FILE" HEAD
        }
    } else {
        Write-Host "错误: VERSION 文件格式无效，应为 MAJOR.MINOR.PATCH"
        exit 1
    }
} else {
    Write-Host "未找到 VERSION 文件"
    Write-Host "检查现有 tag..."
    $tags = git tag -l --sort=-version:refname
    $LATEST_TAG = if ($tags) { $tags[0] } else { $null }
    if ($LATEST_TAG) {
        Write-Host "当前最新 tag: $LATEST_TAG"
    } else {
        Write-Host "未找到 tag，创建初始 tag: v0.0.0"
        git tag v0.0.0 HEAD
    }
}

Write-Host ""
Write-Host "=========================================="
Write-Host "迁移完成"
Write-Host "=========================================="
Write-Host ""
Write-Host "下一步："
Write-Host "1. 推送 tag 到远程: git push --tags"
Write-Host "2. (可选) 删除 VERSION 文件: git rm VERSION; git commit -m 'chore: remove VERSION file'"
Write-Host "3. 编译项目验证版本号是否正确"
Write-Host ""

