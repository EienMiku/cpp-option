# cpp-option

[English](README.md)

Rust `Option` 类型的 C++23 的实现，用于类型安全的空值处理。

## 特性

- 类型安全的空值处理
- 支持 C++23 特性（显式 `this` 参数、`std::expected`）
- 提供头文件和模块版本
- 与标准库集成

## 要求

- 支持 C++23 的编译器

## 用法

包含头文件：
```cpp
#include "option.hpp"
```

或导入模块：
```cpp
import option;
```

基本用法：
```cpp
auto value = opt::some(42);
auto empty = opt::none<int>;

if (value.is_some()) {
	int x = value.unwrap();
}

int result = empty.unwrap_or(0);
```

## 核心方法

### 创建
- `opt::some()` - 创建包含值的 option
- `opt::none<T>` - 创建空的 option
- `opt::some()` - 创建 void 类型的 option

### 检查
- `is_some()` - 检查 option 是否有值
- `is_none()` - 检查 option 是否为空

### 值提取
- `unwrap()` - 提取值，若为空则抛出异常
- `unwrap_or()` - 提取值或返回默认值
- `unwrap_or_default()` - 提取值或返回默认构造的值
- `expect()` - 提取值或抛出带消息的异常

### 转换
- `map()` - 若有值则进行转换
- `and_then()` - 链式操作
- `filter()` - 根据谓词过滤值
- `or_else()` - 若为空则提供备选值

### 其他
- 更多详情请参考 [Rust `Option` 文档](https://docs.rs/rustc-std-workspace-std/1.0.1/std/option/index.html)。

## 许可证

详见 LICENSE 文件。
