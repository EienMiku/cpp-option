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
        return opt::none<double>();
    }
    return opt::some(numerator / denominator);
}

auto result = divide(2.0, 3.0);
if (result.is_some()) {
    std::println("Result: {}", result.unwrap());
} else {
    std::println("Cannot divide by zero");
}
```

## `option` vs Raw Pointers

C++ raw pointers can be null, which may lead to undefined behavior. `option` can safely wrap raw pointers, but it's recommended to use smart pointers (`std::unique_ptr`, `std::shared_ptr`) when possible.

```cpp
#include "option.hpp"

void check_optional(const opt::option<int*>& optional) {
    if (optional.is_some()) {
        std::println("Has value {}", *optional.unwrap());
    } else {
        std::println("No value");
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

## Implementation Notes

This library uses a union-based storage mechanism, supporting value types, reference types (as pointers), and `void` (presence only), combining Rust `Option` semantics with C++23 features:

- Complete move/copy semantics and perfect forwarding
- Extensive `constexpr` support
- Explicit `this` parameter (deducing this)
- Integration with standard types like `std::expected`, `std::pair`, etc.

## Method Overview

Besides basic checks, `option` provides a rich set of member methods for state queries, value extraction, transformation, combination, in-place modification, and type conversion.

### State Queries

`is_some()` / `is_none()`: Check if value is present.

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some());

constexpr auto y = opt::none<int>();
static_assert(y.is_none());
```

`is_some_and()` and `is_none_or()` can be used with predicates:

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some_and([](int x) { return x > 1; }));

constexpr auto y = opt::none<int>();
static_assert(y.is_none_or([](int x) { return x == 2; }));
```

### Reference Adapters

- `as_ref()`: Convert to `option<const T&>`
- `as_mut()`: Convert to `option<T&>`
- `as_deref()`: Dereference and convert to `option<const T::element_type&>` (for pointers/smart pointers)
- `as_deref_mut()`: Dereference and convert to `option<T::element_type&>`

```cpp
constexpr auto x = opt::some(std::string("hello"));
static_assert(std::same_as<decltype(x.as_ref()), opt::option<const std::string &>>);
```

### Value Extraction

- `unwrap()`: Extract value, throws if None
- `expect(msg)`: Throws with custom message if None
- `unwrap_or(default)`: Returns default if None
- `unwrap_or_default()`: Returns default-constructed value if None
- `unwrap_or_else(func)`: Returns result of function if None
- `unwrap_unchecked()`: Extract without check (undefined if None)

```cpp
constexpr auto x = opt::some(std::string("value"));
static_assert(x.expect("should have a value") == "value");
static_assert(x.unwrap() == "value");

constexpr auto y = opt::none<std::string_view>();
static_assert(y.unwrap_or("default"sv) == "default"sv);
static_assert(y.unwrap_or_default() == "");
static_assert(y.unwrap_or_else([]() { return "computed"sv; }) == "computed"sv);
```

### Conversion to `std::expected`

- `ok_or(error)`: Converts to `std::expected<T, E>`, or error if None
- `ok_or_else(func)`: Converts to `std::expected<T, E>`, or result of function if None
- `transpose()`: Converts `option<std::expected<T, E>>` to `std::expected<option<T>, E>`

```cpp
constexpr auto x = opt::some(std::string("foo"));
constexpr auto y = x.ok_or("error"sv);
static_assert(y.value() == "foo");

constexpr auto z = opt::none<std::string>();
constexpr auto w = z.ok_or("error"sv);
static_assert(w.error() == "error");
```

### Transformation and Mapping

- `map(func)`: Apply function if value is present, return new `option`
- `map_or(default, func)`: Apply function if present, else return default
- `map_or_else(default_func, func)`: Apply function if present, else return result of fallback function
- `filter(predicate)`: Filter value by predicate
- `flatten()`: Flatten one level of nested `option<option<T>>`
- `inspect(func)`: Run side-effect function if value is present, return self

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

These methods convert `option<T>` to another type `U`:

- `map_or()`: Apply function if present, else return default
- `map_or_else()`: Apply function if present, else return fallback function result

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

### Combination and Unpacking

- `zip(other)`: If both have value, returns `option<std::pair<T, U>>`
- `zip_with(other, func)`: If both have value, combine with function
- `unzip()`: `option<std::pair<T, U>>` to `pair<option<T>, option<U>>`

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

### Boolean Logic Operations

- `and_(other)`: If value present, return other; else None
- `or_(other)`: If value present, return self; else other
- `xor_(other)`: If only one has value, return it; else None
- `and_then(func)`: If value present, call function and return new `option`
- `or_else(func)`: If None, call function and return new `option`

### Comparison and Ordering

If `T` supports comparison, so does `option<T>`. None is always less than Some, and Some is compared by value.

```cpp
static_assert(opt::none<int>() < opt::some(0));
static_assert(opt::some(0) < opt::some(1));
```

### In-place Modification

- `insert(value)`: Insert new value, discarding old
- `get_or_insert(value)`: Get current value, or insert default if None
- `get_or_insert_default()`: Insert default-constructed value if None
- `get_or_insert_with(func)`: Insert value from function if None

```cpp
constexpr auto foo() {
    auto x  = opt::none<int>();
    auto &y = x.get_or_insert(5);
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::some(5), 5 });
```

### Ownership Transfer

- `take()`: Take value and set to None
- `replace(value)`: Replace with new value, return old value

```cpp
constexpr auto foo() {
    auto x = opt::some(2);
    auto y = x.take();
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::none<int>(), opt::some(2) });
```

## Examples

### Initialize result as empty `option` before loop

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

### Chaining and Transformation

```cpp
#include "option.hpp"
#include <vector>
#include <algorithm>

std::vector<opt::option<int>> options = {
    opt::some(1), 
    opt::none<int>(), 
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

## Build Requirements

- C++23 standard support
- Explicit `this` parameter (`__cpp_explicit_this_parameter >= 202110L`)
- `std::expected` (`__cpp_lib_expected >= 202202L`)

## Usage Examples

### Basic Usage
```cpp
#include "option.hpp"

opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none<int>();

if (value.is_some()) {
    int x = value.unwrap();
    std::println("Value: {}", x);
}

int result = empty.unwrap_or(0);
std::println("Result: {}", result);
```

### Advanced Usage
```cpp
#include "option.hpp"

opt::option<int> value = opt::some(42);
opt::option<int> empty = opt::none<int>();

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
- `opt::none<T>()` — Create an empty option

### Typical Use Cases
```cpp
// Lookup function
opt::option<std::string> find_user_name(int user_id) {
    if (user_id == 42) {
        return opt::some(std::string("Alice"));
    }
    return opt::none<std::string>();
}

// Parse function
opt::option<int> parse_int(const std::string& str) {
    try {
        return opt::some(std::stoi(str));
    } catch (...) {
        return opt::none<int>();
    }
}

// Safe array access
template<typename T>
opt::option<T> safe_get(const std::vector<T>& vec, size_t index) {
    if (index < vec.size()) {
        return opt::some(vec[index]);
    }
    return opt::none<T>();
}
```

## License

See LICENSE file for details.
