#!/bin/sh
#
# 迁移脚本：从 VERSION 文件迁移到 Git Tag 版本管理
#

echo "=========================================="
echo "迁移到 Git Tag 版本管理"
echo "=========================================="
echo ""

# 检查 VERSION 文件是否存在
if [ -f "VERSION" ]; then
    VERSION_FROM_FILE=$(cat VERSION | tr -d ' \t\r\n')
    echo "当前 VERSION 文件内容: $VERSION_FROM_FILE"
    
    # 检查版本格式
    if echo "$VERSION_FROM_FILE" | grep -qE "^[0-9]+\.[0-9]+\.[0-9]+$"; then
        # 获取最新的 tag
        LATEST_TAG=$(git tag -l --sort=-version:refname | head -1)
        
        if [ -n "$LATEST_TAG" ]; then
            # 移除 v 前缀
            LATEST_TAG_VERSION=$(echo "$LATEST_TAG" | sed 's/^v//')
            echo "当前最新 tag: $LATEST_TAG ($LATEST_TAG_VERSION)"
            
            # 比较版本
            if [ "$VERSION_FROM_FILE" != "$LATEST_TAG_VERSION" ]; then
                echo ""
                echo "警告: VERSION 文件 ($VERSION_FROM_FILE) 与最新 tag ($LATEST_TAG_VERSION) 不一致"
                echo ""
                read -p "是否创建新 tag v$VERSION_FROM_FILE 来同步版本? (y/n) " -n 1 -r
                echo ""
                if [[ $REPLY =~ ^[Yy]$ ]]; then
                    NEW_TAG="v$VERSION_FROM_FILE"
                    if git rev-parse "$NEW_TAG" >/dev/null 2>&1; then
                        echo "Tag $NEW_TAG 已存在，跳过创建"
                    else
                        git tag "$NEW_TAG" HEAD
                        echo "已创建 tag: $NEW_TAG"
                    fi
                fi
            else
                echo "版本已同步，无需操作"
            fi
        else
            # 没有 tag，创建初始 tag
            echo "未找到 tag，创建初始 tag: v$VERSION_FROM_FILE"
            git tag "v$VERSION_FROM_FILE" HEAD
        fi
    else
        echo "错误: VERSION 文件格式无效，应为 MAJOR.MINOR.PATCH"
        exit 1
    fi
else
    echo "未找到 VERSION 文件"
    echo "检查现有 tag..."
    LATEST_TAG=$(git tag -l --sort=-version:refname | head -1)
    if [ -n "$LATEST_TAG" ]; then
        echo "当前最新 tag: $LATEST_TAG"
    else
        echo "未找到 tag，创建初始 tag: v0.0.0"
        git tag v0.0.0 HEAD
    fi
fi

echo ""
echo "=========================================="
echo "迁移完成"
echo "=========================================="
echo ""
echo "下一步："
echo "1. 推送 tag 到远程: git push --tags"
echo "2. (可选) 删除 VERSION 文件: git rm VERSION && git commit -m 'chore: remove VERSION file'"
echo "3. 编译项目验证版本号是否正确"
echo ""

