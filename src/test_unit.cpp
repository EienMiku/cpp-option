#include "include/option.hpp"
#include <format>
#include <gtest/gtest.h>
#include <string>
#include <unordered_set>

using namespace std::literals;
using namespace opt;

// =============================
// 1. In-place Construction (std::in_place)
// =============================
TEST(OptionBasic, InPlaceConstruction) {
    // Construct std::string with arguments
    option<std::string> o1(std::in_place, 5, 'x');
    EXPECT_TRUE(o1.is_some());
    EXPECT_EQ(o1.unwrap(), "xxxxx");

    // Construct std::vector with initializer_list
    option<std::vector<int>> o2(std::in_place, { 1, 2, 3, 4, 5 });
    EXPECT_TRUE(o2.is_some());
    EXPECT_EQ(o2->size(), 5u);
    EXPECT_EQ((*o2)[2], 3);

    // Construct std::map with initializer_list and extra argument
    option<std::map<int, std::string>> o3(std::in_place,
                                          std::initializer_list<std::pair<const int, std::string>>{
                                              { 1, "a" },
                                              { 2, "b" }
    },
                                          std::less<int>{});
    EXPECT_TRUE(o3.is_some());
    EXPECT_EQ(o3->size(), 2u);
    EXPECT_EQ(o3->at(1), "a");
    EXPECT_EQ(o3->at(2), "b");
}

// =============================
// 2. Basic Option API: Construction, State, Value
// =============================
// =============================
// Basic Option API: Construction, State, Value
// =============================
TEST(OptionBasic, SomeAndNone) {
    // Case 1: some(int)
    {
        option<int> opt_some = some(42);
        EXPECT_TRUE(opt_some.is_some());
        EXPECT_FALSE(opt_some.is_none());
    }

    // Case 2: none
    {
        option<int> opt_none = none;
        EXPECT_FALSE(opt_none.is_some());
        EXPECT_TRUE(opt_none.is_none());
    }
}

// unwrap, unwrap_or for value and none
TEST(OptionBasic, UnwrapAndUnwrapOr) {
    // Case 1: unwrap() on some
    {
        option<std::string> opt_str = some("hello"s);
        EXPECT_EQ(opt_str.unwrap(), "hello");
        EXPECT_EQ(opt_str.unwrap_or("world"s), "hello");
    }

    // Case 2: unwrap_or() on none
    {
        option<std::string> opt_none = none;
        EXPECT_EQ(opt_none.unwrap_or("world"s), "world");
    }
}

// operator bool, ==, != for some/none
TEST(OptionBasic, OperatorBoolAndEq) {
    // Case 1: operator bool
    {
        option<int> opt_some = some(1);
        option<int> opt_none = none;
        EXPECT_TRUE(opt_some);
        EXPECT_FALSE(opt_none);
    }

    // Case 2: equality and inequality
    {
        option<int> opt1     = some(1);
        option<int> opt2     = some(1);
        option<int> opt3     = some(2);
        option<int> opt_none = none;
        EXPECT_EQ(opt1, opt2);
        EXPECT_NE(opt1, opt3);
        EXPECT_EQ(opt_none, none);
    }
}

// <, >, ==, <=> for option
TEST(OptionBasic, ThreeWayCompare) {
    // Case 1: <, >, ==, <=> comparisons
    {
        option<int> opt1     = some(1);
        option<int> opt2     = some(2);
        option<int> opt_none = none;
        EXPECT_TRUE((opt1 < opt2));
        EXPECT_TRUE((opt_none < opt1));
        EXPECT_TRUE((opt2 > opt_none));
        EXPECT_TRUE((opt1 == opt1));
    }
}

TEST(OptionBasic, ReferenceAdapter) {
    // Test option<T&> basic reference
    {
        int x        = 10;
        auto opt_ref = some<int &>(x);
        EXPECT_TRUE(opt_ref.is_some());
        EXPECT_EQ(&opt_ref.unwrap(), &x);
        opt_ref.unwrap() = 20;
        EXPECT_EQ(x, 20);
    }

    // Test as_ref() returns reference to same object
    {
        int x        = 30;
        auto opt_ref = some<int &>(x);
        auto cref    = opt_ref.as_ref();
        EXPECT_TRUE(cref.is_some());
        EXPECT_EQ(&cref.unwrap(), &x);
    }
}

// =============================
// 3. Reference Wrapper: std::reference_wrapper, std::ref, std::cref
// =============================
TEST(OptionAPI, ReferenceWrapperSome) {
    // Test std::reference_wrapper<int>
    {
        int x = 123;
        std::reference_wrapper<int> rw(x);
        auto opt_rw = opt::some(rw);
        EXPECT_TRUE(opt_rw.is_some());
        EXPECT_EQ(&opt_rw.unwrap(), &x);
        opt_rw.unwrap() = 456;
        EXPECT_EQ(x, 456);
    }

    // Test std::ref
    {
        int y        = 789;
        auto opt_ref = opt::some(std::ref(y));
        EXPECT_TRUE(opt_ref.is_some());
        EXPECT_EQ(&opt_ref.unwrap(), &y);
        opt_ref.unwrap() = 100;
        EXPECT_EQ(y, 100);
    }

    // Test std::cref
    {
        const int z   = 42;
        auto opt_cref = opt::some(std::cref(z));
        EXPECT_TRUE(opt_cref.is_some());
        EXPECT_EQ(opt_cref.unwrap(), 42);
    }
}

TEST(OptionBasic, PointerAdapter) {
    // Test option<T*> with valid pointer
    {
        int v        = 123;
        auto opt_ptr = some(&v);
        EXPECT_TRUE(opt_ptr.is_some());
        EXPECT_EQ(opt_ptr.unwrap(), &v);
    }

    // Test none_opt<T*>()
    {
        auto opt_none = none_opt<int *>;
        EXPECT_TRUE(opt_none.is_none());
    }
}

// =============================
// 4. Unwrap or Default Value
// =============================
TEST(OptionBasic, UnwrapOrDefault) {
    option<std::string> a = none;
    EXPECT_EQ(a.unwrap_or_default(), "");
    option<int> b = none;
    EXPECT_EQ(b.unwrap_or_default(), 0);
}

// =============================
// 5. Map and Filter
// =============================
TEST(OptionBasic, MapAndFilter) {
    option<int> a = some(5);
    auto b        = a.map([](int x) { return x * 2; });
    EXPECT_EQ(b, some(10));
    auto c = a.filter([](int x) { return x > 10; });
    EXPECT_TRUE(c.is_none());
}

// =============================
// 6. AndThen, OrElse: Chaining Option Operations
// =============================
TEST(OptionBasic, AndThenOrElse) {
    option<int> a = some(3);
    auto b        = a.and_then([](int x) { return some(x + 1); });
    EXPECT_EQ(b, some(4));
    option<int> n = none;
    auto c        = n.or_else([] { return some(99); });
    EXPECT_EQ(c, some(99));
}

// =============================
// 7. Take, Replace, State After Take
// =============================
TEST(OptionBasic, TakeAndReplace) {
    auto a   = some("abc"s);
    auto old = a.replace("xyz"s);
    EXPECT_EQ(a, some("xyz"s));
    EXPECT_EQ(old, some("abc"s));
    auto taken = a.take();
    EXPECT_EQ(taken, some("xyz"s));
    EXPECT_TRUE(a.is_none());
}

// =============================
// 8. Zip and Unzip for Option Pairs
// =============================
TEST(OptionBasic, ZipUnzip) {
    option<int> a = some(1);
    auto b        = some("hi"s);
    auto zipped   = a.zip(b);
    EXPECT_TRUE(zipped.is_some());
    auto [x, y] = zipped.unwrap();
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, "hi");
    auto unzipped = zipped.unzip();
    EXPECT_EQ(unzipped.first, some(1));
    EXPECT_EQ(unzipped.second, some("hi"s));
}

// =============================
// 9. Unwrap/Expect Throws on None
// =============================
TEST(OptionEdge, UnwrapNoneThrows) {
    option<int> n = none;
    EXPECT_THROW(n.unwrap(), opt::option_panic);
    EXPECT_THROW(n.expect("msg"), opt::option_panic);
}

// =============================
// 10. Option<void> Basic Usage
// =============================
TEST(OptionEdge, VoidOption) {
    option<void> a = some();
    option<void> b = none;
    EXPECT_TRUE(a.is_some());
    EXPECT_TRUE(b.is_none());
    EXPECT_NO_THROW(a.unwrap());
    EXPECT_THROW(b.unwrap(), opt::option_panic);
}

// =============================
// 11. Option<T*>: nullptr, Sentinel, none_opt
// =============================
TEST(OptionEdge, PointerNullAndSentinel) {
    int v   = 1;
    auto o1 = some(&v);
    auto o2 = some<int *>(nullptr);
    auto o3 = none_opt<int *>;
    EXPECT_TRUE(o1.is_some());
    EXPECT_EQ(o1.unwrap(), &v);
    EXPECT_TRUE(o2.is_some());
    EXPECT_EQ(o2.unwrap(), nullptr);
    EXPECT_TRUE(o3.is_none());
    EXPECT_NE(o2, o3);
}

// =============================
// 12. Option<T&> Reference Lifetime
// =============================
TEST(OptionEdge, ReferenceLifetime) {
    int x           = 7;
    option<int &> o = some<int &>(x);
    EXPECT_EQ(&o.unwrap(), &x);
    x = 8;
    EXPECT_EQ(o.unwrap(), 8);
}

// =============================
// 13. Move, Copy, Swap for Option
// =============================
TEST(OptionEdge, MoveCopySwap) {
    auto a = some("abc"s);
    auto b = a;
    auto c = std::move(a);
    EXPECT_EQ(b, c);
    b.swap(c);
    EXPECT_EQ(b, c);
    option<std::string> d = none;
    d                     = c;
    EXPECT_EQ(d, c);
    d = std::move(c);
    EXPECT_EQ(d, b);
}

// =============================
// 14. MapOr, MapOrElse, Flatten for Nested Option
// =============================
TEST(OptionEdge, MapOrElseFlatten) {
    option<int> a = some(5);
    auto r1       = a.map_or(0, [](int x) { return x + 1; });
    auto r2       = option<int>{}.map_or(0, [](int x) { return x + 1; });
    EXPECT_EQ(r1, 6);
    EXPECT_EQ(r2, 0);
    auto r3 = a.map_or_else([] { return 100; }, [](int x) { return x * 3; });
    auto r4 = option<int>{}.map_or_else([] { return 100; }, [](int x) { return x * 3; });
    EXPECT_EQ(r3, 15);
    EXPECT_EQ(r4, 100);
    option<option<int>> nested = some(some(42));
    EXPECT_EQ(nested.flatten(), some(42));
    option<option<int>> none_nested = some(option<int>{});
    EXPECT_TRUE(none_nested.flatten().is_none());
}

// =============================
// 15. OkOr, OkOrElse, Expected Interop
// =============================
TEST(OptionEdge, OkOrExpected) {
    auto a  = some("foo"s);
    auto e1 = a.ok_or("err"s);
    EXPECT_TRUE(e1.has_value());
    option<std::string> b = none;
    auto e2               = b.ok_or("err"s);
    EXPECT_FALSE(e2.has_value());
    EXPECT_EQ(e2.error(), "err");
    auto e3 = b.ok_or_else([] { return std::string("fail"); });
    EXPECT_FALSE(e3.has_value());
    EXPECT_EQ(e3.error(), "fail");
}

// =============================
// 16. Option as Range: For Loop
// =============================
TEST(OptionEdge, IteratorBehavior) {
    option<int> a = some(9);
    int sum       = 0;
    for (auto v : a)
        sum += v;
    EXPECT_EQ(sum, 9);
    sum = 0;
    for (auto v : option<int>{})
        sum += v;
    EXPECT_EQ(sum, 0);
}

// =============================
// 17. Hash and Format for Option
// =============================
TEST(OptionEdge, HashAndFormat) {
    option<int> a = some(42);
    option<int> b = none;
    std::unordered_set<option<int>> s;
    s.insert(a);
    s.insert(b);
    EXPECT_EQ(s.count(a), 1u);
    EXPECT_EQ(s.count(b), 1u);
    auto str = std::format("{} {}", a, b);
    EXPECT_TRUE(str.find("some(42)") != std::string::npos);
    EXPECT_TRUE(str.find("none") != std::string::npos);
}

// =============================
// 18. Unzip, Transpose for Nested Option/Expected
// =============================
TEST(OptionEdge, NestedOptionPairExpected) {
    option<std::pair<int, std::string>> p = some(std::pair{ 1, "x"s });
    auto uz                               = p.unzip();
    EXPECT_EQ(uz.first, some(1));
    EXPECT_EQ(uz.second, some("x"s));
    option<std::expected<int, std::string>> oe = some(std::expected<int, std::string>{ 1 });
    auto to                                    = oe.transpose();
    EXPECT_TRUE(to.has_value());
    EXPECT_EQ(to.value(), some(1));
    option<std::expected<int, std::string>> oe2 = some(std::expected<int, std::string>{ std::unexpected("err") });
    auto to2                                    = oe2.transpose();
    EXPECT_FALSE(to2.has_value());
    EXPECT_EQ(to2.error(), "err");
}

// =============================
// 19. GetOrInsert, Insert, Take, Replace, TakeIf
// =============================
TEST(OptionEdge, GetOrInsertInsertTakeReplace) {
    option<int> a = none;
    int &r        = a.get_or_insert(7);
    EXPECT_EQ(r, 7);
    r = 8;
    EXPECT_EQ(a.unwrap(), 8);
    int &r2 = a.get_or_insert_default();
    EXPECT_EQ(&r2, &a.unwrap());
    a       = none;
    int &r3 = a.get_or_insert_with([] { return 9; });
    EXPECT_EQ(r3, 9);
    a.insert(10);
    EXPECT_EQ(a.unwrap(), 10);
    auto taken = a.take();
    EXPECT_EQ(taken, some(10));
    EXPECT_TRUE(a.is_none());
    a        = some(11);
    auto old = a.replace(12);
    EXPECT_EQ(a, some(12));
    EXPECT_EQ(old, some(11));
    auto taken2 = a.take_if([](int v) { return v == 12; });
    EXPECT_EQ(taken2, some(12));
    EXPECT_TRUE(a.is_none());
}

// =============================
// 20. AsRef, AsMut, AsDeref for Option
// =============================
TEST(OptionEdge, AsRefAsMutDeref) {
    int x         = 5;
    option<int> a = some(x);
    auto aref     = a.as_ref();
    EXPECT_TRUE(aref.is_some());
    EXPECT_EQ(aref.unwrap(), x);
    auto amut = a.as_mut();
    EXPECT_TRUE(amut.is_some());
    amut.unwrap() = 6;
    EXPECT_EQ(a.unwrap(), 6);
    option<int *> p = some(&x);
    auto deref      = p.as_deref();
    EXPECT_TRUE(deref.is_some());
    EXPECT_EQ(deref.unwrap(), x);
}

// =============================
// 21. Xor, ZipWith for Option
// =============================
TEST(OptionEdge, XorZipWith) {
    option<int> a = some(1);
    option<int> b = some(2);
    option<int> n = none;
    EXPECT_EQ(a.xor_(n), a);
    EXPECT_EQ(n.xor_(b), b);
    EXPECT_EQ(a.xor_(b), none);
    EXPECT_EQ(n.xor_(n), none);
    auto zipped = a.zip_with(b, [](int x, int y) { return x + y; });
    EXPECT_EQ(zipped, some(3));
}

// =============================
// 22. Static Factory: make_option, none_opt
// =============================
TEST(OptionEdge, StaticMakeOptionNoneOpt) {
    auto a = make_option(1);
    EXPECT_EQ(a, some(1));
    auto b = none_opt<double>;
    EXPECT_TRUE(b.is_none());
}

// =============================
// 23. Interop with std::optional: Assign, Convert, Move
// =============================

// =============================
// Interop with std::optional: Assign, Convert, Move
// =============================
TEST(OptionAPI, StdOptionalCompat) {
    // some -> option -> optional
    std::optional<int> so1 = 123;
    opt::option<int> o1    = so1;
    EXPECT_TRUE(o1.is_some());
    EXPECT_EQ(o1.unwrap(), 123);
    std::optional<int> so1b = static_cast<std::optional<int>>(o1);
    ASSERT_TRUE(so1b.has_value());
    EXPECT_EQ(*so1b, 123);

    // none -> option -> optional
    std::optional<int> so2 = std::nullopt;
    opt::option<int> o2    = so2;
    EXPECT_TRUE(o2.is_none());
    std::optional<int> so2b = static_cast<std::optional<int>>(o2);
    EXPECT_FALSE(so2b.has_value());

    // option some -> optional
    opt::option<int> o3    = opt::some(456);
    std::optional<int> so3 = static_cast<std::optional<int>>(o3);
    ASSERT_TRUE(so3.has_value());
    EXPECT_EQ(*so3, 456);

    // option none -> optional
    opt::option<int> o4    = opt::none;
    std::optional<int> so4 = static_cast<std::optional<int>>(o4);
    EXPECT_FALSE(so4.has_value());

    // move assign
    opt::option<std::string> s1 = std::optional<std::string>("abc");
    opt::option<std::string> s2 = std::move(s1);
    EXPECT_EQ(s2, opt::some("abc"s));
}

// =============================
// 24. Option<void> API: is_some, is_none, unwrap, reset
// =============================
TEST(OptionAPI, VoidOptionAPI) {
    opt::option<void> a = opt::some();
    opt::option<void> b = opt::none;
    EXPECT_TRUE(a.is_some());
    EXPECT_TRUE(b.is_none());
    EXPECT_NO_THROW(a.unwrap());
    EXPECT_THROW(b.unwrap(), opt::option_panic);
    a.reset();
    EXPECT_TRUE(a.is_none());
}

// =============================
// 25. Option<T&> Reference API
// =============================
TEST(OptionAPI, ReferenceEdge) {
    int x                = 1;
    opt::option<int &> o = opt::some<int &>(x);
    EXPECT_EQ(&o.unwrap(), &x);
    o.unwrap() = 42;
    EXPECT_EQ(x, 42);
}

// =============================
// 26. Option<T*> Pointer API
// =============================
TEST(OptionAPI, PointerEdge) {
    int v                 = 7;
    opt::option<int *> o1 = opt::some(&v);
    opt::option<int *> o2 = opt::some<int *>(nullptr);
    opt::option<int *> o3 = opt::none_opt<int *>;
    EXPECT_TRUE(o1.is_some());
    EXPECT_TRUE(o2.is_some());
    EXPECT_TRUE(o3.is_none());
    EXPECT_NE(o2, o3);
    o1.reset();
    EXPECT_TRUE(o1.is_none());
}

// =============================
// 27. Swap, Reset, Insert, GetOrInsert, GetOrInsertDefault, GetOrInsertWith
// =============================
TEST(OptionAPI, SwapResetInsert) {
    opt::option<int> a = opt::some(1);
    opt::option<int> b = opt::none;
    a.swap(b);
    EXPECT_TRUE(a.is_none());
    EXPECT_EQ(b, opt::some(1));
    b.reset();
    EXPECT_TRUE(b.is_none());
    b.insert(9);
    EXPECT_EQ(b, opt::some(9));
    int &r = b.get_or_insert(10);
    EXPECT_EQ(r, 9);
    b       = opt::none;
    int &r2 = b.get_or_insert_default();
    EXPECT_EQ(r2, 0);
    b       = opt::none;
    int &r3 = b.get_or_insert_with([] { return 77; });
    EXPECT_EQ(r3, 77);
}

// =============================
// 28. AsDeref, AsDerefMut for Pointer and UniquePtr
// =============================
TEST(OptionAPI, AsDeref) {
    int x                = 5;
    opt::option<int *> p = opt::some(&x);
    auto d               = p.as_deref();
    EXPECT_TRUE(d.is_some());
    EXPECT_EQ(d.unwrap(), 5);
    auto dm = p.as_deref_mut();
    EXPECT_TRUE(dm.is_some());
    dm.unwrap() = 6;
    EXPECT_EQ(x, 6);
    opt::option<std::unique_ptr<int>> up = opt::some(std::make_unique<int>(8));
    auto d2                              = up.as_deref();
    EXPECT_TRUE(d2.is_some());
    EXPECT_EQ(d2.unwrap(), 8);
}

// =============================
// 29. Flatten, Transpose, Unzip, ZipWith for Nested/Paired Option
// =============================
TEST(OptionAPI, FlattenTransposeUnzipZipWith) {
    opt::option<opt::option<int>> nested = opt::some(opt::some(42));
    EXPECT_EQ(nested.flatten(), opt::some(42));
    opt::option<opt::option<int>> none_nested = opt::some(opt::option<int>{});
    EXPECT_TRUE(none_nested.flatten().is_none());
    opt::option<std::pair<int, std::string>> p = opt::some(std::pair{ 1, "x"s });
    auto uz                                    = p.unzip();
    EXPECT_EQ(uz.first, opt::some(1));
    EXPECT_EQ(uz.second, opt::some("x"s));
    opt::option<std::expected<int, std::string>> oe = opt::some(std::expected<int, std::string>{ 1 });
    auto to                                         = oe.transpose();
    EXPECT_TRUE(to.has_value());
    EXPECT_EQ(to.value(), opt::some(1));
    opt::option<std::expected<int, std::string>> oe2 =
        opt::some(std::expected<int, std::string>{ std::unexpected("err") });
    auto to2 = oe2.transpose();
    EXPECT_FALSE(to2.has_value());
    EXPECT_EQ(to2.error(), "err");
    opt::option<int> a = opt::some(1), b = opt::some(2);
    auto zipped = a.zip_with(b, [](int x, int y) { return x + y; });
    EXPECT_EQ(zipped, opt::some(3));
}

// =============================
// 30. Map, Filter, Inspect for Option
// =============================
TEST(OptionAPI, MapFilterInspect) {
    opt::option<int> a = opt::some(5);
    auto b             = a.map([](int x) { return x * 2; });
    EXPECT_EQ(b, opt::some(10));
    auto c = a.filter([](int x) { return x > 10; });
    EXPECT_TRUE(c.is_none());
    int side = 0;
    a.inspect([&](int v) { side = v; });
    EXPECT_EQ(side, 5);
}

// =============================
// 31. UnwrapUnchecked for Some (UB for None, Not Tested)
// =============================
TEST(OptionAPI, UnwrapUnchecked) {
    opt::option<int> a = opt::some(123);
    EXPECT_EQ(a.unwrap_unchecked(), 123);
}

// =============================
// 32. Operator->, Operator* for Option
// =============================
TEST(OptionAPI, OperatorArrowStar) {
    opt::option<std::string> a = opt::some("abc"s);
    EXPECT_EQ(a->size(), 3u);
    EXPECT_EQ((*a)[0], 'a');
}

// =============================
// 33. All Comparison Operators for Option
// =============================
TEST(OptionAPI, CompareAll) {
    opt::option<int> a = opt::some(1), b = opt::some(2), n = opt::none;
    EXPECT_TRUE((a < b));
    EXPECT_TRUE((n < a));
    EXPECT_TRUE((b > n));
    EXPECT_TRUE((a == a));
    EXPECT_TRUE((n == opt::none));
    EXPECT_TRUE((a != b));
    EXPECT_TRUE((a <=> b) == std::strong_ordering::less);
    EXPECT_TRUE((a <=> n) == std::strong_ordering::greater);
}

// =============================
// 34. Static Factory: make_option, none_opt (Redundant, for Coverage)
// =============================
TEST(OptionAPI, StaticMakeOptionNoneOpt) {
    auto a = opt::make_option(1);
    EXPECT_EQ(a, opt::some(1));
    auto b = opt::none_opt<double>;
    EXPECT_TRUE(b.is_none());
}

// =============================
// 35. Expect Throws with Custom Message
// =============================
TEST(OptionAPI, ExpectMessage) {
    opt::option<int> n = opt::none;
    try {
        n.expect("custom msg");
        FAIL();
    } catch (const opt::option_panic &e) {
        EXPECT_TRUE(std::string(e.what()).find("custom msg") != std::string::npos);
    }
}

// =============================
// 36. Hash and Format for Option (Again, for Coverage)
// =============================
TEST(OptionAPI, HashFormat) {
    opt::option<int> a = opt::some(42);
    opt::option<int> b = opt::none;
    std::unordered_set<opt::option<int>> s;
    s.insert(a);
    s.insert(b);
    EXPECT_EQ(s.count(a), 1u);
    EXPECT_EQ(s.count(b), 1u);
    auto str = std::format("{} {}", a, b);
    EXPECT_TRUE(str.find("some(42)") != std::string::npos);
    EXPECT_TRUE(str.find("none") != std::string::npos);
}

// =============================
// 37. Edge Types: const T, enum, struct, int*, fnptr
// =============================
enum class MyEnum { A, B };
struct MyStruct {
    int x;
};
int f_for_option_fnptr(int x) {
    return x + 1;
}
TEST(OptionAPI, EdgeTypes) {
    auto a = opt::some<const int>(1);
    EXPECT_EQ(a.unwrap(), 1);
    opt::option<MyEnum> e = opt::some(MyEnum::A);
    EXPECT_EQ(e.unwrap(), MyEnum::A);
    opt::option<MyStruct> s = opt::some(MyStruct{ 7 });
    EXPECT_EQ(s.unwrap().x, 7);
    int arr_raw[2]         = { 1, 2 };
    opt::option<int *> arr = opt::some(arr_raw);
    EXPECT_EQ(arr.unwrap()[1], 2);
    using fnptr           = int (*)(int);
    opt::option<fnptr> fn = opt::some(&f_for_option_fnptr);
    EXPECT_EQ(fn.unwrap()(1), 2);
}

// =============================
// 38. Constexpr Usage for Option
// =============================
constexpr opt::option<int> constexpr_some() {
    return opt::some(123);
}
constexpr opt::option<int> constexpr_none() {
    return opt::none_opt<int>;
}
static_assert(constexpr_some().is_some());
static_assert(constexpr_none().is_none());

// =============================
// 39. Move After Option, Moved-From State
// =============================
TEST(OptionAPI, MoveAfter) {
    opt::option<std::string> a = opt::some("abc"s);
    opt::option<std::string> b = std::move(a);
    EXPECT_EQ(b, opt::some("abc"s));
}

// =============================
// 40. Option<Custom> User Type
// =============================
struct Custom {
    int x;
    bool operator==(const Custom &o) const {
        return x == o.x;
    }
};
TEST(OptionAPI, CustomType) {
    opt::option<Custom> a = opt::some(Custom{ 42 });
    EXPECT_EQ(a.unwrap().x, 42);
    a = opt::none;
    EXPECT_TRUE(a.is_none());
}

// =============================
//  Main entry for GoogleTest
// =============================
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}