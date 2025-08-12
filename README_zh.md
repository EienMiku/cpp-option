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

constexpr opt::option<double> divide(double numerator, double denominator) {
    if (denominator == 0.0) {
        return opt::none;
    }
    return opt::some(numerator / denominator);
}

constexpr auto result_1 = divide(2.0, 3.0);
static_assert(result_1 == opt::some(0.6666666666666666));

constexpr auto result_2 = divide(2.0, 0.0);
static_assert(result_2 == opt::none);
```

## 与标准库兼容性

`opt::option` **完全兼容** C++26 草案的 `std::optional`，但某些成员函数会将返回的 `std::optional` 改为 `opt::option`。你可以将 `std::optional` 完全替换为 `opt::option`，并享受更丰富的功能和也许更好的性能。


## `option` 与原始指针

C++ 原始指针可为空，易导致空指针解引用等未定义行为。`option` 可安全封装原始指针，但推荐优先使用智能指针（如 `std::unique_ptr`、`std::shared_ptr`）。

```cpp
#include "option.hpp"

constexpr auto whatever = 1;
static_assert(opt::some(&whatever).unwrap() == &whatever);

static_assert(opt::some(&whatever).is_some());

static_assert(opt::some(nullptr).unwrap() == nullptr);

void check_optional(const opt::option<int*>& optional) {
    if (optional.is_some()) {
        std::println("有值 {}", *optional.unwrap());
    } else {
        std::println("没有值");
    }
}

int main() {
    auto optional = opt::none;
    check_optional(optional);

    int value = 9000;
    auto optional2 = opt::some(&value);
    check_optional(optional2);
}
```

## 错误处理模式与链式调用

当多个操作均可能返回 `option` 时，可通过链式方法调用简化嵌套判断：

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
        return opt::none;
    return opt::some(x - y);
};

auto checked_mul = [](int x, int y) -> opt::option<int> {
    if (x > INT_MAX / y)
        return opt::none;
    return opt::some(x * y);
};

auto lookup = [](int x) -> opt::option<std::string> {
    auto it = bt.find(x);
    if (it != bt.end())
        return opt::some(it->second);
    return opt::none;
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

本库采用 `union` 存储机制，支持值类型、引用类型（以指针实现）、`void` 类型（仅表示有无值），并结合 Rust `Option` 语义与 C++26 特性：

- 完善的移动/拷贝语义与完美转发
- 丰富的 `constexpr` 支持
- 显式 `this` 参数（deducing this）
- 与 `std::expected`、`std::pair` 等标准库类型的集成

## 方法总览

`option` 除了基本的条件判断，还提供了丰富的成员方法，涵盖状态查询、值提取、变换、组合、就地修改、类型转换等。


### 状态查询

`option` 提供多种状态查询方法，便于判断和分支处理：
- `is_some()`：判断是否有值（返回 `true`/`false`）。
- `is_none()`：判断是否为空（返回 `true`/`false`）。
- `is_some_and(pred)`：有值且满足谓词 pred 时返回 `true`，否则返回 `false`。
- `is_none_or(pred)`：为空或值满足谓词 pred 时返回 `true`，否则返回 `false`。

```cpp
// is_some
constexpr auto x = opt::some(2);
static_assert(x.is_some());

// is_none
constexpr opt::option<int> y = opt::none;
// constexpr auto y = opt::none_opt<int>;
static_assert(y.is_none());

// is_some_and
constexpr auto z = opt::some(2);
static_assert(z.is_some_and([](int x) { return x > 1; }));

// is_none_or
constexpr auto w = opt::option<int>(opt::none);
static_assert(w.is_none_or([](int x) { return x == 2; }));
```


### 引用适配器

`option` 支持多种引用适配器，便于安全地获取引用或解引用后的可选值：
- `as_ref()`：转为 `option<const T&>`。
- `as_mut()`：转为 `option<T&>`。
- `as_deref()`：若有值则对其解引用，转为 `option<const U&>`，`U` 为 `T` 解引用后的类型。适用于指针 / 智能指针等。
- `as_deref_mut()`：若有值则对其解引用，转为 `option<U&>`，`U` 为 `T` 解引用后的类型。适用于指针 / 智能指针等。

```cpp
// as_ref
constexpr auto as_ref_test() {
    auto s = opt::some("abc"s);

    return s.as_ref().map([](const auto &v) {
        return v.size();
    });
}

static_assert(as_ref_test() == opt::some(3uz));

// as_mut
constexpr auto as_mut_test() {
    auto v = opt::some(1);

    *v.as_mut() += 1;

    return v;
}

static_assert(as_mut_test() == opt::some(2));

// as_deref
constexpr auto as_deref_test() {
    auto v = 42;

    auto opt = opt::some(std::addressof(v));

    return opt.as_deref().unwrap();
}

static_assert(as_deref_test() == opt::some(42));

// as_deref_mut
constexpr auto as_deref_mut_test() {
    auto v = 1;

    auto opt = opt::some(std::addressof(v));

    *opt.as_deref_mut() += 1;

    return *opt.unwrap();
}

static_assert(as_deref_mut_test() == opt::some(2));
```


### 提取值

`option` 提供多种安全或灵活的值提取方式，适应不同的错误处理需求：
- `unwrap()`：提取 `option` 中的值，若为空则抛出异常（`option_panic`）。
- `expect(msg)`：与 `unwrap()` 类似，但可自定义异常消息。
- `unwrap_or(default)`：若有值则返回，否则返回指定默认值。
- `unwrap_or_default()`：若有值则返回，否则返回类型 `T` 的默认值（需 `T` 可默认构造）。
- `unwrap_or_else(func)`：若有值则返回，否则调用函数生成返回值。
- `unwrap_unchecked()`：无检查提取（为空时为未定义行为，仅建议在已知有值时使用）。

示例：
```cpp
// unwrap
constexpr auto x = opt::some(std::string("value"));
static_assert(x.unwrap() == "value");

// expect
static_assert(x.expect("必须有值") == "value");

// unwrap_or
constexpr opt::option<std::string> y = opt::none;
constexpr auto default_string = std::string("default");
static_assert(y.unwrap_or(default_string) == "default");

// unwrap_or_default
static_assert(y.unwrap_or_default() == "");

// unwrap_or_else
static_assert(y.unwrap_or_else([] { return std::string("computed"); }) == "computed");

// 注意：unwrap_unchecked() 仅在确定有值时使用，否则为未定义行为
auto z = opt::some(42);
int v = z.unwrap_unchecked(); // 安全
// auto w = opt::option<int>(opt::none).unwrap_unchecked(); // 未定义行为
```


### 与 `std::expected` 交互

`option` 可与 `std::expected` 互操作，便于错误传播和链式组合：
- `ok_or(error)`：若有值则转为 `std::expected<T, E>`，否则返回带指定错误的 `expected`。
- `ok_or_else(func)`：若有值则转为 `expected`，否则用函数生成错误。
- `transpose()`：将 `option<std::expected<T, E>>` 转为 `std::expected<option<T>, E>`，即把 `option` 的外层和 `expected` 的外层交换。

示例：
```cpp
// ok_or
constexpr auto x = opt::some("foo"s);
constexpr auto y = x.ok_or("error"sv);
static_assert(y.has_value() && y.value() == "foo");

constexpr auto z = opt::none_opt<std::string>;
constexpr auto w = z.ok_or("error info"sv);
static_assert(!w.has_value() && w.error() == "error info");

// ok_or_else
constexpr auto w2 = z.ok_or_else([] {
    return std::string("whatever error");
});
static_assert(!w2.has_value() && w2.error() == "whatever error");

// transpose
constexpr auto opt_exp = opt::some(std::expected<int, const char *>{ 42 });
constexpr auto exp_opt = opt_exp.transpose();
static_assert(exp_opt.has_value() && exp_opt.value() == opt::some(42));

constexpr auto opt_exp2 = opt::some(std::expected<int, const char *>{ std::unexpected{ "fail" } });
constexpr auto exp_opt2 = opt_exp2.transpose();
static_assert(!exp_opt2.has_value() && exp_opt2.error() == "fail"sv);
```

### 变换与映射

`option` 支持多种变换与映射操作，便于链式处理、类型转换和条件变换：
- `map(func)`：有值时对值应用函数，返回新 `option`，否则返回 none。
- `map_or(default, func)`：有值时应用函数，否则直接返回默认值。
- `map_or_default(func)`：有值时应用函数，否则返回类型 `func` 返回类型 `U` 的默认值。
- `map_or_else(default_func, func)`：有值时应用函数，否则调用 default_func 生成返回值。
- `filter(predicate)`：有值且满足谓词时保留，否则返回 none。
- `flatten()`：将 `option<option<T>>` 展开为 `option<T>`。
- `inspect(func)`：有值时执行副作用函数，返回自身。

示例：
```cpp
// map
constexpr auto x = opt::some(4);
constexpr auto y = x.map([](int v) { return v * 2; });
static_assert(y == opt::some(8));

constexpr auto z = opt::option<int>(opt::none);
constexpr auto w = z.map([](int v) { return v * 2; });
static_assert(w == opt::none);

// map_or
constexpr auto f = opt::some(10);
constexpr auto r1 = f.map_or(0, [](int v) { return v + 1; });
static_assert(r1 == 11);
constexpr auto r2 = opt::option<int>(opt::none).map_or(0, [](int v) { return v + 1; });
static_assert(r2 == 0);

// map_or_default
constexpr opt::option<int> v = opt::none;
constexpr auto r5 = v.map_or_default([](auto &&v) { return v + 100; });
static_assert(r5 == 0);

// map_or_else
constexpr auto r3 = f.map_or_else([] { return 100; }, [](int v) { return v * 3; });
static_assert(r3 == 30);
constexpr auto r4 = opt::option<int>(opt::none).map_or_else([] { return 100; }, [](int v) { return v * 3; });
static_assert(r4 == 100);

// filter
constexpr auto filtered = opt::some(5).filter([](int v) { return v > 3; });
static_assert(filtered == opt::some(5));
constexpr auto filtered2 = opt::some(2).filter([](int v) { return v > 3; });
static_assert(filtered2 == opt::none);

// flatten
constexpr auto nested = opt::some(opt::some(42));
constexpr auto flat = nested.flatten();
static_assert(flat == opt::some(42));

// inspect
auto log_fn = [](int v) { std::println("got value: {}", v); };
opt::some(123).inspect(log_fn); // 有值时会输出
```

### 组合与解包

`option` 支持多个 `option` 之间的组合与解包，便于并行处理、结构化数据和解构：
- `zip(other)`：若自身和 `other` 都有值，则返回包含 `pair` 的 `option`，否则返回 `none`。
- `zip_with(other, func)`：若都为 `some`，则用 `func` 合并两个值，返回新 `option`，否则返回 `none`。
- `unzip()`：将 `option<std::pair<T, U>>` 拆分为 `std::pair<option<T>, option<U>>`，即分别提取。

示例：
```cpp
// zip
constexpr auto a = opt::some(1);
constexpr auto b = opt::some(2);
constexpr auto zipped = a.zip(b);
static_assert(zipped == opt::some(std::pair{ 1, 2 }));

constexpr auto none_a = opt::none_opt<int>;
constexpr auto zipped2 = none_a.zip(b);
static_assert(zipped2 == opt::none);

constexpr auto s1 = opt::some("foo"sv);
constexpr auto s2 = opt::some("bar"sv);
constexpr auto zipped3 = s1.zip_with(s2, [](auto x, auto y) { return x.size() + y.size(); });
static_assert(zipped3 == opt::some(6zu));

// unzip
constexpr auto pair_opt = opt::some(std::pair(42, "hi"sv));
constexpr auto unzipped = pair_opt.unzip();
static_assert(unzipped.first == opt::some(42));
static_assert(unzipped.second == opt::some("hi"sv));

constexpr auto none_pair = opt::none_opt<std::pair<int, std::string_view>>;
constexpr auto unzipped2 = none_pair.unzip();
static_assert(unzipped2.first == opt::none);
static_assert(unzipped2.second == opt::none);
```

### 布尔逻辑操作

`option` 提供类似布尔逻辑的组合操作，便于条件判断、链式分支和表达式式控制流：
- `and_(other)`：若自身有值，返回 other，否则返回 none。
- `or_(other)`：若自身有值，返回自身，否则返回 other。
- `xor_(other)`：仅有一个为 some 时返回该值，否则返回 none。
- `and_then(func)`：有值时调用 func 并返回其结果（常用于链式处理），否则返回 none。
- `or_else(func)`：为空时调用 func 并返回其结果，否则返回自身。

示例：
```cpp
constexpr auto a = opt::some(1);
constexpr auto b = opt::some(2);
constexpr auto n = opt::none_opt<int>;

// and_
static_assert(a.and_(b) == b);
static_assert(n.and_(b) == opt::none);

// or_
static_assert(a.or_(b) == a);
static_assert(n.or_(b) == b);

// xor_
static_assert(a.xor_(n) == a);
static_assert(n.xor_(b) == b);
static_assert(a.xor_(b) == opt::none);
static_assert(n.xor_(n) == opt::none);

// and_then
constexpr auto f = [](int x) { return opt::some(x * 10); };
static_assert(a.and_then(f) == opt::some(10));
static_assert(n.and_then(f) == opt::none);

// or_else
constexpr auto g = [] { return opt::some(99); };
static_assert(a.or_else(g) == a);
static_assert(n.or_else(g) == opt::some(99));
```

### 比较与排序

若 `T` 支持比较，`option<T>` 也支持所有标准比较操作。规则如下：
- 空的 `option`（`none`）总是小于有值的 `option`（`some`）。
- 两个 `some` 时按值比较。
- 支持 `<`, `<=`, `>`, `>=`, `==`, `!=` 以及三路比较（`<=>`）。
- 支持 `lt`, `le`, `gt`, `ge`, `eq`, `ne` 和 `cmp`。

示例：
```cpp
static_assert(opt::none < opt::some(0));
static_assert(opt::none <= opt::none);
static_assert(opt::none.le(opt::none));
static_assert(opt::some(0) < opt::some(1));
static_assert(opt::some(0) <= opt::some(1));
static_assert(opt::some(1) > opt::none);
static_assert(opt::some(1) >= opt::some(0));
static_assert(opt::some(1).ge(opt::some(0)));
static_assert(opt::some(1) == opt::some(1));
static_assert(opt::none == opt::none);
static_assert(opt::some(1) != opt::some(0));
static_assert(opt::some(1).ne(opt::some(0)));
static_assert((opt::some(1) <=> opt::none) == std::strong_ordering::greater);
static_assert((opt::some(1).cmp(opt::some(1))) == std::strong_ordering::equal);
```

### 就地修改

`option` 支持原地修改和懒惰初始化，便于高效管理可选值：
- `insert(value)`：直接插入新值，覆盖原有内容。
- `get_or_insert(value)`：若已有值则返回引用，否则插入 value 并返回引用。
- `get_or_insert_default()`：若为空则插入类型默认值（需 T 可默认构造），返回引用。
- `get_or_insert_with(func)`：若为空则调用 func 生成值插入，返回引用。

示例：
```cpp
// insert
constexpr auto ins() {
    opt::option<int> x = opt::none;
    x.insert(123);
    return x;
}
static_assert(ins() == opt::some(123));

// get_or_insert
constexpr auto bar() {
    opt::option<int> n = opt::none;
    int &ref           = n.get_or_insert(42);
    return std::pair{ n, ref };
}
static_assert(bar() == std::pair{ opt::some(42), 42 });

// get_or_insert_default
constexpr auto baz() {
    opt::option<int> n = opt::none;
    int &ref           = n.get_or_insert_default();
    return std::pair{ n, ref };
}
static_assert(baz() == std::pair{ opt::some(0), 0 });

// get_or_insert_with
constexpr auto with() {
    opt::option<int> x = opt::none;
    int &ref           = x.get_or_insert_with([] { return 77; });
    return std::pair{ x, ref };
}
static_assert(with() == std::pair{ opt::some(77), 77 });
```

### 所有权转移

`option` 支持安全的所有权转移操作，便于值的移动和重置：
- `take()`：取出当前值并将自身置空，返回原值的 `option`。
- `replace(value)`：用新值替换当前值，返回原值的 `option`。
- `take_if(pred)`：若当前值满足条件，则取出并将自身置空，返回原值的 `option`。

示例：
```cpp
// take
constexpr auto foo() {
    auto x = opt::some(2);
    auto y = x.take();
    return std::pair{ x, y };
}
static_assert(foo() == std::pair{ opt::none, opt::some(2) });

// take_if
constexpr auto bar() {
    auto x = opt::some(3);
    auto y = x.take_if([](int v) { return v > 5; });
    return std::pair{ x, y };
}
static_assert(bar() == std::pair{ opt::some(3), opt::none });

// replace
constexpr auto baz() {
    auto s = opt::some("abc"s);
    auto old = s.replace("xyz");
    return std::pair{ s, old };
}
static_assert(baz() == std::pair{ opt::some("xyz"s), opt::some("abc"s) });
```

### 克隆与复制引用对象的值

存储引用的 `option` 支持对引用对象的显式克隆和复制操作，便于在需要时进行深层或浅层复制：
- `cloned()`：返回一个新对象，包含当前引用的值的深层复制。由于无法判断自定义类型的复制构造函数是否是深层复制，需要对象有符合语义的 `clone()` 成员函数或可平凡复制。
- `copied()`：返回一个新对象，包含当前引用的值的浅层复制。

```cpp
// cloned
static constexpr auto v_1 = 1;
constexpr auto ref = opt::some(std::ref(v_1));
static_assert(ref.cloned() == opt::some(1));

struct x {
    int v;

    constexpr x clone() const noexcept {
        return *this;
    }

    constexpr bool operator==(const x &) const = default;
};

constexpr auto v_2 = x{ .v = 1 };
static_assert(opt::some(std::ref(v_2)).cloned() == opt::some(x{ .v = 1 }));

// copied
struct y {
    constexpr bool operator==(const y &) const = default;
};
constexpr auto v_3 = y{};
static_assert(opt::some(std::ref(v_3)).copied() == opt::some(y{}));
```

## 示例

### 在循环前初始化结果为空 `option`

```cpp
enum class Kingdom { Plant, Animal };

struct BigThing {
    Kingdom kind;
    int size;
    std::string_view name;
};

constexpr std::array<BigThing, 6> all_the_big_things = {
    {
     { Kingdom::Plant, 250, std::string_view("redwood") },
     { Kingdom::Plant, 230, std::string_view("noble fir") },
     { Kingdom::Plant, 229, std::string_view("sugar pine") },
     { Kingdom::Animal, 25, std::string_view("blue whale") },
     { Kingdom::Animal, 19, std::string_view("fin whale") },
     { Kingdom::Animal, 15, std::string_view("north pacific right whale") },
     }
};

constexpr opt::option<std::string_view> find_biggest_animal_name() {
    int max_size                           = 0;
    opt::option<std::string_view> max_name = opt::none;
    for (const auto &thing : all_the_big_things) {
        if (thing.kind == Kingdom::Animal && thing.size > max_size) {
            max_size = thing.size;
            max_name = opt::some(thing.name);
        }
    }
    return max_name;
}

constexpr auto name_of_biggest_animal = find_biggest_animal_name();
static_assert(name_of_biggest_animal.is_some(), "there are no animals :(");
static_assert(name_of_biggest_animal.unwrap() == std::string_view("blue whale"),
              "the biggest animal should be blue whale");
```


### 链式方法调用与变换

```cpp
#include "option.hpp"
#include <vector>
#include <algorithm>

std::vector<opt::option<int>> options = {
    opt::some(1), 
    opt::none, 
    opt::some(3)
};

// 收集所有有值的项，我没想到比较好的 constexpr 例子
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

## 安装

本库为头文件库 / 模块库，支持多种集成方式：

- **头文件方式**：直接复制 `src/include/option.hpp` 到你的项目，并在代码中 `#include "option.hpp"`。
- **模块方式**：复制 `src/option.cppm` 到你的项目，并在代码中 `import option;`。注意需编译器支持模块和标准库模块。

## 测试与基准

### 单元测试（gtest）

本项目内置了基于 [GoogleTest](https://github.com/google/googletest) 的单元测试，测试文件为 `src/test_unit.cpp`。

支持三大主流编译器（GCC、Clang、MSVC），分别对应 target：

- GCC: `test_unit_gcc`
- Clang: `test_unit_clang`
- MSVC: `test_unit_msvc`

可通过如下命令编译并运行（以 GCC 为例）：

```sh
xmake run test_unit_gcc --file=xmake.ci.lua
```

如需自定义或扩展测试，请参考 `src/test_unit.cpp`，并确保已安装 gtest。

### 性能基准（benchmark）

性能测试基于 [Google Benchmark](https://github.com/google/benchmark)，主文件为 `src/bench.cpp`。

同样支持三大主流编译器，分别对应 target：

- GCC: `bench_gcc`
- Clang: `bench_clang`
- MSVC: `bench_msvc`

可通过如下命令编译并运行（以 Clang 为例）：

```sh
xmake run bench_clang --file=xmake.ci.lua
```

如需添加新的基准测试，请参考 `src/bench.cpp`，并确保已安装 benchmark。

## CI 持续集成

本项目已集成 GitHub Actions 自动化流程，支持自动编译、测试和基准运行。CI 配置见 `.github/workflows/ci.yml`，主要流程如下：

1. 安装编译器和 xmake
2. 编译所有目标（包括测试和基准）
3. 自动运行单元测试

如需本地模拟 CI 流程，可直接运行：

```sh
xmake --yes --file=xmake.ci.lua
xmake run test_unit --file=xmake.ci.lua
xmake run bench --file=xmake.ci.lua
```

## 编译要求

- C++23 标准支持
- 显式 `this` 参数（`__cpp_explicit_this_parameter >= 202110L`）
- `std::expected`（`__cpp_lib_expected >= 202202L`）


## 用法示例

### 基本用法

```cpp
opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none;

if (value.is_some()) {
    int x = value.unwrap();
    std::println("值: {}", x);
}

int result = empty.unwrap_or(0);
std::println("结果: {}", result);
```

### 高级用法

```cpp
opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none;

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
- `opt::none` —— 空 option
- `opt::none_opt<T>` —— 创建类型为 T 的空 option

### 典型用例
```cpp
// 查找函数
opt::option<std::string> find_user_name(int user_id) {
    if (user_id == 42) {
        return opt::some(std::string("Alice"));
    }
    return opt::none;
}

// 解析函数
opt::option<int> parse_int(const std::string& str) {
    try {
        return opt::some(std::stoi(str));
    } catch (...) {
        return opt::none;
    }
}

// 安全数组访问
template<typename T>
opt::option<T> safe_get(const std::vector<T>& vec, size_t index) {
    if (index < vec.size()) {
        return opt::some(vec[index]);
    }
    return opt::none;
}
```

## 许可证

详见 LICENSE 文件。
