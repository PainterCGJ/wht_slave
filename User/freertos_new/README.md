# FreeRTOS New/Delete Override

## 概述

此模块重载了 C++ 的全局 `new` 和 `delete` 操作符，使其使用 FreeRTOS 的内存管理函数（`pvPortMalloc` 和 `vPortFree`）而不是标准 C++ 库的内存管理。

## 功能特性

- ✅ 重载 `operator new` 和 `operator new[]`
- ✅ 重载 `operator delete` 和 `operator delete[]`
- ✅ 支持 `std::nothrow` 版本
- ✅ 支持 C++17 的带大小参数的 `delete` 操作符
- ✅ 嵌入式系统友好：不抛出异常，分配失败时返回 `nullptr`

## 使用方法

**无需额外操作！** 只要此模块被编译和链接到项目中，所有 C++ 代码中的 `new` 和 `delete` 操作都会自动使用 FreeRTOS 的内存管理。

### 示例

```cpp
// 在代码中正常使用 new/delete，会自动使用 FreeRTOS 内存管理
// 注意：分配失败时返回 nullptr（不抛出异常，适合嵌入式系统）
MyClass* obj = new MyClass();
if (obj == nullptr) {
    // 处理分配失败
    // 注意：在嵌入式系统中，应该始终检查 new 的返回值
}
delete obj;

// 数组分配
int* arr = new int[100];
if (arr == nullptr) {
    // 处理分配失败
}
delete[] arr;

// nothrow 版本（行为与标准版本相同，都返回 nullptr）
MyClass* obj2 = new(std::nothrow) MyClass();
if (obj2 == nullptr) {
    // 处理分配失败
}
```

## 工作原理

1. 全局操作符重载：重载了所有全局的 `new`/`delete` 操作符
2. FreeRTOS 集成：使用 `pvPortMalloc()` 进行内存分配，使用 `vPortFree()` 进行内存释放
3. 嵌入式友好：所有 `new` 操作符在分配失败时返回 `nullptr`，不抛出异常（适合禁用异常的单片机环境）

## 注意事项

- 此模块会替换**所有** C++ 代码中的内存分配，确保所有动态内存都通过 FreeRTOS 堆管理
- 确保 FreeRTOS 配置了足够的堆空间（`configTOTAL_HEAP_SIZE`）
- **重要**：在嵌入式系统中，应该始终检查 `new` 的返回值是否为 `nullptr`
- 此实现不抛出异常，适合使用 `-fno-exceptions` 编译选项的嵌入式项目
- 如果需要在某些特定场景使用标准库的内存管理，需要使用 `std::malloc` 和 `std::free`

## 技术细节

- 头文件：`freertos_new.h`（可选包含，用于文档目的）
- 实现文件：`freertos_new.cpp`（必须编译和链接）
- 依赖：FreeRTOS（需要 `FreeRTOS.h` 头文件）

