# cpp-option

[中文](README_zh.md)

`cpp-option` is a generic library based on C++23, implementing a type-safe optional value container similar to Rust's `Option` type. The library aims to provide safer and more concise handling of nullable values in C++, supporting rich operations and modern C++ features.

## Overview

`opt::option<T>` represents an optional value: it either contains a value of type `T` (Some), or is empty (None). This type is useful for:

- Initial values for variables
- Return values for functions that may fail (e.g., lookup, parse)
- Simple error reporting (None means error)
- Optional struct fields
- Optional function parameters
- Safe wrapper for nullable pointers
- Value swapping and ownership transfer in complex scenarios

With `option`, you can explicitly handle the presence or absence of a value, avoiding common errors like null pointer dereference or uninitialized variables.

```cpp
#include "option.hpp"

opt::option<double> divide(double numerator, double denominator) {
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

## `option` and Raw Pointers

C++ raw pointers can be null, which may lead to undefined behavior. `option` can safely wrap raw pointers, but it's recommended to use smart pointers (`std::unique_ptr`, `std::shared_ptr`) when possible.

```cpp
#include "option.hpp"

constexpr auto whatever = 1;
static_assert(opt::some(&whatever).unwrap() == &whatever);

static_assert(opt::some(&whatever).is_some());

static_assert(opt::some(nullptr).unwrap() == nullptr);

void check_optional(const opt::option<int*>& optional) {
    if (optional.is_some()) {
        std::println("Has value {}", *optional.unwrap());
    } else {
        std::println("No value");
    }
}

int main() {
    auto optional = opt::none_opt<int*>();
    check_optional(optional);

    int value = 9000;
    auto optional2 = opt::some(&value);
    check_optional(optional2);
}
```

Additionally, `option` supports pointer optimization (cannot `constexpr`). Define `OPT_OPTION_PTR_OPTIMIZATION` to enable this feature.

```cpp
#define OPT_OPTION_PTR_OPTIMIZATION
#include "option.hpp"
#include <cassert>

auto v     = 0;
auto opt_1 = opt::some(&v);
static_assert(sizeof(opt_1) == sizeof(void *));

assert(opt_1.is_some());
assert(opt_1.unwrap() == &v);

auto opt_2 = opt::some<int *>(nullptr);
assert(opt_2.is_some());
assert(opt_2.unwrap() == nullptr);

auto opt_3 = opt::none_opt<int *>();
assert(opt_3.is_none());
assert(opt_3 != opt_2);
```

## Error Handling and Chaining

When multiple operations may return `option`, you can use method chaining to simplify nested checks:

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

## Implementation Notes

This library uses a union-based storage mechanism, supporting value types, reference types (as pointers), and `void` (presence only), combining Rust `Option` semantics with C++23 features:

- Complete move/copy semantics and perfect forwarding
- Extensive `constexpr` support
- Explicit `this` parameter (deducing this)
- Integration with standard types like `std::expected`, `std::pair`, etc.

### Special Optimization

Define `OPT_OPTION_PTR_OPTIMIZATION` to enable pointer optimization. For pointer types, `option` uses only pointer-sized storage.

## Method Overview

Besides basic checks, `option` provides a rich set of member methods for state queries, value extraction, transformation, combination, in-place modification, and type conversion.

### State Queries

`option` provides several state query methods for easy branching:
- `is_some()`: whether value is present
- `is_none()`: whether value is absent
- `is_some_and(pred)`: true if value is present and predicate holds
- `is_none_or(pred)`: true if value is absent or predicate holds

```cpp
// is_some
constexpr auto x = opt::some(2);
static_assert(x.is_some());

// is_none
constexpr auto y = opt::none_opt<int>();
static_assert(y.is_none());

// is_some_and
constexpr auto x = opt::some(2);
static_assert(x.is_some_and([](int x) { return x > 1; }));

// is_none_or
constexpr auto y = opt::none_opt<int>();
static_assert(y.is_none_or([](int x) { return x == 2; }));
```

### Reference Adapters

`option` supports reference adapters for safe access:
- `as_ref()`: to `option<const T&>`
- `as_mut()`: to `option<T&>`
- `as_deref()`: dereference to `option<const U&>`
- `as_deref_mut()`: dereference to `option<U&>`

```cpp
int value = 42;
opt::option<std::unique_ptr<int>> x = opt::some(std::make_unique<int>(value));
static_assert(std::same_as<decltype(x.as_deref()), opt::option<const int &>>);
static_assert(std::same_as<decltype(x.as_deref_mut()), opt::option<int &>>);

opt::option<int *> y = opt::some(&value);
static_assert(std::same_as<decltype(y.as_deref()), opt::option<const int &>>);
static_assert(std::same_as<decltype(y.as_deref_mut()), opt::option<int &>>);
```

### Value Extraction

`option` provides several ways to extract values:
- `unwrap()`: extract value, throws if none
- `expect(msg)`: like `unwrap()` but with custom message
- `unwrap_or(default)`: return default if none
- `unwrap_or_default()`: return default-constructed value if none
- `unwrap_or_else(func)`: call function if none
- `unwrap_unchecked()`: unchecked extraction (undefined if none)

```cpp
// unwrap
constexpr auto x = opt::some(std::string("value"));
static_assert(x.unwrap() == "value");

// expect
static_assert(x.expect("should have value") == "value");

// unwrap_or
constexpr auto y = opt::none_opt<std::string>();
constexpr auto default_string = std::string("default");
static_assert(y.unwrap_or(default_string) == "default");

// unwrap_or_default
static_assert(y.unwrap_or_default() == "");

// unwrap_or_else
static_assert(y.unwrap_or_else([] { return std::string("computed"); }) == "computed");

// Note: unwrap_unchecked() is only safe if value is present
auto z = opt::some(42);
int v = z.unwrap_unchecked(); // safe
// auto w = opt::none_opt<int>().unwrap_unchecked(); // undefined behavior
```

### Interop with `std::expected`

`option` can interoperate with `std::expected`:
- `ok_or(error)`: to `std::expected<T, E>`, or error if none
- `ok_or_else(func)`: to `std::expected<T, E>`, or function result if none
- `transpose()`: `option<std::expected<T, E>>` to `std::expected<option<T>, E>`

```cpp
// ok_or
constexpr auto x = opt::some(std::string("foo"));
constexpr auto y = x.ok_or("error"sv);
static_assert(y.has_value() && y.value() == "foo");

constexpr auto z = opt::none_opt<std::string>();
constexpr auto w = z.ok_or("error info"sv);
static_assert(!w.has_value() && w.error() == "error info");

// ok_or_else
constexpr auto err_fn = [] {
    return std::string("whatever error");
};
constexpr auto w2 = z.ok_or_else(err_fn);
static_assert(!w2.has_value() && w2.error() == "whatever error");

// transpose
constexpr auto opt_exp = opt::some(std::expected<int, const char *>{ 42 });
constexpr auto exp_opt = opt_exp.transpose();
static_assert(exp_opt.has_value() && exp_opt.value() == opt::some(42));

constexpr auto opt_exp2 = opt::some(std::expected<int, const char *>{ std::unexpected("fail") });
constexpr auto exp_opt2 = opt_exp2.transpose();
static_assert(!exp_opt2.has_value() && exp_opt2.error() == std::string_view("fail"));
```

### Transformation and Mapping

`option` supports various transformation and mapping operations:
- `map(func)`: apply function if value present
- `map_or(default, func)`: apply function if present, else return default
- `map_or_else(default_func, func)`: apply function if present, else call default_func
- `filter(predicate)`: keep value if predicate holds
- `flatten()`: flatten nested `option<option<T>>`
- `inspect(func)`: run side-effect if value present

```cpp
// map
constexpr auto x = opt::some(4);
constexpr auto y = x.map([](int v) { return v * 2; });
static_assert(y == opt::some(8));

constexpr auto z = opt::none_opt<int>();
constexpr auto w = z.map([](int v) { return v * 2; });
static_assert(w == opt::none);

// map_or
constexpr auto f = opt::some(10);
constexpr auto r1 = f.map_or(0, [](int v) { return v + 1; });
static_assert(r1 == 11);
constexpr auto r2 = opt::none_opt<int>().map_or(0, [](int v) { return v + 1; });
static_assert(r2 == 0);

// map_or_else
constexpr auto r3 = f.map_or_else([] { return 100; }, [](int v) { return v * 3; });
static_assert(r3 == 30);
constexpr auto r4 = opt::none_opt<int>().map_or_else([] { return 100; }, [](int v) { return v * 3; });
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
opt::some(123).inspect(log_fn); // prints if value present
```

### Combination and Unpacking

`option` supports combining and unpacking:
- `zip(other)`: if both present, returns option of pair
- `zip_with(other, func)`: if both present, combine with func
- `unzip()`: option of pair to pair of options

```cpp
// zip
constexpr auto a = opt::some(1);
constexpr auto b = opt::some(2);
constexpr auto zipped = a.zip(b);
static_assert(zipped == opt::some(std::pair(1, 2)));

constexpr auto none_a = opt::none_opt<int>();
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

constexpr auto none_pair = opt::none_opt<std::pair<int, std::string_view>>();
constexpr auto unzipped2 = none_pair.unzip();
static_assert(unzipped2.first == opt::none);
static_assert(unzipped2.second == opt::none);
```

### Boolean Logic Operations

`option` provides boolean-like logic operations:
- `and_(other)`: if present, return other; else none
- `or_(other)`: if present, return self; else other
- `xor_(other)`: only one present, return it; else none
- `and_then(func)`: if present, call func
- `or_else(func)`: if none, call func

```cpp
constexpr auto a = opt::some(1);
constexpr auto b = opt::some(2);
constexpr auto n = opt::none_opt<int>();

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

### Comparison and Ordering

If `T` supports comparison, so does `option<T>`. None is always less than Some, and Some is compared by value.

```cpp
static_assert(opt::none < opt::some(0));
static_assert(opt::some(0) < opt::some(1));
static_assert(opt::some(1) > opt::none);
static_assert(opt::some(1) == opt::some(1));
static_assert(opt::none == opt::none);
static_assert((opt::some(1) <=> opt::none) == std::strong_ordering::greater);
```

### In-place Modification

`option` supports in-place modification and lazy initialization:
- `insert(value)`: insert new value
- `get_or_insert(value)`: get or insert value
- `get_or_insert_default()`: insert default if none
- `get_or_insert_with(func)`: insert value from function if none

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

### Ownership Transfer

`option` supports safe ownership transfer:
- `take()`: take value and set to none
- `replace(value)`: replace with new value, return old value

```cpp
constexpr auto foo() {
    auto x = opt::some(2);
    auto y = x.take();
    return std::pair{ x, y };
}
static_assert(foo() == std::pair{ opt::none, opt::some(2) });

constexpr auto bar() {
    auto s = opt::some("abc"s);
    auto old = s.replace("xyz");
    return std::pair{ s, old };
}
static_assert(bar() == std::pair{ opt::some("xyz"s), opt::some("abc"s) });
```

## Examples

### Initialize result as empty `option` before loop

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

### Chaining and Transformation

```cpp
#include "option.hpp"
#include <vector>
#include <algorithm>

std::vector<opt::option<int>> options = {
    opt::some(1), 
    opt::none, 
    opt::some(3)
};

// Collect all present values
std::vector<int> values;
for (const auto& opt : options) {
    if (opt.is_some()) {
        values.push_back(opt.unwrap());
    }
}
// [1, 3]

// Pipeline transformation and filtering
constexpr auto process = [](int x) -> opt::option<int> {
    return opt::some(x)
        .filter([](int y) { return y > 0; })
        .map([](int y) { return y * 2; });
};

static_assert(process(5) == opt::some(10));
```

## Installation

This library is header-only / module-based, supporting multiple integration methods:

- **Header**: Copy `src/include/option.hpp` to your project and `#include "option.hpp"`.
- **Module**: Copy `src/option.cppm` to your project and use `import option;`. Requires compiler support for modules and standard library modules.

## Testing & Benchmark

### Unit Test (gtest)

Unit tests are based on [GoogleTest](https://github.com/google/googletest), with main file at `src/test_unit.cpp`.

Supported on three major compilers (GCC, Clang, MSVC), with corresponding targets:

- GCC: `test_unit_gcc`
- Clang: `test_unit_clang`
- MSVC: `test_unit_msvc`

Example command (GCC):

```sh
xmake run test_unit_gcc --file=xmake.ci.lua
```

To customize or extend tests, refer to `src/test_unit.cpp` and ensure gtest is installed.

### Benchmark (Google Benchmark)

Benchmarks are based on [Google Benchmark](https://github.com/google/benchmark), with main file at `src/bench.cpp`.

Also supported on three major compilers, with corresponding targets:

- GCC: `bench_gcc`
- Clang: `bench_clang`
- MSVC: `bench_msvc`

Example command (Clang):

```sh
xmake run bench_clang --file=xmake.ci.lua
```

To add new benchmarks, refer to `src/bench.cpp` and ensure benchmark is installed.

## CI (Continuous Integration)

This project uses GitHub Actions for automated build, test, and benchmark. See `.github/workflows/ci.yml` for details. Main steps:

1. Install GCC/Clang/MSVC and xmake
2. Build all targets (including tests and benchmarks)
3. Run unit tests and benchmarks automatically

To simulate CI locally:

```sh
xmake --yes --file=xmake.ci.lua
xmake run test_unit_gcc --file=xmake.ci.lua
xmake run bench_gcc --file=xmake.ci.lua
```

## Build Requirements

- C++23 standard support
- Explicit `this` parameter (`__cpp_explicit_this_parameter >= 202110L`)
- `std::expected` (`__cpp_lib_expected >= 202202L`)

## Usage Examples

### Basic Usage

```cpp
opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none;

if (value.is_some()) {
    int x = value.unwrap();
    std::println("Value: {}", x);
}

int result = empty.unwrap_or(0);
std::println("Result: {}", result);
```

### Advanced Usage

```cpp
opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none;

// map operation
auto doubled = value.map([](int x) { return x * 2; });
std::println("Doubled: {}", doubled);

// or_else operation
auto fallback = empty.or_else([]() { return opt::some(99); });
std::println("Fallback: {}", fallback);

// Chaining
opt::option<int> combined = value.and_then([](int x) { 
    return opt::some(x + 1); 
});
std::println("Combined: {}", combined);
```

## Core Methods

### Creation
- `opt::some(value)` — Create an option with value
- `opt::none` — Empty option
- `opt::none_opt<T>()` — Create an empty option of type T

### Typical Use Cases

```cpp
// Lookup function
opt::option<std::string> find_user_name(int user_id) {
    if (user_id == 42) {
        return opt::some(std::string("Alice"));
    }
    return opt::none;
}

// Parse function
opt::option<int> parse_int(const std::string& str) {
    try {
        return opt::some(std::stoi(str));
    } catch (...) {
        return opt::none;
    }
}

// Safe array access
template<typename T>
opt::option<T> safe_get(const std::vector<T>& vec, size_t index) {
    if (index < vec.size()) {
        return opt::some(vec[index]);
    }
    return opt::none;
}
```

## License

See LICENSE file for details.
