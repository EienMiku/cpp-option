# cpp-option

[中文版](README_zh.md)

A C++23 implementation of Rust's `Option` type for type-safe null value handling.

## Overview

The `option` type represents an optional value: every `option` either contains a value or is empty. `option` types are very common in Rust code, as they have a number of uses:

- Initial values
- Return values for functions that are not defined over their entire input range (partial functions)
- Return value for otherwise reporting simple errors, where an empty `option` is returned on error
- Optional struct fields
- Struct fields that can be loaned or "taken"
- Optional function arguments  
- Nullable pointers
- Swapping things out of difficult situations

`option`s are commonly paired with conditional checks to query the presence of a value and take action, always accounting for the empty case.

```cpp
#include "option.hpp"

opt::option<double> divide(double numerator, double denominator) {
    if (denominator == 0.0) {
        return opt::none<double>();
    } else {
        return opt::some(numerator / denominator);
    }
}

auto result = divide(2.0, 3.0);

// Check for the presence of a value
if (result.is_some()) {
    // The division was valid
    std::println("The result is: {}", result);
} else {
    // The division was invalid
    std::println("Cannot divide by 0");
}
```

## Options and raw pointers

C++'s raw pointer types can point to null; however, this leads to null pointer dereferences and undefined behavior. `option` can be used to wrap raw pointers for additional safety, though modern C++ prefers smart pointers for automatic memory management.

```cpp
#include "option.hpp"

void check_optional(const opt::option<int*>& optional) {
    if (optional.is_some()) {
        std::println("has value {}", *optional.unwrap());
    } else {
        std::println("has no value");
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

Note: For modern C++ code, consider using smart pointers (`std::unique_ptr`, `std::shared_ptr`) directly instead of wrapping them in `option`, as smart pointers already provide memory safety and null-checking capabilities.

## Error handling pattern

Similar to Rust, when writing code that calls many functions that return the `option` type, handling values and empty states can be tedious. Method chaining provides a clean way to handle this:

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

## Representation

This C++ implementation uses a union-based storage similar to `std::optional`, but with additional features inspired by Rust's `Option`. The implementation supports:

- Value types
- Reference types (via pointer storage)
- `void` type (representing boolean-like "has value" or "empty" states)
- Move semantics and perfect forwarding
- `constexpr` operations where possible

## Method Overview

In addition to working with conditional checks, `option` provides a wide variety of different methods.

### Querying the variant

The `is_some()` and `is_none()` methods return `true` if the `option` contains a value or is empty, respectively.

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some());

constexpr auto y = opt::none<int>();
static_assert(y.is_none());
```

The `is_some_and()` and `is_none_or()` methods test the contained value against a predicate:

```cpp
constexpr auto x = opt::some(2);
static_assert(x.is_some_and([](int x) { return x > 1; }));

constexpr auto y = opt::none<int>();
static_assert(y.is_none_or([](int x) { return x == 2; }));
```

### Adapters for working with references

- `as_ref()` converts from `option<T>` to `option<const T&>`
- `as_mut()` converts from `option<T>` to `option<T&>`
- `as_deref()` converts from `option<T>` to `option<const T::element_type&>` (for smart pointers)
- `as_deref_mut()` converts from `option<T>` to `option<T::element_type&>` (for smart pointers)

```cpp
constexpr auto x = opt::some(std::string("hello"));
static_assert(std::same_as<decltype(x.as_ref()), opt::option<const std::string &>>);
```

### Extracting the contained value

These methods extract the contained value in an `option<T>` when it contains a value. If the `option` is empty:

- `expect()` throws with a provided custom message
- `unwrap()` throws with a generic message
- `unwrap_or()` returns the provided default value
- `unwrap_or_default()` returns the default value of the type `T`
- `unwrap_or_else()` returns the result of evaluating the provided function
- `unwrap_unchecked()` produces undefined behavior if called on `None`

```cpp
constexpr auto x = opt::some(std::string("value"));
static_assert(x.expect("should have a value") == "value");
static_assert(x.unwrap() == "value");

constexpr auto y = opt::none<std::string_view>();
static_assert(y.unwrap_or("default"sv) == "default"sv);
static_assert(y.unwrap_or_default() == "");
static_assert(y.unwrap_or_else([]() { return "computed"sv; }) == "computed"sv);
```

### Transforming contained values

These methods transform `option` to `std::expected`:

- `ok_or()` transforms an `option` with a value to `std::expected` with that value, and an empty `option` to `std::unexpected` with the provided error value
- `ok_or_else()` transforms an `option` with a value to `std::expected` with that value, and an empty `option` to `std::unexpected` with the result of the provided function

```cpp
constexpr auto x = opt::some(std::string("foo"));
constexpr auto y = x.ok_or("error"sv);
static_assert(y.value() == "foo");

constexpr auto z = opt::none<std::string>();
constexpr auto w = z.ok_or("error"sv);
static_assert(w.error() == "error");
```

- `transpose()` transposes an `option` of a `std::expected` into a `std::expected` of an `option`

These methods transform the value when present:

- `filter()` calls the provided predicate function on the contained value if present, and returns an `option` with that value if the function returns `true`; otherwise, returns an empty `option`
- `flatten()` removes one level of nesting from an `option<option<T>>`
- `inspect()` calls the provided function with the contained value if present, and returns the original `option`
- `map()` transforms `option<T>` to `option<U>` by applying the provided function to the contained value when present and leaving empty `option`s unchanged

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

These methods transform `option<T>` to a value of a possibly different type `U`:

- `map_or()` applies the provided function to the contained value when present, or returns the provided default value if empty
- `map_or_else()` applies the provided function to the contained value when present, or returns the result of evaluating the provided fallback function if empty

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

These methods combine values from two `option`s:

- `zip()` returns an `option` containing `std::pair(s, o)` if both `option`s contain values; otherwise, returns an empty `option`
- `zip_with()` calls the provided function and returns an `option` containing `f(s, o)` if both `option`s contain values; otherwise, returns an empty `option`

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

### Boolean operators

These methods treat the `option` as a boolean value, where a value-containing `option` acts like `true` and an empty `option` acts like `false`. There are two categories of these methods: ones that take an `option` as input, and ones that take a function as input (to be lazily evaluated).

The `and_()`, `or_()`, and `xor_()` methods take another `option` as input, and produce an `option` as output:

| method | self | other | output |
|--------|------|-------|--------|
| `and_()` | empty | (ignored) | empty |
| `and_()` | has value(x) | empty | empty |
| `and_()` | has value(x) | has value(y) | has value(y) |
| `or_()` | empty | empty | empty |
| `or_()` | empty | has value(y) | has value(y) |
| `or_()` | has value(x) | (ignored) | has value(x) |
| `xor_()` | empty | empty | empty |
| `xor_()` | empty | has value(y) | has value(y) |
| `xor_()` | has value(x) | empty | has value(x) |
| `xor_()` | has value(x) | has value(y) | empty |

```cpp
constexpr auto x = opt::some(2);
constexpr auto y = opt::none<int>();

static_assert(x.and_(y).is_none());
static_assert(x.or_(y).is_some());
static_assert(x.xor_(y).is_some());
```

The `and_then()` and `or_else()` methods take a function as input, and only evaluate the function when they need to produce a new value:

| method | self | function input | function output | final output |
|--------|------|---------------|----------------|--------------|
| `and_then()` | empty | (not provided) | (not evaluated) | empty |
| `and_then()` | has value(x) | `x` | empty | empty |
| `and_then()` | has value(x) | `x` | has value(y) | has value(y) |
| `or_else()` | empty | (not provided) | empty | empty |
| `or_else()` | empty | (not provided) | has value(y) | has value(y) |
| `or_else()` | has value(x) | (not provided) | (not evaluated) | has value(x) |

```cpp
constexpr auto x = opt::some(2);
constexpr auto y = x.and_then([](int x) { return opt::some(x * 2); });
static_assert(y == opt::some(4));

constexpr auto z = opt::none<int>();
constexpr auto w = z.or_else([]() { return opt::some(42); });
static_assert(w == opt::some(42));
```

### Comparison operators

If `T` implements comparison operators, then `option<T>` will derive its comparison implementation. With this order, an empty `option` compares as less than any `option` containing a value, and two value-containing `option`s compare the same way as their contained values would in `T`.

```cpp
static_assert(opt::none<int>() < opt::some(0));
static_assert(opt::some(0) < opt::some(1));
```

### Modifying an `option` in-place

These methods return a mutable reference to the contained value of an `option<T>`:

- `insert()` inserts a value, dropping any old contents
- `get_or_insert()` gets the current value, inserting a provided default value if empty
- `get_or_insert_default()` gets the current value, inserting the default value if empty
- `get_or_insert_with()` gets the current value, inserting a default computed by the provided function if empty

```cpp
constexpr auto foo() {
    auto x  = opt::none<int>();
    auto &y = x.get_or_insert(5);
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::some(5), 5 });
```

These methods transfer ownership of the contained value of an `option`:

- `take()` takes ownership of the contained value if any, replacing the `option` with an empty state
- `replace()` takes ownership of the contained value if any, replacing the `option` with one containing the provided value

```cpp
constexpr auto foo() {
    auto x = opt::some(2);
    auto y = x.take();
    return std::pair{ x, y };
}

static_assert(foo() == std::pair{ opt::none<int>(), opt::some(2) });
```

## Examples

### Initialize a result to `None` before a loop

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

### Chaining operations with method calls

```cpp
#include "option.hpp"
#include <vector>
#include <algorithm>

std::vector<opt::option<int>> options = {
    opt::some(1), 
    opt::none<int>(), 
    opt::some(3)
};

// Collect all some values
std::vector<int> values;
for (const auto& opt : options) {
    if (opt.is_some()) {
        values.push_back(opt.unwrap());
    }
}
// [1, 3]

// Transform and filter in a pipeline
constexpr auto process = [](int x) -> opt::option<int> {
    return opt::some(x)
        .filter([](int y) { return y > 0; })
        .map([](int y) { return y * 2; });
};

static_assert(process(5) == opt::some(10));
```

## Requirements

- C++23 compatible compiler
- Support for explicit `this` parameters (`__cpp_explicit_this_parameter >= 202110L`)
- Support for `std::expected` (`__cpp_lib_expected >= 202202L`)

## Usage

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

// Map operation
auto doubled = value.map([](int x) { return x * 2; });
std::println("Doubled: {}", doubled);

// Or_else operation
auto fallback = empty.or_else([]() { return opt::some(99); });
std::println("Fallback: {}", fallback);

// Chain operations
opt::option<int> combined = value.and_then([](int x) { 
    return opt::some(x + 1); 
});
std::println("Combined: {}", combined);
```

## Core Methods

### Creation
- `opt::some(value)` - create an option containing a value
- `opt::none<T>()` - create an empty option

### Typical Use Cases
```cpp
// Function that may fail to find a value
opt::option<std::string> find_user_name(int user_id) {
    if (user_id == 42) {
        return opt::some(std::string("Alice"));
    }
    return opt::none<std::string>();
}

// Parsing that may fail
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

### Querying
- `is_some()` - check if the option contains a value
- `is_none()` - check if the option is empty
- `is_some_and(predicate)` - check if the option contains a value that satisfies the predicate
- `is_none_or(predicate)` - check if the option is empty or the value satisfies the predicate

### Value Extraction
- `unwrap()` - extract the value, throw if empty
- `unwrap_or(default)` - extract the value or return a default
- `unwrap_or_default()` - extract the value or return a default-constructed value
- `unwrap_or_else(func)` - extract the value or return the result of calling func
- `expect(message)` - extract the value or throw with a custom message
- `unwrap_unchecked()` - extract the value without checking (undefined behavior if empty)

### Transformation
- `map(func)` - transform the contained value
- `map_or(default, func)` - transform the contained value or return default
- `map_or_else(default_func, func)` - transform the contained value or call default_func
- `and_then(func)` - chain operations that return option
- `filter(predicate)` - filter the value by a predicate
- `flatten()` - flatten nested options

### Boolean Operations
- `and_(other)` - return other if this is Some, otherwise None
- `or_(other)` - return this if this is Some, otherwise other
- `xor_(other)` - return Some if exactly one of this and other is Some
- `or_else(func)` - return this if this is Some, otherwise call func

### Conversion
- `ok_or(error)` - convert to std::expected with error value
- `ok_or_else(error_func)` - convert to std::expected with error from func
- `transpose()` - transpose `option<expected<T, E>>` to `expected<option<T>, E>`

### Reference Adapters
- `as_ref()` - convert to option of const reference
- `as_mut()` - convert to option of mutable reference
- `as_deref()` - dereference the contained value
- `as_deref_mut()` - dereference the contained value mutably

### In-place Modification
- `insert(value)` - insert value, returning reference to it
- `get_or_insert(value)` - get current value or insert default
- `get_or_insert_default()` - get current value or insert T{}
- `get_or_insert_with(func)` - get current value or insert result of func
- `take()` - take the contained value, leaving None
- `replace(value)` - replace the contained value

### Combining Options
- `zip(other)` - combine two options into option of pair
- `zip_with(other, func)` - combine two options with a function
- `unzip()` - split option of pair into pair of options

### Inspection
- `inspect(func)` - call func with the contained value if Some

### Comparison
- `cmp(other)` - three-way comparison
- `eq(other)`, `ne(other)`, `lt(other)`, `le(other)`, `gt(other)`, `ge(other)` - comparison operations
- `max(other)`, `min(other)` - get maximum/minimum of two options
- `clamp(min, max)` - clamp option between min and max

### Copying and Cloning
- `copied()` - create a copy of the option
- `cloned()` - create a clone of the option (if T has clone() method)

## License

See LICENSE file.
