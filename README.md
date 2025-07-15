# cpp-option

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
- `opt::some(value)` - create option with value
- `opt::none<T>` - create empty option
- `opt::some()` - create void option

### Checking
- `is_some()` - check if option has value
- `is_none()` - check if option is empty

### Value extraction
- `unwrap()` - extract value, panic if empty
- `unwrap_or(default)` - extract value or return default
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
