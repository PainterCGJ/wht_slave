# 版本管理说明

## 概述

本项目使用 **Git Tag** 自动管理版本信息。版本信息存储在 Git 标签中，根据 commit message 自动创建/更新标签，并在编译时自动读取标签信息写入固件。

## 工作流程

1. **提交代码时**：`commit-msg` hook 分析 commit message，根据类型自动计算新版本号
2. **提交后**：`post-commit` hook 自动创建新的 git tag
3. **推送前**：`pre-push` hook 检查并提示推送 tag
4. **编译时**：CMakeLists.txt 从 git tag 读取版本号并写入固件

## Commit Message 格式

使用 [Conventional Commits](https://www.conventionalcommits.org/) 格式：

| Commit类型 | 版本变化 | 示例 |
|-----------|---------|------|
| `feat:` | MINOR增加 (1.0.0 → 1.1.0) | `feat: 添加新功能` |
| `fix:` / `bugfix:` / `patch:` | PATCH增加 (1.0.0 → 1.0.1) | `fix: 修复bug` |
| `BREAKING CHANGE:` / `feat!:` | MAJOR增加 (1.0.0 → 2.0.0) | `feat!: 重构API` |
| 其他 | PATCH增加 (1.0.0 → 1.0.1) | `docs: 更新文档` |

## 使用示例

### 1. 新功能提交

```bash
git commit -m "feat: 添加FreeRTOS内存管理支持"
# 自动创建tag: v0.1.0 (假设当前最新tag是 v0.0.0)
```

### 2. 修复提交

```bash
git commit -m "fix: 修复内存泄漏问题"
# 自动创建tag: v0.1.1 (假设当前最新tag是 v0.1.0)
```

### 3. 破坏性变更

```bash
git commit -m "feat!: 重构通信协议"
# 自动创建tag: v1.0.0 (假设当前最新tag是 v0.1.1)
```

### 4. 推送代码和标签

```bash
# 推送代码
git push origin main

# 推送所有标签
git push --tags

# 或推送特定标签
git push origin v1.0.0
```

## Git Tag 格式

- **格式**：`vMAJOR.MINOR.PATCH`（如 `v1.2.3`）
- **自动管理**：由 Git hooks 自动创建，无需手动创建
- **支持格式**：CMakeLists.txt 同时支持 `v1.0.0` 和 `1.0.0` 两种格式

## 版本号计算规则

1. **从最新 tag 读取基础版本号**
2. **根据 commit message 类型递增版本号**：
   - `BREAKING CHANGE` / `feat!:` → MAJOR + 1, MINOR = 0, PATCH = 0
   - `feat:` → MINOR + 1, PATCH = 0
   - `fix:` / `bugfix:` / `patch:` → PATCH + 1
   - 其他 → PATCH + 1
3. **创建新 tag**：指向当前 commit

## 编译时版本处理

### 版本号来源

1. **优先从 git tag 读取**：使用 `git describe --tags --abbrev=0` 获取最新 tag
2. **计算 commit 数量**：如果有新的 commit，会在 PATCH 版本号上增加 commit 数量
3. **回退机制**：如果没有 tag，使用 commit 数量生成版本号 `0.0.{commit_count}`

### 版本信息写入固件

编译时，CMakeLists.txt 会：
1. 读取 git tag
2. 提取 MAJOR、MINOR、PATCH 版本号
3. 通过编译宏定义传递给代码：
   - `FIRMWARE_VERSION_MAJOR`
   - `FIRMWARE_VERSION_MINOR`
   - `FIRMWARE_VERSION_PATCH`
   - `FIRMWARE_VERSION_STRING`
   - `FIRMWARE_BUILD_DATE`

## 手动操作

### 查看当前版本

```bash
# 查看最新tag
git describe --tags --abbrev=0

# 查看所有tag
git tag -l

# 查看tag详细信息
git show v1.0.0
```

### 手动创建tag（不推荐）

如果需要手动创建 tag（例如发布版本）：

```bash
# 创建带注释的tag
git tag -a v1.0.0 -m "Release version 1.0.0"

# 创建轻量级tag
git tag v1.0.0

# 推送tag
git push origin v1.0.0
```

### 删除tag

```bash
# 删除本地tag
git tag -d v1.0.0

# 删除远程tag
git push origin --delete v1.0.0
```

### 重置版本

如果需要重置版本号：

```bash
# 删除所有tag（谨慎操作）
git tag -d $(git tag -l)

# 创建初始tag
git tag v0.0.0

# 推送tag
git push --tags
```

## 编译时版本显示

编译完成后会自动显示版本信息：

```
==========================================
Firmware Build Complete
==========================================
Project Name: wht_slave
Firmware Version: 1.2.3
  Major Version: 1
  Minor Version: 2
  Patch Version: 3
Build Date: 20250115
Build Type: Debug
UWB Chip Type: CX310
==========================================
```

## Merge 时的版本处理

### Merge Commit 处理

- `prepare-commit-msg` hook 会检测 merge commit
- 对于 merge commit，**不会创建新的 tag**（避免重复更新）
- 版本由各个分支的 commit 自动管理，merge 时不需要特殊处理

### 版本冲突预防

1. **Tag 是全局的**：不会产生冲突
2. **Merge 时不创建 tag**：避免版本跳跃
3. **自动版本递增**：确保版本号递增

## 故障排除

### Tag 未创建

1. 检查 hooks 是否可执行（Windows 下 Git 会自动处理）
2. 检查 commit message 格式是否正确
3. 查看 `.git/hooks/commit-msg` 文件是否存在
4. 检查是否有 `.git/SKIP_VERSION_UPDATE` 文件（merge commit 会跳过）

### 推送时提示 tag 未推送

```bash
# 推送所有标签
git push --tags

# 或推送特定标签
git push origin v1.0.0
```

### 版本号不正确

1. 检查 git tag 是否存在：`git tag -l`
2. 检查 CMakeLists.txt 是否正确读取 tag
3. 查看 CMake 配置时的日志输出
4. 确认 tag 格式正确（`v1.0.0` 或 `1.0.0`）

### 没有 tag 时的处理

如果没有 tag，系统会：
1. 使用 commit 数量生成版本号：`0.0.{commit_count}`
2. 如果无法获取 commit 信息，使用默认版本：`0.0.0`

## 从 VERSION 文件迁移

如果之前使用 VERSION 文件管理版本，迁移步骤：

1. **创建初始 tag**：
   ```bash
   # 读取当前 VERSION 文件
   VERSION=$(cat VERSION)
   # 创建对应的 tag
   git tag v$VERSION
   git push --tags
   ```

2. **删除 VERSION 文件**（可选）：
   ```bash
   git rm VERSION
   git commit -m "chore: remove VERSION file, use git tags instead"
   ```

3. **验证**：编译项目，确认版本号正确

## 注意事项

1. **Tag 需要手动推送**：`git push` 不会自动推送 tag，需要使用 `git push --tags`
2. **每次 commit 都会创建 tag**：确保 commit message 格式正确
3. **推送前检查**：pre-push hook 会提示未推送的 tag
4. **Merge commit 不创建 tag**：避免版本重复更新
5. **Tag 格式**：推荐使用 `v` 前缀（如 `v1.0.0`），但 `1.0.0` 格式也支持
6. **版本号不会倒退**：系统确保版本号递增

## 优势

相比 VERSION 文件方式：

1. **版本信息在 Git 历史中**：每个 tag 都对应一个 commit，便于追溯
2. **无冲突问题**：Tag 是全局的，merge 时不会产生冲突
3. **更符合 Git 工作流**：使用 Git 原生功能管理版本
4. **便于发布管理**：可以直接基于 tag 创建 release
5. **版本追溯**：可以轻松查看每个版本的代码状态
