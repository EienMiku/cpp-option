export module option:ops;

import std;
import :fwd;
import :classes;
import :none;

export template <class T>
constexpr bool std::ranges::enable_view<opt::option<T>> = true;
export template <class T>
constexpr auto std::format_kind<opt::option<T>> = std::range_format::disabled;
export template <class T>
constexpr bool std::ranges::enable_borrowed_range<opt::option<T &>> = true;

export namespace opt {
    namespace detail {
        template <class T>
        concept is_derived_from_optional = requires(const T &t) { []<class U>(const option<U> &) {}(t); };
    } // namespace detail

    // https://eel.is/c++draft/optional.nullopt#lib:nullopt_t
    // struct none_t {}; // See above

    inline constexpr none_t none{};

    constexpr bool operator==(none_t, none_t) noexcept {
        return true;
    }

    constexpr bool operator==(none_t, std::nullopt_t) noexcept {
        return true;
    }

    constexpr std::strong_ordering operator<=>(none_t, none_t) noexcept {
        return std::strong_ordering::equal;
    }

    // https://eel.is/c++draft/optional.relops#lib:operator==,optional
    template <class T, class U>
    constexpr bool operator==(const option<T> &x,
                              const std::optional<U> &y) noexcept(noexcept(static_cast<bool>(*x == *y)))
        requires requires {
            { *x == *y } -> std::convertible_to<bool>;
        }
    {
        if (x.has_value() != y.has_value()) {
            return false;
        } else if (x.has_value() == false) {
            return true;
        } else {
            return *x == *y;
        }
    }

    template <class T, class U>
    constexpr bool operator==(const option<T> &x, const option<U> &y) noexcept(noexcept(static_cast<bool>(*x == *y)))
        requires requires {
            { *x == *y } -> std::convertible_to<bool>;
        }
    {
        if (x.has_value() != y.has_value()) {
            return false;
        } else if (x.has_value() == false) {
            return true;
        } else {
            return *x == *y;
        }
    }

    // https://eel.is/c++draft/optional.relops#lib:operator!=,optional
    template <class T, class U>
    constexpr bool operator!=(const option<T> &x, const option<U> &y) noexcept(noexcept(static_cast<bool>(*x != *y)))
        requires requires {
            { *x != *y } -> std::convertible_to<bool>;
        }
    {
        if (x.has_value() != y.has_value()) {
            return true;
        } else if (x.has_value() == false) {
            return false;
        } else {
            return *x != *y;
        }
    }

    template <typename T, typename U>
    constexpr bool operator==(const option<T> &x, const option<U> &y) noexcept
        requires std::same_as<std::remove_cv_t<T>, void> && std::same_as<std::remove_cv_t<U>, void>
    {
        if (x.has_value() != y.has_value()) {
            return false;
        }
        return true;
    }

    // https://eel.is/c++draft/optional.relops#lib:operator%3c,optional
    template <class T, class U>
    constexpr bool operator<(const option<T> &x, const option<U> &y) noexcept(noexcept(static_cast<bool>(*x < *y))) {
        if (!y) {
            return false;
        } else if (!x) {
            return true;
        } else {
            return *x < *y;
        }
    }

    // https://eel.is/c++draft/optional.relops#lib:operator%3e,optional
    template <class T, class U>
    constexpr bool operator>(const option<T> &x, const option<U> &y) noexcept(noexcept(static_cast<bool>(*x > *y))) {
        if (!x) {
            return false;
        } else if (!y) {
            return true;
        } else {
            return *x > *y;
        }
    }

    // https://eel.is/c++draft/optional.relops#lib:operator%3c=,optional
    template <class T, class U>
    constexpr bool operator<=(const option<T> &x, const option<U> &y) noexcept(noexcept(static_cast<bool>(*x <= *y))) {
        if (!x) {
            return true;
        } else if (!y) {
            return false;
        } else {
            return *x <= *y;
        }
    }

    // https://eel.is/c++draft/optional.relops#lib:operator%3e=,optional
    template <class T, class U>
    constexpr bool operator>=(const option<T> &x, const option<U> &y) noexcept(noexcept(static_cast<bool>(*x >= *y))) {
        if (!y) {
            return true;
        } else if (!x) {
            return false;
        } else {
            return *x >= *y;
        }
    }

    // https://eel.is/c++draft/optional.relops#lib:operator%3c=%3e,optional
    template <class T, std::three_way_comparable_with<T> U>
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const option<T> &x,
                                                                const option<U> &y) noexcept(noexcept(*x <=> *y)) {
        if (x && y) {
            return *x <=> *y;
        } else {
            return x.has_value() <=> y.has_value();
        }
    }

    template <class T, std::three_way_comparable_with<T> U>
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const option<T> &x,
                                                                const std::optional<U> &y) noexcept(noexcept(*x
                                                                                                             <=> *y)) {
        if (x && y) {
            return *x <=> *y;
        } else {
            return x.has_value() <=> y.has_value();
        }
    }

    template <typename T, typename U>
        requires std::same_as<std::remove_cv_t<T>, void> && std::same_as<std::remove_cv_t<U>, void>
    constexpr std::strong_ordering operator<=>(const option<T> &x, const option<U> &y) noexcept {
        if (x && y) {
            return std::strong_ordering::equal;
        } else {
            return x.is_some() <=> y.is_some();
        }
    }

    // https://eel.is/c++draft/optional.nullops#lib:operator==,optional
    template <class T>
    constexpr bool operator==(const std::optional<T> &x, none_t) noexcept {
        return !x;
    }

    template <class T>
    constexpr bool operator==(const option<T> &x, std::nullopt_t) noexcept {
        return !x;
    }

    template <class T>
    constexpr bool operator==(const option<T> &x, none_t) noexcept {
        return !x;
    }

    // https://eel.is/c++draft/optional.nullops#lib:operator%3c=%3e,optional
    template <class T>
    constexpr std::strong_ordering operator<=>(const std::optional<T> &x, none_t) noexcept {
        return x.has_value() <=> false;
    }

    template <class T>
    constexpr std::strong_ordering operator<=>(const option<T> &x, std::nullopt_t) noexcept {
        return x.has_value() <=> false;
    }

    template <class T>
    constexpr std::strong_ordering operator<=>(const option<T> &x, none_t) noexcept {
        return x.has_value() <=> false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator==,optional
    template <class T, class U>
    constexpr bool operator==(const option<T> &x, const U &v) noexcept(noexcept(static_cast<bool>(*x == v)))
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { *x == v } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? *x == v : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator==,optional_
    template <class T, class U>
    constexpr bool operator==(const T &v, const option<U> &x) noexcept(noexcept(static_cast<bool>(v == *x)))
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { v == *x } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? v == *x : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator!=,optional
    template <class T, class U>
    constexpr bool operator!=(const option<T> &x, const U &v) noexcept(noexcept(static_cast<bool>(*x != v)))
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { *x != v } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? *x != v : true;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator!=,optional_
    template <class T, class U>
    constexpr bool operator!=(const T &v, const option<U> &x) noexcept(noexcept(static_cast<bool>(v != *x)))
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { v != *x } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? v != *x : true;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c,optional
    template <class T, class U>
    constexpr bool operator<(const option<T> &x, const U &v)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { *x < v } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? *x < v : true;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c,optional_
    template <class T, class U>
    constexpr bool operator<(const T &v, const option<U> &x)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { v < *x } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? v < *x : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e,optional
    template <class T, class U>
    constexpr bool operator>(const option<T> &x, const U &v)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { *x > v } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? *x > v : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e,optional_
    template <class T, class U>
    constexpr bool operator>(const T &v, const option<U> &x)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { v > *x } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? v > *x : true;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c=,optional
    template <class T, class U>
    constexpr bool operator<=(const option<T> &x, const U &v)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { *x <= v } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? *x <= v : true;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c=,optional_
    template <class T, class U>
    constexpr bool operator<=(const T &v, const option<U> &x)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { v <= *x } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? v <= *x : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e=,optional
    template <class T, class U>
    constexpr bool operator>=(const option<T> &x, const U &v)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { *x >= v } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? *x >= v : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e=,optional_
    template <class T, class U>
    constexpr bool operator>=(const T &v, const option<U> &x)
        requires (!detail::specialization_of<U, std::optional>)
              && (!detail::specialization_of<U, option>)
              && requires {
                     { v >= *x } -> std::convertible_to<bool>;
                 }
    {
        return x.has_value() ? v >= *x : true;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c=%3e,optional
    template <class T, class U>
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const option<T> &x, const U &v)
        requires (!detail::is_derived_from_optional<U>) && std::three_way_comparable_with<T, U>
    {
        return x.has_value() ? (*x <=> v) : std::strong_ordering::less;
    }

    // https://eel.is/c++draft/optional.specalg#lib:swap,optional
    template <class T>
    constexpr void swap(std::optional<T> &x, option<T> &y) noexcept(noexcept(y.swap(x)))
        requires std::is_reference_v<T> || (std::is_move_constructible_v<T> && std::is_swappable_v<T>)
    {
        y.swap(x);
    }

    template <class T>
    constexpr void swap(option<T> &x, std::optional<T> &y) noexcept(noexcept(x.swap(y)))
        requires std::is_reference_v<T> || (std::is_move_constructible_v<T> && std::is_swappable_v<T>)
    {
        x.swap(y);
    }

    template <class T>
    constexpr void swap(option<T> &x, option<T> &y) noexcept(noexcept(x.swap(y)))
        requires std::is_reference_v<T> || (std::is_move_constructible_v<T> && std::is_swappable_v<T>)
    {
        x.swap(y);
    }

    // https://eel.is/c++draft/optional.specalg#lib:make_optional
    template <class T>
    constexpr option<std::decay_t<T>> make_option(T &&v) noexcept(
        std::is_nothrow_constructible_v<option<std::decay_t<T>>, T>)
        requires (std::is_constructible_v<std::decay_t<T>, T>)
    {
        return option<std::decay_t<T>>(std::forward<T>(v));
    }

    template <class T, class... Args>
    constexpr option<T> make_option(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        return option<T>(std::in_place, std::forward<Args>(args)...);
    }

    template <class T, class U, class... Args>
    constexpr option<T> make_option(std::initializer_list<U> il, Args &&...args) noexcept(
        std::is_nothrow_constructible_v<T, std::initializer_list<U> &, Args...>) {
        return option<T>(std::in_place, il, std::forward<Args>(args)...);
    }

    template <typename T>
    constexpr option<std::decay_t<T>> some(T &&value) noexcept(
        std::is_nothrow_constructible_v<option<std::decay_t<T>>, T &&>)
        requires (!detail::specialization_of<std::decay_t<T>, std::reference_wrapper>)
              && (!detail::option_prohibited_type<std::decay_t<T>>)
    {
        return option<std::decay_t<T>>{ std::forward<T>(value) };
    }

    // needs to specify `<T&>`
    template <typename T>
    constexpr option<T> some(std::type_identity_t<T> value_ref)
        requires std::is_lvalue_reference_v<T> && (!detail::option_prohibited_type<T>)
    {
        return option<T>{ value_ref };
    }

    // std::ref => T&
    template <typename T>
    constexpr option<typename T::type &> some(T value_ref) noexcept
        requires detail::specialization_of<T, std::reference_wrapper>
              && (!detail::option_prohibited_type<typename T::type>)
    {
        return option<typename T::type &>{ value_ref.get() };
    }

    template <typename T, typename... Ts>
    constexpr option<T> some(Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts &&...>)
        requires std::constructible_from<T, Ts &&...> && (!detail::option_prohibited_type<T>)
    {
        return option<T>{ std::in_place, T(std::forward<Ts>(args)...) };
    }

    template <typename T, typename U, typename... Ts>
    constexpr option<T> some(std::initializer_list<U> il, Ts &&...args) noexcept(
        std::is_nothrow_constructible_v<option<T>, std::initializer_list<U> &, Ts &&...>)
        requires std::constructible_from<T, std::initializer_list<U>, Ts &&...> && (!detail::option_prohibited_type<T>)
    {
        return option<T>{ std::in_place, il, std::forward<Ts>(args)... };
    }

    constexpr option<void> some() noexcept {
        return option<void>{ true };
    }

    template <typename T>
    constexpr option<T> none_opt() noexcept {
        return option<T>{};
    }

    template <typename T>
        requires (!detail::specialization_of<T, std::optional>)
    option(T) -> option<T>;

    template <typename T>
    option(std::reference_wrapper<T>) -> option<T &>;

    template <typename T>
        requires detail::specialization_of<std::remove_cvref_t<T>, std::optional>
    option(T &&) -> option<typename std::remove_cvref_t<T>::value_type>;
} // namespace opt

export namespace opt::detail {
    template <typename T>
    concept hash_enabled = requires { std::hash<T>{}(std::declval<T>()); };

    constexpr std::size_t unspecified_hash_value = 0;
} // namespace opt::detail

// https://eel.is/c++draft/optional.hash#lib:hash,optional
export template <typename T>
    requires opt::detail::hash_enabled<std::remove_const_t<T>> || std::same_as<std::remove_cv_t<T>, void>
struct std::hash<opt::option<T>> {
    std::size_t operator()(const opt::option<T> &o) const
        noexcept(noexcept(std::hash<std::remove_const_t<T>>()(o.unwrap_unchecked()))
                 || std::same_as<std::remove_cv_t<T>, void>) {
        if (o.is_some()) {
            if constexpr (std::same_as<std::remove_cv_t<T>, void>) {
                return 1;
            } else {
                return std::hash<std::remove_const_t<T>>()(o.unwrap_unchecked());
            }
        }
        return opt::detail::unspecified_hash_value;
    }
};

export template <typename T>
struct std::formatter<opt::option<T>> {
    constexpr auto parse(format_parse_context &ctx) noexcept {
        return ctx.begin();
    }

    auto format(const opt::option<T> &opt, auto &ctx) const {
        if constexpr (std::same_as<std::remove_cv_t<T>, void>) {
            if (opt.is_some()) {
                return std::format_to(ctx.out(), "some()");
            }
            return std::format_to(ctx.out(), "none");
        } else {
            if (opt.is_some()) {
                return std::format_to(ctx.out(), "some({})", opt.unwrap_unchecked());
            }
            return std::format_to(ctx.out(), "none");
        }
    }
};

