# cpp-option

[中文版](README_zh.md)

C++23 implementation of Rust's `Option` type for type-safe null value handling.

## Features

- Type-safe null handling
- C++23 features support (explicit `this` parameters, `std::expected`)
- Header file and module versions available
- Standard library integration

## Requirements

- C++23 compatible compiler

## Usage

Include the header file:
```cpp
#include "option.hpp"
```

Or import the module:
```cpp
import option;
```

Basic usage:
```cpp
auto value = opt::some(42);
auto empty = opt::none<int>;

if (value.is_some()) {
    int x = value.unwrap();
}

int result = empty.unwrap_or(0);
```

## Core Methods

### Creation
- `opt::some()` - create option with value
- `opt::none<T>` - create empty option
- `opt::some()` - create void option

### Checking
- `is_some()` - check if option has value
- `is_none()` - check if option is empty

### Value extraction
- `unwrap()` - extract value, panic if empty
- `unwrap_or()` - extract value or return default
- `unwrap_or_default()` - extract value or return default constructed
- `expect()` - extract value or panic with message

### Transformation
- `map()` - transform value if present
- `and_then()` - chain operations
- `filter()` - filter value by predicate
- `or_else()` - provide alternative if empty

### Other
- See [Rust's `Option` documentation](https://docs.rs/rustc-std-workspace-std/1.0.1/std/option/index.html) for more details.

## License

See LICENSE file.

---

# cpp-option 中文版

[English Version](#cpp-option) | [中文版](#cpp-option-中文版)

C++23 实现了 Rust 的 `Option` 类型，用于类型安全的空值处理。

## 功能

- 类型安全的空值处理
- 支持 C++23 特性（显式 `this` 参数，`std::expected`）
- 提供头文件和模块版本
- 标准库集成

## 要求

- C++23 兼容的编译器

## 使用方法

包含头文件：
```cpp
#include "option.hpp"
```

或者导入模块：
```cpp
import option;
```

基本使用：
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
- `opt::some()` - 创建带值的选项
- `opt::none<T>` - 创建空选项
- `opt::some()` - 创建 void 选项

### 检查
- `is_some()` - 检查选项是否有值
- `is_none()` - 检查选项是否为空

### 值提取
- `unwrap()` - 提取值，若为空则抛出异常
- `unwrap_or()` - 提取值或返回默认值
- `unwrap_or_default()` - 提取值或返回默认构造的值
- `expect()` - 提取值或抛出带消息的异常

### 转换
- `map()` - 如果有值则转换
- `and_then()` - 链式操作
- `filter()` - 根据谓词过滤值
- `or_else()` - 如果为空则提供替代值

### 其他
- 更多详情请参阅 [Rust 的 `Option` 文档](https://docs.rs/rustc-std-workspace-std/1.0.1/std/option/index.html)。

## 许可证

详见 LICENSE 文件。
