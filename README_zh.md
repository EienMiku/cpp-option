# cpp-option

[English](README.md)

Rust `Option` 类型的 C++23 实现，用于类型安全的空值处理。

## 概述

`option` 类型表示一个可选值：每个 `option` 要么包含一个值，要么为空。`option` 类型在 Rust 代码中非常常见，因为它们有许多用途：

- 初始值
- 不在整个输入范围内定义的函数返回值（部分函数）
- 用于报告简单错误的返回值，其中空的 `option` 表示错误
- 可选的结构体字段
- 可以被借用或"取走"的结构体字段
- 可选的函数参数
- 可空指针
- 处理困难情况时的值交换

`option` 通常与条件检查配合使用，用于查询值的存在性并采取行动，始终考虑空值的情况。

```cpp
#include "option.hpp"

opt::option<double> divide(double numerator, double denominator) {
    if (denominator == 0.0) {
        return opt::none<double>();
    } else {
        return opt::some(numerator / denominator);
    }
}

// 函数的返回值是一个 option
auto result = divide(2.0, 3.0);

// 检查值的存在性
if (result.is_some()) {
    // 除法有效
    std::println("结果：{}", result);
} else {
    // 除法无效
    std::println("不能除以 0");
}
```

## 选项和原始指针

C++ 的原始指针类型可以指向 null；但是，这会导致空指针解引用和未定义行为。`option` 可用于包装原始指针以提供额外的安全性，尽管现代 C++ 更倾向于使用智能指针来自动管理内存。

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

注意：对于现代 C++ 代码，考虑直接使用智能指针（`std::unique_ptr`、`std::shared_ptr`）而不是将它们包装在 `option` 中，因为智能指针已经提供了内存安全性和空值检查功能。

## 错误处理模式

与 Rust 类似，当编写调用许多返回 `option` 类型的函数的代码时，处理值和空状态可能会很繁琐。方法链提供了一种处理这种情况的简洁方法：

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

## 表示

这个 C++ 实现使用类似于 `std::optional` 的基于联合的存储，但具有受 Rust 的 `Option` 启发的额外功能。该实现支持：

- 值类型
- 引用类型（通过指针存储）
- `void` 类型（表示类似布尔值的"有值"或"空"状态）
- 移动语义和完美转发
- 尽可能的 `constexpr` 操作

## 方法概述

除了使用条件检查外，`option` 还提供了各种不同的方法。

### 查询变体

`is_some()` 和 `is_none()` 方法分别在 `option` 包含值或为空时返回 `true`。

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some());

constexpr auto y = opt::none<int>();
static_assert(y.is_none());
```

`is_some_and()` 和 `is_none_or()` 方法根据谓词测试包含的值：

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some_and([](int x) { return x > 1; }));

constexpr auto y = opt::none<int>();
static_assert(y.is_none_or([](int x) { return x == 2; }));
```

### 用于处理引用的适配器

- `as_ref()` 从 `option<T>` 转换为 `option<const T&>`
- `as_mut()` 从 `option<T>` 转换为 `option<T&>`
- `as_deref()` 从 `option<T>` 转换为 `option<const T::element_type&>`（对于智能指针）
- `as_deref_mut()` 从 `option<T>` 转换为 `option<T::element_type&>`（对于智能指针）

```cpp
constexpr auto x = opt::some(std::string("hello"));
static_assert(std::same_as<decltype(x.as_ref()), opt::option<const std::string &>>);
```

### 提取包含的值

这些方法在 `option<T>` 包含值时提取包含的值。如果 `option` 为空：

- `expect()` 抛出带有提供的自定义消息的异常
- `unwrap()` 抛出带有通用消息的异常
- `unwrap_or()` 返回提供的默认值
- `unwrap_or_default()` 返回类型 `T` 的默认值
- `unwrap_or_else()` 返回评估提供的函数的结果
- `unwrap_unchecked()` 如果在 `None` 上调用，会产生未定义行为

```cpp
constexpr auto x = opt::some(std::string("value"));
static_assert(x.expect("should have a value") == "value");
static_assert(x.unwrap() == "value");

constexpr auto y = opt::none<std::string_view>();
static_assert(y.unwrap_or("default"sv) == "default"sv);
static_assert(y.unwrap_or_default() == "");
static_assert(y.unwrap_or_else([]() { return "computed"sv; }) == "computed"sv);
```

### 转换包含的值

这些方法将 `option` 转换为 `std::expected`：

- `ok_or()` 将包含值的 `option` 转换为带有该值的 `std::expected`，将空的 `option` 转换为带有提供的错误值的 `std::unexpected`
- `ok_or_else()` 将包含值的 `option` 转换为带有该值的 `std::expected`，将空的 `option` 转换为带有提供的函数结果的 `std::unexpected`

```cpp
constexpr auto x = opt::some(std::string("foo"));
constexpr auto y = x.ok_or("error"sv);
static_assert(y.value() == "foo");

constexpr auto z = opt::none<std::string>();
constexpr auto w = z.ok_or("error"sv);
static_assert(w.error() == "error");
```

- `transpose()` 将 `std::expected` 的 `option` 转置为 `option` 的 `std::expected`

这些方法在有值时转换值：

- `filter()` 如果存在值，则对包含的值调用提供的谓词函数，如果函数返回 `true`，则返回包含该值的 `option`；否则返回空的 `option`
- `flatten()` 从 `option<option<T>>` 中删除一层嵌套
- `inspect()` 如果存在值，则使用包含的值调用提供的函数，并返回原始 `option`
- `map()` 通过将提供的函数应用于存在的包含值，将 `option<T>` 转换为 `option<U>`，并保持空的 `option` 不变

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

这些方法将 `option<T>` 转换为可能不同类型 `U` 的值：

- `map_or()` 将提供的函数应用于存在的包含值，如果为空则返回提供的默认值
- `map_or_else()` 将提供的函数应用于存在的包含值，如果为空则返回评估提供的回退函数的结果

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

这些方法组合两个 `option` 的值：

- `zip()` 如果两个 `option` 都包含值，则返回包含 `std::pair(s, o)` 的 `option`；否则返回空的 `option`
- `zip_with()` 如果两个 `option` 都包含值，则调用提供的函数并返回包含 `f(s, o)` 的 `option`；否则返回空的 `option`

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

### 布尔运算符

这些方法将 `option` 视为布尔值，其中包含值的 `option` 相当于 `true`，空的 `option` 相当于 `false`。这些方法有两类：接受 `option` 作为输入的方法，以及接受函数作为输入的方法（要懒惰求值）。

`and_()`、`or_()` 和 `xor_()` 方法接受另一个 `option` 作为输入，并产生一个 `option` 作为输出：

| 方法 | self | other | 输出 |
|------|------|-------|------|
| `and_()` | 空 | (忽略) | 空 |
| `and_()` | 有值(x) | 空 | 空 |
| `and_()` | 有值(x) | 有值(y) | 有值(y) |
| `or_()` | 空 | 空 | 空 |
| `or_()` | 空 | 有值(y) | 有值(y) |
| `or_()` | 有值(x) | (忽略) | 有值(x) |
| `xor_()` | 空 | 空 | 空 |
| `xor_()` | 空 | 有值(y) | 有值(y) |
| `xor_()` | 有值(x) | 空 | 有值(x) |
| `xor_()` | 有值(x) | 有值(y) | 空 |

```cpp
constexpr auto x = opt::some(2);
constexpr auto y = opt::none<int>();

static_assert(x.and_(y).is_none());
static_assert(x.or_(y).is_some());
static_assert(x.xor_(y).is_some());
```

`and_then()` 和 `or_else()` 方法接受一个函数作为输入，并且只有在需要产生新值时才评估该函数：

| 方法 | self | 函数输入 | 函数输出 | 最终输出 |
|------|------|----------|----------|----------|
| `and_then()` | 空 | (未提供) | (未评估) | 空 |
| `and_then()` | 有值(x) | `x` | 空 | 空 |
| `and_then()` | 有值(x) | `x` | 有值(y) | 有值(y) |
| `or_else()` | 空 | (未提供) | 空 | 空 |
| `or_else()` | 空 | (未提供) | 有值(y) | 有值(y) |
| `or_else()` | 有值(x) | (未提供) | (未评估) | 有值(x) |

```cpp
constexpr auto x = opt::some(2);
constexpr auto y = x.and_then([](int x) { return opt::some(x * 2); });
static_assert(y == opt::some(4));

constexpr auto z = opt::none<int>();
constexpr auto w = z.or_else([]() { return opt::some(42); });
static_assert(w == opt::some(42));
```

### 比较运算符

如果 `T` 实现了比较运算符，那么 `option<T>` 将派生其比较实现。在这种顺序下，空的 `option` 比任何包含值的 `option` 都小，两个包含值的 `option` 的比较方式与它们在 `T` 中包含的值相同。

```cpp
static_assert(opt::none<int>() < opt::some(0));
static_assert(opt::some(0) < opt::some(1));
```

### 就地修改 `option`

这些方法返回对 `option<T>` 包含值的可变引用：

- `insert()` 插入一个值，丢弃任何旧内容
- `get_or_insert()` 获取当前值，如果为空则插入提供的默认值
- `get_or_insert_default()` 获取当前值，如果为空则插入默认值
- `get_or_insert_with()` 获取当前值，如果为空则插入由提供的函数计算的默认值

```cpp
constexpr auto foo() {
    auto x  = opt::none<int>();
    auto &y = x.get_or_insert(5);
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::some(5), 5 });
```

这些方法转移 `option` 包含值的所有权：

- `take()` 如果存在，则获取包含值的所有权，将 `option` 替换为空状态
- `replace()` 如果存在，则获取包含值的所有权，将 `option` 替换为包含提供值的状态

```cpp
constexpr auto foo() {
    auto x = opt::some(2);
    auto y = x.take();
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::none<int>(), opt::some(2) });
```

## 示例

### 在循环前将结果初始化为 `None`

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

### 使用方法调用链式操作

```cpp
#include "option.hpp"
#include <vector>
#include <algorithm>

std::vector<opt::option<int>> options = {
    opt::some(1), 
    opt::none<int>(), 
    opt::some(3)
};

// 收集所有 some 值
std::vector<int> values;
for (const auto& opt : options) {
    if (opt.is_some()) {
        values.push_back(opt.unwrap());
    }
}
// [1, 3]

// 在管道中转换和过滤
constexpr auto process = [](int x) -> opt::option<int> {
    return opt::some(x)
        .filter([](int y) { return y > 0; })
        .map([](int y) { return y * 2; });
};

static_assert(process(5) == opt::some(10));
```

## 要求

- 支持 C++23 的编译器
- 支持显式 `this` 参数（`__cpp_explicit_this_parameter >= 202110L`）
- 支持 `std::expected`（`__cpp_lib_expected >= 202202L`）

## 用法

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

// 映射操作
auto doubled = value.map([](int x) { return x * 2; });
std::println("加倍: {}", doubled);

// Or_else 操作
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
- `opt::some(value)` - 创建包含值的 option
- `opt::none<T>()` - 创建空的 option

### 典型用例
```cpp
// 可能无法找到值的函数
opt::option<std::string> find_user_name(int user_id) {
    if (user_id == 42) {
        return opt::some(std::string("Alice"));
    }
    return opt::none<std::string>();
}

// 可能失败的解析
opt::option<int> parse_int(const std::string& str) {
    try {
        return opt::some(std::stoi(str));
    } catch (...) {
        return opt::none<int>();
    }
}

// 安全的数组访问
template<typename T>
opt::option<T> safe_get(const std::vector<T>& vec, size_t index) {
    if (index < vec.size()) {
        return opt::some(vec[index]);
    }
    return opt::none<T>();
}
```

### 查询
- `is_some()` - 检查 option 是否包含值
- `is_none()` - 检查 option 是否为空
- `is_some_and(predicate)` - 检查 option 是否包含满足谓词的值
- `is_none_or(predicate)` - 检查 option 是否为空或值满足谓词

### 值提取
- `unwrap()` - 提取值，若为空则抛出异常
- `unwrap_or(default)` - 提取值或返回默认值
- `unwrap_or_default()` - 提取值或返回默认构造的值
- `unwrap_or_else(func)` - 提取值或返回调用 func 的结果
- `expect(message)` - 提取值或抛出带有自定义消息的异常
- `unwrap_unchecked()` - 提取值而不检查（如果为空则为未定义行为）

### 转换
- `map(func)` - 转换包含的值
- `map_or(default, func)` - 转换包含的值或返回默认值
- `map_or_else(default_func, func)` - 转换包含的值或调用 default_func
- `and_then(func)` - 链接返回 option 的操作
- `filter(predicate)` - 根据谓词过滤值
- `flatten()` - 展平嵌套的 option

### 布尔操作
- `and_(other)` - 如果 this 是 Some 则返回 other，否则返回 None
- `or_(other)` - 如果 this 是 Some 则返回 this，否则返回 other
- `xor_(other)` - 如果 this 和 other 中恰好一个是 Some 则返回 Some
- `or_else(func)` - 如果 this 是 Some 则返回 this，否则调用 func

### 转换
- `ok_or(error)` - 转换为带有错误值的 std::expected
- `ok_or_else(error_func)` - 转换为带有 func 错误的 std::expected
- `transpose()` - 将 `option<expected<T, E>>` 转置为 `expected<option<T>, E>`

### 引用适配器
- `as_ref()` - 转换为 const 引用的 option
- `as_mut()` - 转换为可变引用的 option
- `as_deref()` - 解引用包含的值
- `as_deref_mut()` - 可变解引用包含的值

### 就地修改
- `insert(value)` - 插入值，返回对它的引用
- `get_or_insert(value)` - 获取当前值或插入默认值
- `get_or_insert_default()` - 获取当前值或插入 T{}
- `get_or_insert_with(func)` - 获取当前值或插入 func 的结果
- `take()` - 取出包含的值，留下 None
- `replace(value)` - 替换包含的值

### 组合 Option
- `zip(other)` - 将两个 option 组合成 pair 的 option
- `zip_with(other, func)` - 使用函数组合两个 option
- `unzip()` - 将 pair 的 option 拆分为 option 的 pair

### 检查
- `inspect(func)` - 如果是 Some，则使用包含的值调用 func

### 比较
- `cmp(other)` - 三路比较
- `eq(other)`, `ne(other)`, `lt(other)`, `le(other)`, `gt(other)`, `ge(other)` - 比较操作
- `max(other)`, `min(other)` - 获取两个 option 的最大值/最小值
- `clamp(min, max)` - 将 option 限制在 min 和 max 之间

### 复制和克隆
- `copied()` - 创建 option 的副本
- `cloned()` - 创建 option 的克隆（如果 T 有 clone() 方法）

## 许可证

详见 LICENSE 文件。
