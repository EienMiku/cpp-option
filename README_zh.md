# cpp-option

[English](README.md)

`cpp-option` 是一个基于 C++23 的泛型库，实现了类似 Rust `Option` 类型的类型安全可选值容器。该库旨在为 C++ 提供更安全、简洁的可空值处理方式，支持丰富的操作方法和现代 C++ 特性。

## 概述

`opt::option<T>` 表示一个可选值：要么包含类型为 `T` 的值（有值），要么为空（无值）。该类型用于以下场景：

- 变量的初始值
- 部分函数的返回值（如查找、解析等可能失败的操作）
- 简单错误报告（无值即为错误）
- 可选结构体字段
- 可选函数参数
- 可空指针的安全封装
- 复杂场景下的值交换与所有权转移

`option` 类型配合条件判断，可显式处理值存在与否，避免空指针解引用、未初始化等常见错误。


```cpp
#include "option.hpp"

opt::option<double> divide(double numerator, double denominator) {
    if (denominator == 0.0) {
        return opt::none<double>();
    }
    return opt::some(numerator / denominator);
}

auto result = divide(2.0, 3.0);
if (result.is_some()) {
    std::println("结果: {}", result.unwrap());
} else {
    std::println("不能除以 0");
}
```

## `option` 与原始指针

C++ 原始指针可为 null，易导致空指针解引用等未定义行为。`option` 可安全封装原始指针，但推荐优先使用智能指针（如 `std::unique_ptr`、`std::shared_ptr`）。

```cpp
#include "option.hpp"

void check_optional(const opt::option<int*>& optional) {
    if (optional.is_some()) {
        std::println("有值 {}", *optional.unwrap());
    } else {
        std::println("没有值");
    }
}

int main() {
    auto optional = opt::none<int*>();
    check_optional(optional);

    int value = 9000;
    auto optional2 = opt::some(&value);
    check_optional(optional2);
}
```


## 错误处理模式与链式调用

当多个操作均可能返回 `option` 时，可通过链式方法调用（method chaining）简化嵌套判断：

```cpp
#include "option.hpp"
#include <unordered_map>
#include <vector>

std::unordered_map<int, std::string> bt = {
    { 20, "foo" },
    { 42, "bar" }
};

auto checked_sub = [](int x, int y) -> opt::option<int> {
    if (x < y)
        return opt::none<int>();
    return opt::some(x - y);
};

auto checked_mul = [](int x, int y) -> opt::option<int> {
    if (x > INT_MAX / y)
        return opt::none<int>();
    return opt::some(x * y);
};

auto lookup = [](int x) -> opt::option<std::string> {
    auto it = bt.find(x);
    if (it != bt.end())
        return opt::some(it->second);
    return opt::none<std::string>();
};

std::vector<int> values = { 0, 1, 11, 200, 22 };
std::vector<std::string> results;

for (int x : values) {
    auto result = checked_sub(x, 1)
        .and_then([](int x) { return checked_mul(x, 2); })
        .and_then([](int x) { return lookup(x); })
        .or_else([]() { return opt::some(std::string("error!")); });

    results.push_back(result.unwrap());
}
// results: ["error!", "error!", "foo", "error!", "bar"]
```


## 实现说明

本库采用联合体（`union`）存储机制，支持值类型、引用类型（以指针实现）、`void` 类型（仅表示有无值），并结合 Rust `Option` 语义与 C++23 特性：

- 完善的移动/拷贝语义与完美转发
- 丰富的 `constexpr` 支持
- 显式 `this` 参数（deducing this）
- 与 `std::expected`、`std::pair` 等标准库类型的集成


## 方法总览

`option` 除了基本的条件判断，还提供了丰富的成员方法，涵盖状态查询、值提取、变换、组合、就地修改、类型转换等。


### 状态查询

`is_some()` / `is_none()`：判断是否有值。

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some());

constexpr auto y = opt::none<int>();
static_assert(y.is_none());
```

`is_some_and()` 和 `is_none_or()` 可结合谓词判断：

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some_and([](int x) { return x > 1; }));

constexpr auto y = opt::none<int>();
static_assert(y.is_none_or([](int x) { return x == 2; }));
```


### 引用适配器

- `as_ref()`：转为 `option<const T&>`
- `as_mut()`：转为 `option<T&>`
- `as_deref()`：解引用后转为 `option<const T::element_type&>`（适用于指针/智能指针等）
- `as_deref_mut()`：解引用后转为 `option<T::element_type&>`

```cpp
constexpr auto x = opt::some(std::string("hello"));
static_assert(std::same_as<decltype(x.as_ref()), opt::option<const std::string &>>);
```


### 提取值

- `unwrap()`：提取值，若为空则抛出异常
- `expect(msg)`：为空时抛出带自定义消息的异常
- `unwrap_or(default)`：为空时返回指定默认值
- `unwrap_or_default()`：为空时返回类型 `T` 的默认值
- `unwrap_or_else(func)`：为空时返回函数结果
- `unwrap_unchecked()`：无检查提取（为空时未定义行为）

```cpp
constexpr auto x = opt::some(std::string("value"));
static_assert(x.expect("should have a value") == "value");
static_assert(x.unwrap() == "value");

constexpr auto y = opt::none<std::string_view>();
static_assert(y.unwrap_or("default"sv) == "default"sv);
static_assert(y.unwrap_or_default() == "");
static_assert(y.unwrap_or_else([]() { return "computed"sv; }) == "computed"sv);
```


### 转换为 `std::expected`

- `ok_or(error)`：有值时转为 `std::expected<T, E>`，为空时转为带错误的 `std::expected<T, E>`
- `ok_or_else(func)`：有值时转为 `std::expected<T, E>`，为空时转为 `func` 生成的 `std::expected<T, E>`
- `transpose()`：`option<std::expected<T, E>>` 转为 `std::expected<option<T>, E>`

```cpp
constexpr auto x = opt::some(std::string("foo"));
constexpr auto y = x.ok_or("error"sv);
static_assert(y.value() == "foo");

constexpr auto z = opt::none<std::string>();
constexpr auto w = z.ok_or("error"sv);
static_assert(w.error() == "error");
```

- `transpose()`：将 `option<std::expected<T, E>>` 转为 `std::expected<option<T>, E>`


### 变换与映射

- `map(func)`：有值时应用函数，返回新 `option`
- `map_or(default, func)`：有值时应用函数，否则返回默认值
- `map_or_else(default_func, func)`：有值时应用函数，否则返回回退函数结果
- `filter(predicate)`：有值时按谓词过滤
- `flatten()`：展开一层嵌套的 `option<option<T>>`
- `inspect(func)`：有值时执行副作用函数，返回自身

```cpp
constexpr auto x = opt::some(4);
constexpr auto y = x.filter([](int x) {
    return x > 2;
});
static_assert(y == opt::some(4));

constexpr auto z = opt::some(1);
constexpr auto w = z.filter([](int x) {
    return x > 2;
});
static_assert(w == opt::none<int>());

constexpr auto a = opt::some(2);
constexpr auto b = a.map([](int x) {
    return x * 2;
});
static_assert(b == opt::some(4));
```

这些方法将 `option<T>` 转为其他类型 `U`：

- `map_or()`：有值时应用函数，否则返回默认值
- `map_or_else()`：有值时应用函数，否则返回回退函数结果

```cpp
constexpr auto x = opt::some(std::string("foo"));
constexpr auto y = x.map_or(42, [](const std::string &s) {
    return static_cast<int>(s.length());
});
static_assert(y == 3);

constexpr auto z = opt::none<std::string>();
constexpr auto w = z.map_or(42, [](const std::string &s) {
    return static_cast<int>(s.length());
});
static_assert(w == 42);
```


### 组合与解包

- `zip(other)`：两个都有值时返回 `option<std::pair<T, U>>`
- `zip_with(other, func)`：两个都有值时用函数合并
- `unzip()`：`option<std::pair<T, U>>` 拆分为 `pair<option<T>, option<U>>`

```cpp
constexpr auto x = opt::some("foo"sv);
constexpr auto y = opt::some("bar"sv);

constexpr auto z = x.zip_with(y, [](auto a, auto b) {
    return a[0] == b[0];
});
static_assert(z == opt::some(false));


constexpr auto w = x.zip(y);
static_assert(w == opt::some(std::pair("foo"sv, "bar"sv)));
```


### 布尔逻辑操作

- `and_(other)`：有值时返回 `other`，否则返回空 `option`
- `or_(other)`：有值时返回自身，否则返回 `other`
- `xor_(other)`：仅有一个为有值时返回有值的 `option`，否则返回空 `option`
- `and_then(func)`：有值时调用函数，返回新 `option`
- `or_else(func)`：为空时调用函数，返回新 `option`


### 比较与排序

若 `T` 支持比较，`option<T>` 也支持。空的 `option` 总小于有值的 `option`，有值时按值比较。

```cpp
static_assert(opt::none<int>() < opt::some(0));
static_assert(opt::some(0) < opt::some(1));
```


### 就地修改

- `insert(value)`：插入新值，丢弃旧值
- `get_or_insert(value)`：获取当前值，若为空则插入默认值
- `get_or_insert_default()`：为空则插入类型默认值
- `get_or_insert_with(func)`：为空则插入函数生成的值

```cpp
constexpr auto foo() {
    auto x  = opt::none<int>();
    auto &y = x.get_or_insert(5);
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::some(5), 5 });
```


### 所有权转移

- `take()`：取出值并置空
- `replace(value)`：替换为新值，返回旧值

```cpp
constexpr auto foo() {
    auto x = opt::some(2);
    auto y = x.take();
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::none<int>(), opt::some(2) });
```


## 示例

### 在循环前初始化结果为空 `option`

```cpp
#include "option.hpp"

enum class Kingdom { Plant, Animal };

struct BigThing {
    Kingdom kind;
    int size;
    std::string_view name;
};

constexpr std::array<BigThing, 6> all_the_big_things = {{
    { Kingdom::Plant,  250, std::string_view("redwood") },
    { Kingdom::Plant,  230, std::string_view("noble fir") },
    { Kingdom::Plant,  229, std::string_view("sugar pine") },
    { Kingdom::Animal, 25,  std::string_view("blue whale") },
    { Kingdom::Animal, 19,  std::string_view("fin whale") },
    { Kingdom::Animal, 15,  std::string_view("north pacific right whale") },
}};

constexpr opt::option<std::string_view> find_biggest_animal_name() {
    int max_size = 0;
    opt::option<std::string_view> max_name = opt::none<std::string_view>();
    for (const auto& thing : all_the_big_things) {
        if (thing.kind == Kingdom::Animal && thing.size > max_size) {
            max_size = thing.size;
            max_name = opt::some(thing.name);
        }
    }
    return max_name;
}

constexpr auto name_of_biggest_animal = find_biggest_animal_name();
static_assert(name_of_biggest_animal.is_some(), "there are no animals :(");
static_assert(name_of_biggest_animal.unwrap() == std::string_view("blue whale"), "the biggest animal should be blue whale");
```


### 链式方法调用与变换

```cpp
#include "option.hpp"
#include <vector>
#include <algorithm>

std::vector<opt::option<int>> options = {
    opt::some(1), 
    opt::none<int>(), 
    opt::some(3)
};

// 收集所有有值的项
std::vector<int> values;
for (const auto& opt : options) {
    if (opt.is_some()) {
        values.push_back(opt.unwrap());
    }
}
// [1, 3]

// 管道式变换与过滤
constexpr auto process = [](int x) -> opt::option<int> {
    return opt::some(x)
        .filter([](int y) { return y > 0; })
        .map([](int y) { return y * 2; });
};

static_assert(process(5) == opt::some(10));
```


## 编译要求

- C++23 标准支持
- 显式 `this` 参数（`__cpp_explicit_this_parameter >= 202110L`）
- `std::expected`（`__cpp_lib_expected >= 202202L`）


## 用法示例

### 基本用法
```cpp
#include "option.hpp"

opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none<int>();

if (value.is_some()) {
    int x = value.unwrap();
    std::println("值: {}", x);
}

int result = empty.unwrap_or(0);
std::println("结果: {}", result);
```

### 高级用法
```cpp
#include "option.hpp"

opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none<int>();

// map 操作
auto doubled = value.map([](int x) { return x * 2; });
std::println("加倍: {}", doubled);

// or_else 操作
auto fallback = empty.or_else([]() { return opt::some(99); });
std::println("后备: {}", fallback);

// 链式操作
opt::option<int> combined = value.and_then([](int x) { 
    return opt::some(x + 1); 
});
std::println("组合: {}", combined);
```

## 核心方法

### 创建
- `opt::some(value)` —— 创建包含值的 option
- `opt::none<T>()` —— 创建空 option

### 典型用例
```cpp
// 查找函数
opt::option<std::string> find_user_name(int user_id) {
    if (user_id == 42) {
        return opt::some(std::string("Alice"));
    }
    return opt::none<std::string>();
}

// 解析函数
opt::option<int> parse_int(const std::string& str) {
    try {
        return opt::some(std::stoi(str));
    } catch (...) {
        return opt::none<int>();
    }
}

// 安全数组访问
template<typename T>
opt::option<T> safe_get(const std::vector<T>& vec, size_t index) {
    if (index < vec.size()) {
        return opt::some(vec[index]);
    }
    return opt::none<T>();
}
```

## 许可证

详见 LICENSE 文件。
