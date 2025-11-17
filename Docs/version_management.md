# 版本管理说明

## 概述

本项目使用Git hooks自动根据commit message更新版本号。版本信息存储在 `VERSION` 文件中，并在编译时自动读取。

## 工作流程

1. **提交代码时**：`commit-msg` hook 分析commit message，根据类型自动更新 `VERSION` 文件
2. **提交后**：`post-commit` hook 自动将 `VERSION` 文件添加到staging area
3. **推送前**：`pre-push` hook 检查 `VERSION` 文件是否已提交
4. **编译时**：CMakeLists.txt 优先从 `VERSION` 文件读取版本号

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
# VERSION: 0.0.0 → 0.1.0
```

### 2. 修复提交

```bash
git commit -m "fix: 修复内存泄漏问题"
# VERSION: 0.1.0 → 0.1.1
```

### 3. 破坏性变更

```bash
git commit -m "feat!: 重构通信协议"
# VERSION: 0.1.1 → 1.0.0
```

### 4. 推送代码

```bash
# 正常推送，VERSION文件会自动包含
git push origin main
```

## 版本文件

- **位置**：项目根目录的 `VERSION` 文件
- **格式**：`MAJOR.MINOR.PATCH`（如 `1.2.3`）
- **自动管理**：由Git hooks自动更新，无需手动编辑

## 手动操作

### 查看当前版本

```bash
cat VERSION
```

### 手动设置版本（不推荐）

如果需要手动设置版本：

```bash
echo "1.0.0" > VERSION
git add VERSION
git commit -m "chore: 设置版本号为1.0.0"
```

### 重置版本

```bash
echo "0.0.0" > VERSION
git add VERSION
git commit -m "chore: 重置版本号"
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

## 故障排除

### VERSION文件未更新

1. 检查hooks是否可执行（Windows下Git会自动处理）
2. 检查commit message格式是否正确
3. 查看 `.git/hooks/commit-msg` 文件是否存在

### 推送时提示VERSION未提交

```bash
# 手动添加并提交VERSION文件
git add VERSION
git commit -m "chore: update version"
git push
```

### 版本号不正确

1. 检查 `VERSION` 文件内容
2. 检查CMakeLists.txt是否正确读取文件
3. 查看CMake配置时的日志输出

## Merge 时的版本处理

### 云端 Merge 场景

当两个分支合并时，如果两个分支都修改了VERSION文件，会发生以下情况：

#### 1. 自动冲突解决（推荐）

如果发生merge冲突，`post-merge` hook会自动：
- 检测VERSION文件的冲突标记
- 比较两个版本的版本号
- **自动选择较大的版本号**（确保版本号不会倒退）
- 自动解决冲突并添加到staging area

**示例**：
```
分支A: VERSION = 0.2.0
分支B: VERSION = 0.1.5
Merge后: VERSION = 0.2.0 (自动选择较大的版本)
```

#### 2. Merge Commit 处理

- `prepare-commit-msg` hook会检测merge commit
- 对于merge commit，不会触发版本更新（避免重复更新）
- 版本冲突由`post-merge` hook自动解决

#### 3. 手动处理（如果自动解决失败）

如果自动解决失败，需要手动处理：

```bash
# 1. 查看冲突
git status

# 2. 手动编辑VERSION文件，选择较大的版本号
# 例如：0.2.0 和 0.1.5，选择 0.2.0

# 3. 解决冲突后
git add VERSION
git commit -m "chore: resolve VERSION merge conflict"
```

### 版本混乱预防

1. **自动选择较大版本**：确保版本号不会倒退
2. **Merge时不重复更新**：避免merge commit导致版本跳跃
3. **冲突自动解决**：减少手动干预

## 注意事项

1. **VERSION文件会被提交到仓库**：这是正常的，确保团队版本同步
2. **每次commit都会更新版本**：确保commit message格式正确
3. **推送前检查**：pre-push hook会确保VERSION文件已提交
4. **回退机制**：如果VERSION文件无效，CMake会回退到Git标签方式
5. **Merge冲突自动解决**：post-merge hook会自动选择较大的版本号
6. **版本号不会倒退**：merge时总是选择较大的版本，确保版本递增

