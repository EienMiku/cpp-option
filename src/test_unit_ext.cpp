#include <cassert>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "option.hpp"

// ============================================================================
// Helpers
// ============================================================================

struct MoveOnly {
    int value;
    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(MoveOnly &&other) noexcept : value(other.value) { other.value = -1; }
    MoveOnly &operator=(MoveOnly &&other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }
    MoveOnly(const MoveOnly &) = delete;
    MoveOnly &operator=(const MoveOnly &) = delete;
    bool operator==(const MoveOnly &other) const = default;
    auto operator<=>(const MoveOnly &other) const = default;
};

struct Tracked {
    int copies = 0;
    int moves = 0;
    int value = 0;

    constexpr Tracked() = default;
    constexpr explicit Tracked(int v) : value(v) {}
    constexpr Tracked(const Tracked &o) : copies(o.copies + 1), moves(o.moves), value(o.value) {}
    constexpr Tracked(Tracked &&o) noexcept : copies(o.copies), moves(o.moves + 1), value(o.value) {}
    constexpr Tracked &operator=(const Tracked &o) {
        copies = o.copies + 1;
        moves = o.moves;
        value = o.value;
        return *this;
    }
    constexpr Tracked &operator=(Tracked &&o) noexcept {
        copies = o.copies;
        moves = o.moves + 1;
        value = o.value;
        return *this;
    }
    constexpr bool operator==(const Tracked &) const = default;
    constexpr auto operator<=>(const Tracked &) const = default;
};

// SFINAE helpers
template <typename Opt, typename U, typename F>
concept has_map_or = requires { std::declval<Opt>().map_or(std::declval<U>(), std::declval<F>()); };

template <typename Opt, typename D, typename F>
concept has_map_or_else = requires { std::declval<Opt>().map_or_else(std::declval<D>(), std::declval<F>()); };

template <typename Opt, typename U>
concept has_unwrap_or = requires { std::declval<Opt>().unwrap_or(std::declval<U>()); };

template <typename Opt, typename F>
concept has_unwrap_or_else = requires { std::declval<Opt>().unwrap_or_else(std::declval<F>()); };

// ============================================================================
// 1. Mapping operations
// ============================================================================

namespace ext::map_ {

    void run_test() {
        // option<T>: some -> maps value
        {
            opt::option<int> o{42};
            auto result = o.map([](int x) { return x * 2; });
            static_assert(std::same_as<decltype(result), opt::option<int>>);
            assert(result.has_value());
            assert(*result == 84);
        }
        // option<T>: none -> returns none
        {
            opt::option<int> o{};
            auto result = o.map([](int x) { return x * 2; });
            assert(!result.has_value());
        }
        // option<T>: map to different type
        {
            opt::option<int> o{42};
            auto result = o.map([](int x) { return std::to_string(x); });
            static_assert(std::same_as<decltype(result), opt::option<std::string>>);
            assert(result.has_value());
            assert(*result == "42");
        }
        // option<T>: map to void
        {
            opt::option<int> o{42};
            int captured = 0;
            auto result = o.map([&](int x) { captured = x; });
            static_assert(std::same_as<decltype(result), opt::option<void>>);
            assert(result.has_value());
            assert(captured == 42);
        }
        // option<T>: rvalue option
        {
            auto result = opt::option<MoveOnly>{MoveOnly{10}}.map([](MoveOnly m) { return m.value + 1; });
            assert(result.has_value());
            assert(*result == 11);
        }
        // option<void>: some -> invokes
        {
            opt::option<void> o{true};
            int called = 0;
            auto result = o.map([&] { called++; return 99; });
            static_assert(std::same_as<decltype(result), opt::option<int>>);
            assert(result.has_value());
            assert(*result == 99);
            assert(called == 1);
        }
        // option<void>: none -> returns none
        {
            opt::option<void> o{};
            auto result = o.map([] { return 99; });
            assert(!result.has_value());
        }
        // option<T&>: maps reference
        {
            int x = 5;
            opt::option<int &> o{x};
            auto result = o.map([](int &v) { return v + 10; });
            assert(result.has_value());
            assert(*result == 15);
        }
    }

} // namespace ext::map_

namespace ext::map_or_ {

    void run_test() {
        // option<T>: some -> uses f
        {
            opt::option<int> o{42};
            auto result = o.map_or(0, [](int x) { return x * 2; });
            assert(result == 84);
        }
        // option<T>: none -> uses default
        {
            opt::option<int> o{};
            auto result = o.map_or(99, [](int x) { return x * 2; });
            assert(result == 99);
        }
        // option<void>: some -> uses f
        {
            opt::option<void> o{true};
            auto result = o.map_or(0, [] { return 42; });
            assert(result == 42);
        }
        // option<void>: none -> uses default
        {
            opt::option<void> o{};
            auto result = o.map_or(99, [] { return 42; });
            assert(result == 99);
        }
        // option<T&>: some -> uses f
        {
            int x = 10;
            opt::option<int &> o{x};
            auto result = o.map_or(0, [](int &v) { return v + 5; });
            assert(result == 15);
        }
        // option<T&>: none -> uses default
        {
            opt::option<int &> o{};
            auto result = o.map_or(99, [](int &v) { return v + 5; });
            assert(result == 99);
        }
        // reference-preserving: both branches return lvalue ref -> returns ref
        {
            int x = 10;
            int fallback = 20;
            opt::option<int> o{x};
            auto &result = o.map_or(fallback, [&](int &) -> int & { return x; });
            static_assert(std::is_lvalue_reference_v<decltype(o.map_or(fallback, [&](int &) -> int & { return x; }))>);
            assert(&result == &x);
        }
        // reference-preserving: none -> returns default ref
        {
            int fallback = 20;
            opt::option<int> o{};
            auto &result = o.map_or(fallback, [&](int &v) -> int & { return v; });
            assert(&result == &fallback);
        }
    }

} // namespace ext::map_or_

namespace ext::map_or_default_ {

    void run_test() {
        // option<T>: some -> maps
        {
            opt::option<int> o{42};
            auto result = o.map_or_default([](int x) { return x * 2; });
            assert(result == 84);
        }
        // option<T>: none -> default constructed
        {
            opt::option<int> o{};
            auto result = o.map_or_default([](int x) { return std::to_string(x); });
            assert(result.empty());
        }
        // option<T>: none -> int default is 0
        {
            opt::option<int> o{};
            auto result = o.map_or_default([](int x) { return x + 1; });
            assert(result == 0);
        }
    }

} // namespace ext::map_or_default_

namespace ext::map_or_else_ {

    void run_test() {
        // option<T>: some -> uses f
        {
            opt::option<int> o{42};
            auto result = o.map_or_else([] { return 0; }, [](int x) { return x * 2; });
            assert(result == 84);
        }
        // option<T>: none -> uses default_f
        {
            opt::option<int> o{};
            auto result = o.map_or_else([] { return 99; }, [](int x) { return x * 2; });
            assert(result == 99);
        }
        // option<void>: some -> uses f
        {
            opt::option<void> o{true};
            auto result = o.map_or_else([] { return 0; }, [] { return 42; });
            assert(result == 42);
        }
        // option<void>: none -> uses default_f
        {
            opt::option<void> o{};
            auto result = o.map_or_else([] { return 99; }, [] { return 42; });
            assert(result == 99);
        }
        // option<T&>: some -> uses f
        {
            int x = 10;
            opt::option<int &> o{x};
            auto result = o.map_or_else([] { return 0; }, [](int &v) { return v + 5; });
            assert(result == 15);
        }
        // reference-preserving: both return lvalue ref
        {
            int x = 10;
            int fallback = 20;
            opt::option<int> o{x};
            auto &result = o.map_or_else([&]() -> int & { return fallback; }, [&](int &) -> int & { return x; });
            assert(&result == &x);
        }
        // reference-preserving: none branch
        {
            int fallback = 20;
            opt::option<int> o{};
            auto &result = o.map_or_else([&]() -> int & { return fallback; }, [&](int &v) -> int & { return v; });
            assert(&result == &fallback);
        }
    }

} // namespace ext::map_or_else_

// ============================================================================
// 2. Inspect
// ============================================================================

namespace ext::inspect_ {

    void run_test() {
        // option<T>: some -> side effect observed
        {
            opt::option<int> o{42};
            int captured = 0;
            auto &&result = o.inspect([&](const int &v) { captured = v; });
            assert(captured == 42);
            assert(&result == &o);
        }
        // option<T>: none -> no side effect
        {
            opt::option<int> o{};
            bool called = false;
            o.inspect([&](const int &) { called = true; });
            assert(!called);
        }
        // option<T>: chaining
        {
            opt::option<int> o{10};
            int sum = 0;
            auto &&result = o.inspect([&](const int &v) { sum += v; }).inspect([&](const int &v) { sum += v; });
            assert(sum == 20);
            (void)result;
        }
        // option<void>: some -> side effect
        {
            opt::option<void> o{true};
            bool called = false;
            o.inspect([&] { called = true; });
            assert(called);
        }
        // option<void>: none -> no side effect
        {
            opt::option<void> o{};
            bool called = false;
            o.inspect([&] { called = true; });
            assert(!called);
        }
        // option<T&>: some -> side effect
        {
            int x = 5;
            opt::option<int &> o{x};
            int captured = 0;
            o.inspect([&](int &v) { captured = v; });
            assert(captured == 5);
        }
        // option<T&>: none -> no side effect
        {
            opt::option<int &> o{};
            bool called = false;
            o.inspect([&](int &) { called = true; });
            assert(!called);
        }
    }

} // namespace ext::inspect_

// ============================================================================
// 3. Filter, Flatten, Transpose
// ============================================================================

namespace ext::filter_ {

    void run_test() {
        // option<T>: some, predicate true -> keeps
        {
            opt::option<int> o{42};
            auto result = o.filter([](const int &v) { return v > 10; });
            assert(result.has_value());
            assert(*result == 42);
        }
        // option<T>: some, predicate false -> none
        {
            opt::option<int> o{5};
            auto result = o.filter([](const int &v) { return v > 10; });
            assert(!result.has_value());
        }
        // option<T>: none -> none
        {
            opt::option<int> o{};
            auto result = o.filter([](const int &) { return true; });
            assert(!result.has_value());
        }
        // option<void>: some, predicate true
        {
            opt::option<void> o{true};
            auto result = o.filter([] { return true; });
            assert(result.has_value());
        }
        // option<void>: some, predicate false
        {
            opt::option<void> o{true};
            auto result = o.filter([] { return false; });
            assert(!result.has_value());
        }
        // option<T&>: some, predicate true -> keeps reference
        {
            int x = 42;
            opt::option<int &> o{x};
            auto result = o.filter([](int &v) { return v > 10; });
            assert(result.has_value());
            assert(&*result == &x);
        }
        // option<T&>: some, predicate false
        {
            int x = 5;
            opt::option<int &> o{x};
            auto result = o.filter([](int &v) { return v > 10; });
            assert(!result.has_value());
        }
    }

} // namespace ext::filter_

namespace ext::flatten_ {

    void run_test() {
        // some(some(v)) -> some(v)
        {
            opt::option<opt::option<int>> o{opt::option<int>{42}};
            auto result = o.flatten();
            static_assert(std::same_as<decltype(result), opt::option<int>>);
            assert(result.has_value());
            assert(*result == 42);
        }
        // some(none) -> none
        {
            opt::option<opt::option<int>> o{opt::option<int>{}};
            auto result = o.flatten();
            assert(!result.has_value());
        }
        // none -> none
        {
            opt::option<opt::option<int>> o{};
            auto result = o.flatten();
            assert(!result.has_value());
        }
    }

} // namespace ext::flatten_

namespace ext::transpose_ {

    void run_test() {
        // some(expected value) -> expected(some(value))
        {
            opt::option<std::expected<int, std::string>> o{std::expected<int, std::string>{42}};
            auto result = o.transpose();
            static_assert(
                std::same_as<decltype(result), std::expected<opt::option<int>, std::string>>);
            assert(result.has_value());
            assert(result->has_value());
            assert(**result == 42);
        }
        // some(expected error) -> expected(error)
        {
            opt::option<std::expected<int, std::string>> o{
                std::expected<int, std::string>{std::unexpect, "err"}};
            auto result = o.transpose();
            assert(!result.has_value());
            assert(result.error() == "err");
        }
        // none -> expected(none option)
        {
            opt::option<std::expected<int, std::string>> o{};
            auto result = o.transpose();
            assert(result.has_value());
            assert(!result->has_value());
        }
    }

} // namespace ext::transpose_

// ============================================================================
// 4. Unwrap variants
// ============================================================================

namespace ext::unwrap_or_ {

    void run_test() {
        // option<T>: some -> returns value
        {
            opt::option<int> o{42};
            assert(o.unwrap_or(0) == 42);
        }
        // option<T>: none -> returns default
        {
            opt::option<int> o{};
            assert(o.unwrap_or(99) == 99);
        }
        // option<T>: rvalue option
        {
            auto result = opt::option<MoveOnly>{MoveOnly{10}}.unwrap_or(MoveOnly{20});
            assert(result.value == 10);
        }
        // option<T>: rvalue none
        {
            auto result = opt::option<MoveOnly>{}.unwrap_or(MoveOnly{20});
            assert(result.value == 20);
        }
        // reference-preserving: both lvalue ref -> returns ref
        {
            int x = 10;
            int fallback = 20;
            opt::option<int> o{x};
            auto &result = o.unwrap_or(fallback);
            static_assert(std::is_lvalue_reference_v<decltype(o.unwrap_or(fallback))>);
            assert(&result == &*o);
        }
        // reference-preserving: none -> returns default ref
        {
            int fallback = 20;
            opt::option<int> o{};
            auto &result = o.unwrap_or(fallback);
            assert(&result == &fallback);
        }
        // option<T&>: some -> returns ref
        {
            int x = 42;
            int fallback = 0;
            opt::option<int &> o{x};
            assert(&o.unwrap_or(fallback) == &x);
        }
        // option<T&>: none -> returns fallback
        {
            int fallback = 0;
            opt::option<int &> o{};
            assert(&o.unwrap_or(fallback) == &fallback);
        }
    }

} // namespace ext::unwrap_or_

namespace ext::unwrap_or_default_ {

    void run_test() {
        // option<T>: some -> returns value
        {
            opt::option<int> o{42};
            assert(o.unwrap_or_default() == 42);
        }
        // option<T>: none -> returns T{}
        {
            opt::option<int> o{};
            assert(o.unwrap_or_default() == 0);
        }
        // option<T>: string
        {
            opt::option<std::string> o{};
            assert(o.unwrap_or_default().empty());
        }
        // option<void>: always void (no-op)
        {
            opt::option<void> o{};
            o.unwrap_or_default(); // should compile and be no-op
        }
    }

} // namespace ext::unwrap_or_default_

namespace ext::unwrap_or_else_ {

    void run_test() {
        // option<T>: some -> returns value, f not called
        {
            opt::option<int> o{42};
            bool called = false;
            auto result = o.unwrap_or_else([&] {
                called = true;
                return 0;
            });
            assert(result == 42);
            assert(!called);
        }
        // option<T>: none -> calls f
        {
            opt::option<int> o{};
            auto result = o.unwrap_or_else([] { return 99; });
            assert(result == 99);
        }
        // option<void>: none -> calls f
        {
            opt::option<void> o{};
            bool called = false;
            o.unwrap_or_else([&] { called = true; });
            assert(called);
        }
        // option<void>: some -> f not called
        {
            opt::option<void> o{true};
            bool called = false;
            o.unwrap_or_else([&] { called = true; });
            assert(!called);
        }
        // option<T&>: some -> returns value
        {
            int x = 42;
            opt::option<int &> o{x};
            auto result = o.unwrap_or_else([] { return 0; });
            assert(result == 42);
        }
        // option<T&>: none -> calls f
        {
            opt::option<int &> o{};
            auto result = o.unwrap_or_else([] { return 99; });
            assert(result == 99);
        }
        // reference-preserving: both return lvalue ref
        {
            int x = 10;
            int fallback = 20;
            opt::option<int> o{x};
            auto &result = o.unwrap_or_else([&]() -> int & { return fallback; });
            static_assert(std::is_lvalue_reference_v<decltype(
                o.unwrap_or_else([&]() -> int & { return fallback; }))>);
            assert(&result == &*o);
        }
    }

} // namespace ext::unwrap_or_else_

// ============================================================================
// 5. ok_or / ok_or_else
// ============================================================================

namespace ext::ok_or_ {

    void run_test() {
        // option<T>: some -> expected with value
        {
            opt::option<int> o{42};
            auto result = o.ok_or(std::string{"error"});
            static_assert(std::same_as<decltype(result), std::expected<int, std::string>>);
            assert(result.has_value());
            assert(*result == 42);
        }
        // option<T>: none -> expected with error
        {
            opt::option<int> o{};
            auto result = o.ok_or(std::string{"error"});
            assert(!result.has_value());
            assert(result.error() == "error");
        }
        // option<void>: some -> expected<void, E>
        {
            opt::option<void> o{true};
            auto result = o.ok_or(42);
            static_assert(std::same_as<decltype(result), std::expected<void, int>>);
            assert(result.has_value());
        }
        // option<void>: none
        {
            opt::option<void> o{};
            auto result = o.ok_or(42);
            assert(!result.has_value());
            assert(result.error() == 42);
        }
        // option<T&>: ok_or produces expected<T&, E>, but std::expected<T&, E>
        // is not supported on all implementations (MSVC), so skip this test.
    }

} // namespace ext::ok_or_

namespace ext::ok_or_else_ {

    void run_test() {
        // option<T>: some -> f not called
        {
            opt::option<int> o{42};
            bool called = false;
            auto result = o.ok_or_else([&] {
                called = true;
                return std::string{"error"};
            });
            assert(result.has_value());
            assert(*result == 42);
            assert(!called);
        }
        // option<T>: none -> f called
        {
            opt::option<int> o{};
            auto result = o.ok_or_else([] { return std::string{"error"}; });
            assert(!result.has_value());
            assert(result.error() == "error");
        }
        // option<void>: some
        {
            opt::option<void> o{true};
            auto result = o.ok_or_else([] { return 42; });
            assert(result.has_value());
        }
        // option<void>: none
        {
            opt::option<void> o{};
            auto result = o.ok_or_else([] { return 42; });
            assert(!result.has_value());
            assert(result.error() == 42);
        }
        // option<T&>: ok_or_else produces expected<T&, E>, but std::expected<T&, E>
        // is not supported on all implementations (MSVC), so skip this test.
    }

} // namespace ext::ok_or_else_

// ============================================================================
// 6. Zip / Unzip
// ============================================================================

namespace ext::zip_ {

    void run_test() {
        // option<T>: both some -> pair
        {
            opt::option<int> a{1};
            opt::option<std::string> b{std::string{"hi"}};
            auto result = a.zip(b);
            assert(result.has_value());
            assert(result->first == 1);
            assert(result->second == "hi");
        }
        // option<T>: first none
        {
            opt::option<int> a{};
            opt::option<std::string> b{std::string{"hi"}};
            auto result = a.zip(b);
            assert(!result.has_value());
        }
        // option<T>: second none
        {
            opt::option<int> a{1};
            opt::option<std::string> b{};
            auto result = a.zip(b);
            assert(!result.has_value());
        }
        // option<T>: both none
        {
            opt::option<int> a{};
            opt::option<std::string> b{};
            auto result = a.zip(b);
            assert(!result.has_value());
        }
        // option<T&>: both some
        {
            int x = 10;
            opt::option<int &> a{x};
            opt::option<int> b{20};
            auto result = a.zip(b);
            assert(result.has_value());
            assert(&result->first == &x);
            assert(result->second == 20);
        }
    }

} // namespace ext::zip_

namespace ext::zip_with_ {

    void run_test() {
        // option<T>: both some -> custom combiner
        {
            opt::option<int> a{3};
            opt::option<int> b{4};
            auto result = a.zip_with(b, [](int x, int y) { return x + y; });
            assert(result.has_value());
            assert(*result == 7);
        }
        // option<T>: one none
        {
            opt::option<int> a{3};
            opt::option<int> b{};
            auto result = a.zip_with(b, [](int x, int y) { return x + y; });
            assert(!result.has_value());
        }
        // option<T&>: both some
        {
            int x = 10;
            opt::option<int &> a{x};
            opt::option<int> b{20};
            auto result = a.zip_with(b, [](int &a_val, int &b_val) { return a_val + b_val; });
            assert(result.has_value());
            assert(*result == 30);
        }
    }

} // namespace ext::zip_with_

namespace ext::unzip_ {

    void run_test() {
        // some pair -> pair of options
        {
            opt::option<std::pair<int, std::string>> o{std::pair{42, std::string{"hi"}}};
            auto [a, b] = o.unzip();
            assert(a.has_value());
            assert(*a == 42);
            assert(b.has_value());
            assert(*b == "hi");
        }
        // none -> pair of nones
        {
            opt::option<std::pair<int, std::string>> o{};
            auto [a, b] = o.unzip();
            assert(!a.has_value());
            assert(!b.has_value());
        }
    }

} // namespace ext::unzip_

// ============================================================================
// 7. Insertion / Replacement
// ============================================================================

namespace ext::get_or_insert_ {

    void run_test() {
        // option<T>: empty -> inserts
        {
            opt::option<int> o{};
            auto &ref = o.get_or_insert(42);
            assert(o.has_value());
            assert(ref == 42);
            assert(&ref == &*o);
        }
        // option<T>: non-empty -> returns existing
        {
            opt::option<int> o{10};
            auto &ref = o.get_or_insert(42);
            assert(ref == 10);
        }
        // option<T&>: empty -> inserts reference
        {
            int x = 42;
            opt::option<int &> o{};
            auto &ref = o.get_or_insert(x);
            assert(o.has_value());
            assert(&ref == &x);
        }
    }

} // namespace ext::get_or_insert_

namespace ext::get_or_insert_default_ {

    void run_test() {
        // option<T>: empty -> inserts T{}
        {
            opt::option<int> o{};
            auto &ref = o.get_or_insert_default();
            assert(o.has_value());
            assert(ref == 0);
        }
        // option<T>: non-empty -> returns existing
        {
            opt::option<int> o{42};
            auto &ref = o.get_or_insert_default();
            assert(ref == 42);
        }
    }

} // namespace ext::get_or_insert_default_

namespace ext::get_or_insert_with_ {

    void run_test() {
        // option<T>: empty -> calls factory
        {
            opt::option<int> o{};
            auto &ref = o.get_or_insert_with([] { return 42; });
            assert(o.has_value());
            assert(ref == 42);
        }
        // option<T>: non-empty -> factory not called
        {
            opt::option<int> o{10};
            bool called = false;
            auto &ref = o.get_or_insert_with([&] {
                called = true;
                return 42;
            });
            assert(ref == 10);
            assert(!called);
        }
    }

} // namespace ext::get_or_insert_with_

namespace ext::insert_ {

    void run_test() {
        // option<T>: empty -> inserts
        {
            opt::option<int> o{};
            auto &&ref = o.insert(42);
            assert(o.has_value());
            assert(ref == 42);
        }
        // option<T>: non-empty -> replaces
        {
            opt::option<int> o{10};
            auto &&ref = o.insert(42);
            assert(ref == 42);
        }
        // option<T&>: inserts reference
        {
            int x = 42;
            opt::option<int &> o{};
            auto &ref = o.insert(x);
            assert(&ref == &x);
        }
    }

} // namespace ext::insert_

namespace ext::replace_ {

    void run_test() {
        // option<T>: some -> returns old, inserts new
        {
            opt::option<int> o{10};
            auto old = o.replace(42);
            assert(o.has_value());
            assert(*o == 42);
            assert(old.has_value());
            assert(*old == 10);
        }
        // option<T>: none -> returns none, inserts new
        {
            opt::option<int> o{};
            auto old = o.replace(42);
            assert(o.has_value());
            assert(*o == 42);
            assert(!old.has_value());
        }
        // option<T&>: some -> returns old ref, inserts new ref
        {
            int x = 10;
            int y = 20;
            opt::option<int &> o{x};
            auto old = o.replace(y);
            assert(o.has_value());
            assert(&*o == &y);
            assert(old.has_value());
            assert(&*old == &x);
        }
    }

} // namespace ext::replace_

namespace ext::take_ {

    void run_test() {
        // option<T>: some -> extracts, leaves none
        {
            opt::option<int> o{42};
            auto taken = o.take();
            assert(!o.has_value());
            assert(taken.has_value());
            assert(*taken == 42);
        }
        // option<T>: none -> stays none
        {
            opt::option<int> o{};
            auto taken = o.take();
            assert(!o.has_value());
            assert(!taken.has_value());
        }
        // option<void>: some -> extracts
        {
            opt::option<void> o{true};
            auto taken = o.take();
            assert(!o.has_value());
            assert(taken.has_value());
        }
        // option<T&>: some -> extracts
        {
            int x = 42;
            opt::option<int &> o{x};
            auto taken = o.take();
            assert(!o.has_value());
            assert(taken.has_value());
            assert(&*taken == &x);
        }
    }

} // namespace ext::take_

namespace ext::take_if_ {

    void run_test() {
        // option<T>: some, predicate true -> takes
        {
            opt::option<int> o{42};
            auto taken = o.take_if([](const int &v) { return v > 10; });
            assert(!o.has_value());
            assert(taken.has_value());
            assert(*taken == 42);
        }
        // option<T>: some, predicate false -> doesn't take
        {
            opt::option<int> o{5};
            auto taken = o.take_if([](const int &v) { return v > 10; });
            assert(o.has_value());
            assert(*o == 5);
            assert(!taken.has_value());
        }
        // option<T>: none -> stays none
        {
            opt::option<int> o{};
            auto taken = o.take_if([](const int &) { return true; });
            assert(!o.has_value());
            assert(!taken.has_value());
        }
        // option<void>: some, predicate true
        {
            opt::option<void> o{true};
            auto taken = o.take_if([] { return true; });
            assert(!o.has_value());
            assert(taken.has_value());
        }
        // option<void>: some, predicate false
        {
            opt::option<void> o{true};
            auto taken = o.take_if([] { return false; });
            assert(o.has_value());
            assert(!taken.has_value());
        }
        // option<T&>: some, predicate true
        {
            int x = 42;
            opt::option<int &> o{x};
            auto taken = o.take_if([](int &v) { return v > 10; });
            assert(!o.has_value());
            assert(taken.has_value());
            assert(&*taken == &x);
        }
    }

} // namespace ext::take_if_

// ============================================================================
// 8. Reference conversions
// ============================================================================

namespace ext::as_ref_ {

    void run_test() {
        // option<T>: some -> option<const T&>
        {
            opt::option<int> o{42};
            auto ref = o.as_ref();
            static_assert(std::same_as<decltype(ref), opt::option<const int &>>);
            assert(ref.has_value());
            assert(*ref == 42);
            assert(&*ref == &*o);
        }
        // option<T>: none -> none
        {
            opt::option<int> o{};
            auto ref = o.as_ref();
            assert(!ref.has_value());
        }
        // option<T&>: some -> option<const T&>
        {
            int x = 42;
            opt::option<int &> o{x};
            auto ref = o.as_ref();
            static_assert(std::same_as<decltype(ref), opt::option<const int &>>);
            assert(ref.has_value());
            assert(&*ref == &x);
        }
    }

} // namespace ext::as_ref_

namespace ext::as_mut_ {

    void run_test() {
        // option<T>: some -> option<T&>
        {
            opt::option<int> o{42};
            auto ref = o.as_mut();
            static_assert(std::same_as<decltype(ref), opt::option<int &>>);
            assert(ref.has_value());
            *ref = 100;
            assert(*o == 100);
        }
        // option<T>: none -> none
        {
            opt::option<int> o{};
            auto ref = o.as_mut();
            assert(!ref.has_value());
        }
        // option<T&>: some -> option<T&> (same)
        {
            int x = 42;
            opt::option<int &> o{x};
            auto ref = o.as_mut();
            static_assert(std::same_as<decltype(ref), opt::option<int &>>);
            assert(ref.has_value());
            *ref = 100;
            assert(x == 100);
        }
    }

} // namespace ext::as_mut_

namespace ext::as_deref_ {

    void run_test() {
        // option<T> where T is dereferenceable (e.g. unique_ptr)
        {
            opt::option<std::unique_ptr<int>> o{std::make_unique<int>(42)};
            auto ref = o.as_deref();
            assert(ref.has_value());
            assert(*ref == 42);
        }
        // option<T> where T is dereferenceable: none
        {
            opt::option<std::unique_ptr<int>> o{};
            auto ref = o.as_deref();
            assert(!ref.has_value());
        }
        // option<T&> where T is pointer
        {
            int val = 42;
            int *ptr = &val;
            opt::option<int *&> o{ptr};
            auto ref = o.as_deref();
            assert(ref.has_value());
            assert(*ref == 42);
        }
    }

} // namespace ext::as_deref_

namespace ext::as_deref_mut_ {

    void run_test() {
        // option<T> where T is dereferenceable
        {
            opt::option<std::unique_ptr<int>> o{std::make_unique<int>(42)};
            auto ref = o.as_deref_mut();
            assert(ref.has_value());
            *ref = 100;
            assert(**o == 100);
        }
        // option<T&> where T is pointer
        {
            int val = 42;
            int *ptr = &val;
            opt::option<int *&> o{ptr};
            auto ref = o.as_deref_mut();
            assert(ref.has_value());
            *ref = 100;
            assert(val == 100);
        }
    }

} // namespace ext::as_deref_mut_

// ============================================================================
// 9. Clone / Copy
// ============================================================================

namespace ext::clone_ {

    void run_test() {
        // option<T>: some -> clones
        {
            opt::option<int> o{42};
            auto c = o.clone();
            assert(c.has_value());
            assert(*c == 42);
        }
        // option<T>: none -> none
        {
            opt::option<int> o{};
            auto c = o.clone();
            assert(!c.has_value());
        }
        // option<void>: some
        {
            opt::option<void> o{true};
            auto c = o.clone();
            assert(c.has_value());
        }
        // option<void>: none
        {
            opt::option<void> o{};
            auto c = o.clone();
            assert(!c.has_value());
        }
    }

} // namespace ext::clone_

namespace ext::clone_from_ {

    void run_test() {
        // option<T>: clone from some to none
        {
            opt::option<int> dest{};
            opt::option<int> src{42};
            dest.clone_from(src);
            assert(dest.has_value());
            assert(*dest == 42);
        }
        // option<T>: clone from some to some
        {
            opt::option<int> dest{10};
            opt::option<int> src{42};
            dest.clone_from(src);
            assert(*dest == 42);
        }
        // option<T>: clone from none to some
        {
            opt::option<int> dest{42};
            opt::option<int> src{};
            dest.clone_from(src);
            assert(!dest.has_value());
        }
        // option<void>: clone from
        {
            opt::option<void> dest{};
            opt::option<void> src{true};
            dest.clone_from(src);
            assert(dest.has_value());
        }
    }

} // namespace ext::clone_from_

namespace ext::cloned_ {

    void run_test() {
        // option<T&>: some -> option<T> with cloned value
        {
            int x = 42;
            opt::option<int &> o{x};
            auto c = o.cloned();
            static_assert(std::same_as<decltype(c), opt::option<int>>);
            assert(c.has_value());
            assert(*c == 42);
            assert(&*c != &x);
        }
        // option<T&>: none -> none
        {
            opt::option<int &> o{};
            auto c = o.cloned();
            assert(!c.has_value());
        }
    }

} // namespace ext::cloned_

namespace ext::copied_ {

    void run_test() {
        // option<T&>: some -> option<T> with copied value
        {
            int x = 42;
            opt::option<int &> o{x};
            auto c = o.copied();
            static_assert(std::same_as<decltype(c), opt::option<int>>);
            assert(c.has_value());
            assert(*c == 42);
        }
        // option<T&>: none
        {
            opt::option<int &> o{};
            auto c = o.copied();
            assert(!c.has_value());
        }
        // option<const T&>: some
        {
            const int x = 42;
            opt::option<const int &> o{x};
            auto c = o.copied();
            static_assert(std::same_as<decltype(c), opt::option<int>>);
            assert(c.has_value());
            assert(*c == 42);
        }
    }

} // namespace ext::copied_

// ============================================================================
// 10. Comparison utilities
// ============================================================================

namespace ext::cmp_ {

    void run_test() {
        // option<T>: both some, equal
        {
            opt::option<int> a{42};
            opt::option<int> b{42};
            assert(a.cmp(b) == std::strong_ordering::equal);
        }
        // option<T>: both some, less
        {
            opt::option<int> a{10};
            opt::option<int> b{42};
            assert(a.cmp(b) == std::strong_ordering::less);
        }
        // option<T>: both some, greater
        {
            opt::option<int> a{42};
            opt::option<int> b{10};
            assert(a.cmp(b) == std::strong_ordering::greater);
        }
        // option<T>: some vs none
        {
            opt::option<int> a{42};
            opt::option<int> b{};
            assert(a.cmp(b) == std::strong_ordering::greater);
        }
        // option<T>: none vs some
        {
            opt::option<int> a{};
            opt::option<int> b{42};
            assert(a.cmp(b) == std::strong_ordering::less);
        }
        // option<T>: both none
        {
            opt::option<int> a{};
            opt::option<int> b{};
            assert(a.cmp(b) == std::strong_ordering::equal);
        }
        // option<void>
        {
            opt::option<void> a{true};
            opt::option<void> b{true};
            assert(a.cmp(b) == std::strong_ordering::equal);

            opt::option<void> c{};
            assert(a.cmp(c) == std::strong_ordering::greater);
            assert(c.cmp(a) == std::strong_ordering::less);
        }
    }

} // namespace ext::cmp_

namespace ext::max_ {

    void run_test() {
        // option<T>: both some -> returns greater
        {
            opt::option<int> a{10};
            opt::option<int> b{42};
            auto result = (a.max)(b);
            assert(result.has_value());
            assert(*result == 42);
        }
        // option<T>: one none -> returns the some
        {
            opt::option<int> a{10};
            opt::option<int> b{};
            auto result = (a.max)(b);
            assert(result.has_value());
            assert(*result == 10);
        }
        {
            opt::option<int> a{};
            opt::option<int> b{42};
            auto result = (a.max)(b);
            assert(result.has_value());
            assert(*result == 42);
        }
        // option<T>: both none -> none
        {
            opt::option<int> a{};
            opt::option<int> b{};
            auto result = (a.max)(b);
            assert(!result.has_value());
        }
        // option<void>: both some
        {
            opt::option<void> a{true};
            opt::option<void> b{true};
            auto result = (a.max)(b);
            assert(result.has_value());
        }
    }

} // namespace ext::max_

namespace ext::min_ {

    void run_test() {
        // option<T>: both some -> returns lesser
        {
            opt::option<int> a{10};
            opt::option<int> b{42};
            auto result = (a.min)(b);
            assert(result.has_value());
            assert(*result == 10);
        }
        // option<T>: one none -> returns the none
        {
            opt::option<int> a{10};
            opt::option<int> b{};
            auto result = (a.min)(b);
            assert(!result.has_value());
        }
        {
            opt::option<int> a{};
            opt::option<int> b{42};
            auto result = (a.min)(b);
            assert(!result.has_value());
        }
        // option<T>: both none -> none
        {
            opt::option<int> a{};
            opt::option<int> b{};
            auto result = (a.min)(b);
            assert(!result.has_value());
        }
    }

} // namespace ext::min_

namespace ext::clamp_ {

    void run_test() {
        // option<T>: value within bounds
        {
            opt::option<int> o{5};
            opt::option<int> lo{1};
            opt::option<int> hi{10};
            auto result = o.clamp(lo, hi);
            assert(result.has_value());
            assert(*result == 5);
        }
        // option<T>: value below min
        {
            opt::option<int> o{0};
            opt::option<int> lo{1};
            opt::option<int> hi{10};
            auto result = o.clamp(lo, hi);
            assert(result.has_value());
            assert(*result == 1);
        }
        // option<T>: value above max
        {
            opt::option<int> o{20};
            opt::option<int> lo{1};
            opt::option<int> hi{10};
            auto result = o.clamp(lo, hi);
            assert(result.has_value());
            assert(*result == 10);
        }
        // option<T>: none -> none
        {
            opt::option<int> o{};
            opt::option<int> lo{1};
            opt::option<int> hi{10};
            auto result = o.clamp(lo, hi);
            assert(!result.has_value());
        }
    }

} // namespace ext::clamp_

// ============================================================================
// 11. Misc: is_some/is_none/is_some_and/is_none_or, as_span, default_, from
// ============================================================================

namespace ext::predicates_ {

    void run_test() {
        // is_some / is_none
        {
            opt::option<int> some{42};
            opt::option<int> none{};
            assert(some.is_some());
            assert(!some.is_none());
            assert(!none.is_some());
            assert(none.is_none());
        }
        // is_some_and
        {
            opt::option<int> o{42};
            assert(o.is_some_and([](const int &v) { return v > 10; }));
            assert(!o.is_some_and([](const int &v) { return v > 100; }));

            opt::option<int> none{};
            assert(!none.is_some_and([](const int &) { return true; }));
        }
        // is_none_or
        {
            opt::option<int> o{42};
            assert(o.is_none_or([](const int &v) { return v > 10; }));
            assert(!o.is_none_or([](const int &v) { return v > 100; }));

            opt::option<int> none{};
            assert(none.is_none_or([](const int &) { return false; }));
        }
        // option<void>
        {
            opt::option<void> some{true};
            opt::option<void> none{};
            assert(some.is_some());
            assert(none.is_none());
            assert(some.is_some_and([] { return true; }));
            assert(!some.is_some_and([] { return false; }));
            assert(none.is_none_or([] { return false; }));
        }
        // option<T&>
        {
            int x = 42;
            opt::option<int &> some{x};
            opt::option<int &> none{};
            assert(some.is_some());
            assert(none.is_none());
            assert(some.is_some_and([](int &v) { return v == 42; }));
            assert(none.is_none_or([](int &) { return false; }));
        }
    }

} // namespace ext::predicates_

namespace ext::as_span_ {

    void run_test() {
        // option<T>: some -> span of 1
        {
            opt::option<int> o{42};
            auto s = o.as_span();
            static_assert(std::same_as<decltype(s), std::span<int>>);
            assert(s.size() == 1);
            assert(s[0] == 42);
        }
        // option<T>: none -> empty span
        {
            opt::option<int> o{};
            auto s = o.as_span();
            assert(s.empty());
        }
    }

} // namespace ext::as_span_

namespace ext::default__ {

    void run_test() {
        // option<T>::default_() -> empty option
        {
            auto o = opt::option<int>::default_();
            assert(!o.has_value());
        }
        // option<void>::default_()
        {
            auto o = opt::option<void>::default_();
            assert(!o.has_value());
        }
    }

} // namespace ext::default__

namespace ext::from_ {

    void run_test() {
        // option<T>::from(value)
        {
            auto o = opt::option<int>::from(42);
            assert(o.has_value());
            assert(*o == 42);
        }
        // option<T>::from(lvalue)
        {
            int x = 42;
            auto o = opt::option<int>::from(x);
            assert(o.has_value());
            assert(*o == 42);
        }
        // option<void>::from()
        {
            auto o = opt::option<void>::from();
            assert(o.has_value());
        }
        // option<T&>::from(option<U>&)
        {
            opt::option<int> src{42};
            auto o = opt::option<int &>::from(src);
            assert(o.has_value());
            assert(&*o == &*src);
        }
        // option<T&>::from(const option<U>&)
        {
            const opt::option<int> src{42};
            auto o = opt::option<const int &>::from(src);
            assert(o.has_value());
            assert(&*o == &*src);
        }
        // option<T&>::from(empty)
        {
            opt::option<int> src{};
            auto o = opt::option<int &>::from(src);
            assert(!o.has_value());
        }
    }

} // namespace ext::from_

// ============================================================================
// 12. Reference-preserving return type constraint tests
// ============================================================================

namespace ext::ref_preserving_constraints {

    // Helper lambdas for testing
    inline int global_val = 42;
    inline auto returns_ref = [](int &) -> int & { return global_val; };
    inline auto returns_val = [](int &) -> int { return 0; };

    // map_or: f returns lvalue ref, U is lvalue ref -> OK (returns ref)
    static_assert(has_map_or<opt::option<int> &, int &, decltype(returns_ref)>);

    // map_or: f returns lvalue ref, U is rvalue -> rejected
    static_assert(!has_map_or<opt::option<int> &, int, decltype(returns_ref)>);

    // map_or: f returns value, U is lvalue ref -> OK (returns value)
    static_assert(has_map_or<opt::option<int> &, int &, decltype(returns_val)>);

    // map_or: f returns value, U is rvalue -> OK (returns value)
    static_assert(has_map_or<opt::option<int> &, int, decltype(returns_val)>);

    // unwrap_or: option lvalue, U is lvalue ref -> returns ref
    static_assert(has_unwrap_or<opt::option<int> &, int &>);

    // unwrap_or: option lvalue, U is rvalue -> returns value (OK, no extra constraint)
    static_assert(has_unwrap_or<opt::option<int> &, int>);

    // unwrap_or: option rvalue, U is rvalue -> returns value
    static_assert(has_unwrap_or<opt::option<int>, int>);

    // map_or_else: both return ref -> OK
    inline auto default_returns_ref = []() -> int & { return global_val; };
    inline auto default_returns_val = []() -> int { return 0; };

    static_assert(has_map_or_else<opt::option<int> &, decltype(default_returns_ref), decltype(returns_ref)>);

    // map_or_else: f returns ref, default returns value -> rejected
    static_assert(!has_map_or_else<opt::option<int> &, decltype(default_returns_val), decltype(returns_ref)>);

    // unwrap_or_else: f returns ref, *self is lvalue ref -> returns ref (OK)
    static_assert(has_unwrap_or_else<opt::option<int> &, decltype(default_returns_ref)>);

    // unwrap_or_else: f returns val -> returns val (always OK)
    static_assert(has_unwrap_or_else<opt::option<int> &, decltype(default_returns_val)>);

    // unwrap_or_else: f returns ref, *self is rvalue -> rejected (would silently copy ref)
    static_assert(!has_unwrap_or_else<opt::option<int>, decltype(default_returns_ref)>);

    void run_test() {
        // Runtime: map_or returns actual alias when both are refs
        {
            int x = 10;
            int fallback = 20;
            opt::option<int> o{x};
            auto &result = o.map_or(fallback, [](int &v) -> int & { return v; });
            assert(&result == &*o);

            // Modify through the reference
            result = 999;
            assert(*o == 999);
        }
        // Runtime: unwrap_or returns alias
        {
            int fallback = 20;
            opt::option<int> o{42};
            auto &result = o.unwrap_or(fallback);
            assert(&result == &*o);
        }
        // Runtime: unwrap_or none returns fallback alias
        {
            int fallback = 20;
            opt::option<int> o{};
            auto &result = o.unwrap_or(fallback);
            assert(&result == &fallback);
        }
        // Runtime: map_or_else both ref
        {
            int x = 10;
            int fallback = 20;
            opt::option<int> o{x};
            auto &result =
                o.map_or_else([&]() -> int & { return fallback; }, [](int &v) -> int & { return v; });
            assert(&result == &*o);
        }
        // Runtime: unwrap_or_else ref
        {
            int fallback = 20;
            opt::option<int> o{42};
            auto &result = o.unwrap_or_else([&]() -> int & { return fallback; });
            assert(&result == &*o);
        }
        {
            int fallback = 20;
            opt::option<int> o{};
            auto &result = o.unwrap_or_else([&]() -> int & { return fallback; });
            assert(&result == &fallback);
        }
    }

} // namespace ext::ref_preserving_constraints

// ============================================================================
// Main
// ============================================================================

int main() {
    ext::map_::run_test();
    ext::map_or_::run_test();
    ext::map_or_default_::run_test();
    ext::map_or_else_::run_test();

    ext::inspect_::run_test();

    ext::filter_::run_test();
    ext::flatten_::run_test();
    ext::transpose_::run_test();

    ext::unwrap_or_::run_test();
    ext::unwrap_or_default_::run_test();
    ext::unwrap_or_else_::run_test();

    ext::ok_or_::run_test();
    ext::ok_or_else_::run_test();

    ext::zip_::run_test();
    ext::zip_with_::run_test();
    ext::unzip_::run_test();

    ext::get_or_insert_::run_test();
    ext::get_or_insert_default_::run_test();
    ext::get_or_insert_with_::run_test();
    ext::insert_::run_test();
    ext::replace_::run_test();
    ext::take_::run_test();
    ext::take_if_::run_test();

    ext::as_ref_::run_test();
    ext::as_mut_::run_test();
    ext::as_deref_::run_test();
    ext::as_deref_mut_::run_test();

    ext::clone_::run_test();
    ext::clone_from_::run_test();
    ext::cloned_::run_test();
    ext::copied_::run_test();

    ext::cmp_::run_test();
    ext::max_::run_test();
    ext::min_::run_test();
    ext::clamp_::run_test();

    ext::predicates_::run_test();
    ext::as_span_::run_test();
    ext::default__::run_test();
    ext::from_::run_test();

    ext::ref_preserving_constraints::run_test();
}
