module;

#include <cassert>

export module option:classes;

import std;
import :fwd;
import :panic;
import :storage;
import :none;

#pragma push_macro("force_inline")
#undef force_inline
#if defined(__clang__) || defined(__GNUC__)
    #define force_inline [[gnu::always_inline]]
#else
    #define force_inline [[msvc::forceinline]]
#endif

#pragma push_macro("hot_path")
#undef hot_path
#if defined(__clang__) || defined(__GNUC__)
    #define hot_path [[gnu::hot]]
#else
    #define hot_path
#endif

#pragma push_macro("cpp20_no_unique_address")
#undef cpp20_no_unique_address
#if defined(_MSC_VER)
    #define cpp20_no_unique_address [[msvc::no_unique_address]]
#else
    #define cpp20_no_unique_address [[no_unique_address]]
#endif

#pragma push_macro("has_cpp_lib_optional_ref")
#undef has_cpp_lib_optional_ref
#if (defined(__cpp_lib_optional) && __cpp_lib_optional >= 202506L)                                                     \
    || (defined(__cpp_lib_freestanding_optional) && __cpp_lib_freestanding_optional >= 202506L)
    #define has_cpp_lib_optional_ref 1
#else
    #define has_cpp_lib_optional_ref 0
#endif

export namespace opt {
    template <>
    class option<void> {

    public:
        using value_type = void;

    private:
        detail::option_storage<value_type> storage;

    public:
        constexpr option() noexcept = default;
        constexpr option(std::nullopt_t) noexcept {}
        constexpr option(none_t) noexcept {}
        constexpr option(bool has_value) noexcept : storage{ has_value } {}
        constexpr explicit option(std::in_place_t) noexcept : storage{ true } {}
        constexpr option(const option &) = default;
        constexpr option(option &&) = default;

        constexpr option &operator=(std::nullopt_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option &operator=(none_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option &operator=(const option &) = default;
        constexpr option &operator=(option &&) = default;

        constexpr void emplace() noexcept {
            storage.emplace();
        }

        constexpr void swap(option &rhs) noexcept {
            std::swap(storage, rhs.storage);
        }

        // iterator

        constexpr auto operator->(this auto &&) noexcept = delete;
        constexpr auto &&operator*(this auto &&) noexcept = delete;

        constexpr explicit operator bool() const noexcept {
            return is_some();
        }

        constexpr bool has_value() const noexcept {
            return is_some();
        }

        // No `void &`
        constexpr auto &value(this auto &&) = delete;

        constexpr auto value_or(this auto &&, auto &&) noexcept {}

        template <typename F>
        constexpr auto and_then(F &&f) const {
            using U = std::invoke_result_t<F>;

            static_assert(detail::option_type<U> || detail::specialization_of<std::remove_cvref_t<U>, std::optional>);

            if (is_some()) {
                auto result = std::invoke(std::forward<F>(f));
                if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                    // Convert std::optional to option for consistency
                    using value_type = typename std::remove_cvref_t<U>::value_type;
                    return option<value_type>{ std::move(result) };
                } else {
                    return result;
                }
            }

            if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{};
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        template <std::invocable F>
        constexpr auto transform(F &&f) const -> option<std::remove_cv_t<std::invoke_result_t<F>>> {
            return map(std::forward<F>(f));
        }

        template <std::invocable F>
        constexpr auto or_else(F &&f) const
            requires (detail::option_type<std::invoke_result_t<F>>
                      || detail::specialization_of<std::remove_cvref_t<std::invoke_result_t<F>>, std::optional>)
        {
            using U = std::invoke_result_t<F>;

            if (is_some()) {
                return *this;
            }

            auto result = std::invoke(std::forward<F>(f));
            if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{ std::move(result) };
            } else {
                return result;
            }
        }

        constexpr void reset() noexcept {
            storage.reset();
        }

        template <detail::option_like U>
        constexpr auto and_(U &&optb) const noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<U>>)
            -> option {
            if (is_some()) {
                return option(std::forward<U>(optb));
            }
            return option{};
        }

        constexpr auto as_deref_mut() = delete;

        // No `void &`
        constexpr auto as_mut() = delete;

        // No `void &`
        constexpr auto as_ref() = delete;

        constexpr auto expect(const char *msg) const -> void {
            if (is_none()) [[unlikely]] {
                throw option_panic(msg);
            }
        }

        template <std::predicate F>
        constexpr auto filter(F &&f) const -> option {
            if (is_some() && std::invoke(std::forward<F>(f))) {
                return *this;
            }
            return option{};
        }

        // No `void &`
        template <typename U>
        constexpr auto &get_or_insert(U &&) = delete;

        // No `void &`
        constexpr auto &get_or_insert_default() = delete;

        // No `void &`
        template <std::invocable F>
        constexpr auto &get_or_insert_with(F &&) = delete;

        // No `void &`
        constexpr auto insert() = delete;

        template <typename Self, std::invocable F>
        constexpr auto inspect(this Self &&self, F &&f) -> Self && {
            if (self.is_some()) {
                std::invoke(std::forward<F>(f));
            }
            return std::forward<Self>(self);
        }

        constexpr auto is_none() const noexcept -> bool {
            return !is_some();
        }

        template <std::predicate F>
        constexpr auto is_none_or(F &&f) const -> bool {
            return is_none() || std::invoke(std::forward<F>(f));
        }

        hot_path constexpr auto is_some() const noexcept -> bool {
            return storage.has_value();
        }

        template <std::predicate F>
        constexpr auto is_some_and(F &&f) const -> bool {
            return is_some() && std::invoke(std::forward<F>(f));
        }

        template <std::invocable F>
        constexpr auto map(F &&f) const -> option<std::remove_cv_t<std::invoke_result_t<F>>> {
            using U = std::remove_cv_t<std::invoke_result_t<F>>;

            if (is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f));
                    return option{ true };
                } else {
                    return option<U>{ std::invoke(std::forward<F>(f)) };
                }
            }

            return option<U>{};
        }

        template <typename F, typename U, typename RF = std::invoke_result_t<F>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<RF> && std::is_lvalue_reference_v<U>,
                                                    std::common_reference_t<RF, U>, std::remove_cvref_t<RF>>>
            requires std::invocable<F>
        constexpr Ret map_or(U &&default_value, F &&f) const
            requires (!std::is_lvalue_reference_v<RF> || std::is_lvalue_reference_v<U>)
                  && std::convertible_to<RF, Ret>
                  && std::convertible_to<U, Ret>
        {
            if (is_some()) {
                return std::invoke(std::forward<F>(f));
            }
            return std::forward<U>(default_value);
        }

        template <typename D, typename F, typename RF = std::invoke_result_t<F>, typename RD = std::invoke_result_t<D>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<RF> && std::is_lvalue_reference_v<RD>,
                                                    std::common_reference_t<RF, RD>, std::remove_cvref_t<RF>>>
            requires std::invocable<D>
                  && std::invocable<F>
                     constexpr Ret map_or_else(D &&default_f, F &&f) const
                         requires (!std::is_lvalue_reference_v<RF> || std::is_lvalue_reference_v<RD>)
                               && std::convertible_to<RF, Ret>
                               && std::convertible_to<RD, Ret>
        {
            if (is_some()) {
                return std::invoke(std::forward<F>(f));
            }
            return std::invoke(std::forward<D>(default_f));
        }

        template <typename E>
        constexpr auto ok_or(E &&err) const -> std::expected<value_type, std::remove_cvref_t<E>> {
            using result_type = std::expected<value_type, std::remove_cvref_t<E>>;
            if (is_some()) {
                return result_type{};
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        template <std::invocable F>
        constexpr auto ok_or_else(F &&err) const
            -> std::expected<value_type, std::remove_cvref_t<std::invoke_result_t<F>>> {
            using result_type = std::expected<value_type, std::remove_cvref_t<std::invoke_result_t<F>>>;
            if (is_some()) {
                return result_type{};
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(err)) };
        }

        template <typename Self, detail::option_like U>
        constexpr auto or_(this Self &&self, U &&optb) -> option {
            if (self.is_some()) {
                return self;
            }
            return option(std::forward<U>(optb));
        }

        template <typename Self>
        constexpr auto replace(this Self &&self) -> Self && {
            return std::forward<Self>(self);
        }

        constexpr auto take() -> option {
            option result{};
            if (is_some()) {
                std::ranges::swap(storage, result.storage);
            }
            return result;
        }

        template <typename F>
        constexpr auto take_if(F &&f) -> option {
            option result{};
            if (is_some() && std::invoke(std::forward<F>(f))) {
                std::ranges::swap(storage, result.storage);
            }
            return result;
        }

        constexpr auto unwrap() const -> void {
            if (is_none()) [[unlikely]] {
                throw option_panic("Attempted to access value of empty option");
            }
        }

        constexpr auto unwrap_or() const -> void {}

        constexpr auto unwrap_or_default() const -> void {}

        template <std::invocable F>
        constexpr auto unwrap_or_else(F &&f) const -> void {
            if (is_none()) [[unlikely]] {
                std::invoke(std::forward<F>(f));
            }
        }

        constexpr auto unwrap_unchecked() const -> void {}

        template <detail::option_like U>
            requires (!detail::option_type<U>) || std::same_as<typename U::value_type, value_type>
        constexpr auto xor_(U &&optb) const -> option {
            auto optb_reformed = option(std::forward<U>(optb));
            if (is_some() && optb_reformed.is_none()) {
                return *this;
            }
            if (is_none() && optb_reformed.is_some()) {
                return optb_reformed;
            }
            return option{};
        }

        // Cannot initialize `std::pair` containing `void` types
        constexpr auto zip(auto &&) const = delete;
        constexpr auto zip_with(auto &&, auto &&) const = delete;

        static constexpr auto default_() noexcept -> option {
            return option{};
        }

        static constexpr auto from() -> option {
            return { true };
        }

        constexpr auto cmp(const option &other) const -> std::strong_ordering {
            if (is_some() && other.is_some()) {
                return std::strong_ordering::equal;
            } else {
                return is_some() <=> other.is_some();
            }
        }

        constexpr auto(max)(const option &other) const noexcept -> option {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_some() ? *this : other;
        }

        constexpr auto(max)(option &&other) const noexcept -> option {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_some() ? *this : other;
        }

        constexpr auto(min)(const option &other) const noexcept -> option {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_none() ? *this : other;
        }

        constexpr auto(min)(option &&other) const noexcept -> option {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_none() ? *this : other;
        }

        constexpr auto clamp(const option &, const option &) const noexcept -> option {
            return *this;
        }

        constexpr auto clamp(option &&, option &&) const noexcept -> option {
            return *this;
        }

        constexpr auto eq(const option &other) const noexcept -> bool {
            return is_some() == other.is_some();
        }

        constexpr auto ne(const option &other) const noexcept -> bool {
            return is_some() != other.is_some();
        }

        constexpr auto partial_cmp(const option &other) const noexcept -> std::strong_ordering {
            return cmp(other);
        }

        constexpr auto lt(const option &other) const noexcept -> bool {
            return is_none() && other.is_some();
        }

        constexpr auto le(const option &other) const noexcept -> bool {
            return is_none() || other.is_some();
        }

        constexpr auto gt(const option &other) const noexcept -> bool {
            return is_some() && other.is_none();
        }

        constexpr auto ge(const option &other) const noexcept -> bool {
            return is_some() || other.is_none();
        }

        constexpr auto clone() const noexcept -> option {
            return *this;
        }

        constexpr void clone_from(const option &source) noexcept {
            storage.has_value_ = source.storage.has_value_;
        }
    };

    template <>
    class option<const void> : public option<void> {
    public:
        using value_type = const void;
        using option<void>::option;
        using option<void>::operator=;
    };

    template <>
    class option<volatile void> : public option<void> {
    public:
        using value_type = volatile void;
        using option<void>::option;
        using option<void>::operator=;
    };

    template <>
    class option<const volatile void> : public option<void> {
    public:
        using value_type = const volatile void;
        using option<void>::option;
        using option<void>::operator=;
    };

    template <typename T>
    class option {
    private:
        detail::option_storage<T> storage;

        template <class F, class... Ts>
        constexpr option(detail::within_invoke_t, F &&f, Ts &&...args) :
            storage{ detail::within_invoke_t{}, std::forward<F>(f), std::forward<Ts>(args)... } {}

    public:
        static_assert(!detail::option_prohibited_type<T>);

        template <class U>
        friend class option;

        constexpr explicit operator std::optional<T>() const noexcept(std::is_nothrow_copy_constructible_v<T>) {
            if (is_some()) {
                return std::optional<T>{ storage.get() };
            }
            return std::nullopt;
        }

        // https://eel.is/c++draft/optional.optional.general
        using value_type = T;
        using iterator = T *;
        using const_iterator = const T *;

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor
        constexpr option() noexcept = default;

        constexpr option(std::nullopt_t) noexcept : storage{} {}

        constexpr option(none_t) noexcept : storage{} {}

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor_
        //
        // Unlike `std::optional`, `option` uses a different storage mechanism, making
        // this constructor non-trivial even if `T` is trivially copy constructible.
        // For a trivial constructor, consider using the `option` type directly.
        constexpr option(const std::optional<T> &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>) {
            if (rhs.has_value()) {
                storage.emplace(*rhs);
            }
        }

        constexpr option(const option &) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor__
        //
        // Unlike `std::optional`, `option` uses a different storage mechanism, making
        // this constructor non-trivial even if `T` is trivially move constructible.
        // For a trivial constructor, consider using the `option` type directly.
        constexpr option(std::optional<T> &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::is_move_constructible_v<T>
        {
            if (rhs.has_value()) {
                storage.emplace(std::move(*rhs));
            }
        }

        constexpr option(option &&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor___
        template <class... Args>
        constexpr explicit option(std::in_place_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
            requires std::is_constructible_v<T, Args...>
            : storage{ std::in_place, std::forward<Args>(args)... } {}

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor____
        template <class U, class... Args>
        constexpr explicit option(std::in_place_t, std::initializer_list<U> il, Args &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Args...>)
            requires std::is_constructible_v<T, std::initializer_list<U>, Args...>
            : storage{ std::in_place, il, std::forward<Args>(args)... } {}

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor_____
        template <class U = std::remove_cv_t<T>>
        constexpr explicit(!std::convertible_to<U, T>) option(U &&v) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, std::optional<T>>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, option>)
                  && ((!std::same_as<std::remove_cv_t<T>, bool>)
                      || (!detail::specialization_of<std::remove_cvref_t<U>, std::optional>
                          && !detail::specialization_of<std::remove_cvref_t<U>, option>))
                  && std::is_constructible_v<T, U>
            : storage(std::in_place, std::forward<U>(v)) {}

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor______
        template <class U>
        constexpr explicit(!std::is_convertible_v<const U &, T>)
            option(const std::optional<U> &rhs) noexcept(std::is_nothrow_constructible_v<T, const U &>)
            requires (!std::same_as<T, U>)
                  && std::is_constructible_v<T, const U &>
                  && (std::same_as<std::remove_cv_t<T>, bool> || !detail::converts_from_any_cvref<T, std::optional<U>>)
        {
            if (rhs.has_value()) {
                storage.emplace(*rhs);
            }
        }

        template <typename U>
        constexpr explicit(!std::convertible_to<const U &, T>)
            option(const option<U> &rhs) noexcept(std::is_nothrow_constructible_v<T, const U &>)
            requires (!std::same_as<T, U>)
                  && std::is_constructible_v<T, const U &>
                  && (std::same_as<std::remove_cv_t<T>, bool> || !detail::converts_from_any_cvref<T, option<U>>)
        {
            if (rhs.is_some()) {
                storage.emplace(rhs.storage.get());
            }
        }

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor_______
        template <class U>
        constexpr explicit(!std::is_convertible_v<U, T>)
            option(std::optional<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires (!std::same_as<T, U>)
                  && std::is_constructible_v<T, U>
                  && (std::same_as<std::remove_cv_t<T>, bool> || !detail::converts_from_any_cvref<T, std::optional<U>>)
        {
            if (rhs.has_value()) {
                storage.emplace(*std::move(rhs));
            }
        }

        template <typename U>
        constexpr explicit(!std::convertible_to<U, T>)
            option(option<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires (!std::same_as<T, U>)
                  && std::constructible_from<T, U>
                  && (std::same_as<std::remove_cv_t<T>, bool> || !detail::converts_from_any_cvref<T, option<U>>)
        {
            if (rhs.is_some()) {
                storage.emplace(std::move(rhs.storage.get()));
            }
        }

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional
        constexpr option<T> &operator=(std::nullopt_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option<T> &operator=(none_t) noexcept {
            storage.reset();
            return *this;
        }

        // https://eel.is/c++draft/optional.optional#lib:operator=,optional_
        constexpr option &operator=(const option &) noexcept(std::is_nothrow_copy_assignable_v<T>
                                                             && std::is_nothrow_copy_constructible_v<T>)
            requires std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T>
        = default;

        // https://eel.is/c++draft/optional.optional#lib:operator=,optional__
        constexpr option &operator=(option &&) noexcept(std::is_nothrow_move_assignable_v<T>
                                                        && std::is_nothrow_move_constructible_v<T>)
            requires std::is_move_assignable_v<T> && std::is_move_constructible_v<T>
        = default;

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional___
        template <class U = std::remove_cv_t<T>>
        constexpr option &operator=(U &&v) noexcept(std::is_nothrow_constructible_v<T, U>
                                                    && std::is_nothrow_assignable_v<T &, U>)
            requires (!std::is_same_v<std::remove_cvref_t<U>, option>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, std::optional<T>>)
                  && (!std::conjunction_v<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>>)
                  && std::is_constructible_v<T, U>
                  && std::is_assignable_v<T &, U>
        {
            if (is_some()) {
                storage.get() = std::forward<U>(v);
            } else {
                storage.emplace(std::forward<U>(v));
            }
            return *this;
        }

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional____
        template <class U>
        constexpr option &operator=(const std::optional<U> &rhs) noexcept(
            std::is_nothrow_constructible_v<T, const U &> && std::is_nothrow_assignable_v<T &, const U &>)
            requires (!std::is_same_v<T, U>)
                  && std::is_constructible_v<T, const U &>
                  && std::is_assignable_v<T &, const U &>
                  && (!detail::converts_from_any_cvref<T, std::optional<U>>)
                  && (!std::is_assignable_v<T &, std::optional<U> &>)
                  && (!std::is_assignable_v<T &, std::optional<U> &&>)
                  && (!std::is_assignable_v<T &, const std::optional<U> &>)
                  && (!std::is_assignable_v<T &, const std::optional<U> &&>)

        {
            if (rhs.has_value()) {
                if (storage.has_value()) {
                    storage.get() = *rhs;
                } else {
                    storage.emplace(*rhs);
                }
            } else {
                if (storage.has_value()) {
                    storage.reset();
                }
            }
            return *this;
        }

        template <typename U>
        constexpr option &operator=(const option<U> &rhs) noexcept(std::is_nothrow_constructible_v<T, const U &>
                                                                   && std::is_nothrow_assignable_v<T &, const U &>)
            requires (!std::is_same_v<T, U>)
                  && std::is_constructible_v<T, const U &>
                  && std::is_assignable_v<T &, const U &>
                  && (!detail::converts_from_any_cvref<T, option<U>>)
                  && (!std::is_assignable_v<T &, option<U> &>)
                  && (!std::is_assignable_v<T &, option<U> &&>)
                  && (!std::is_assignable_v<T &, const option<U> &>)
                  && (!std::is_assignable_v<T &, const option<U> &&>)
        {
            if (rhs.is_some()) {
                if (storage.has_value()) {
                    storage.get() = rhs.storage.get();
                } else {
                    storage.emplace(rhs.storage.get());
                }
            } else {
                if (storage.has_value()) {
                    storage.reset();
                }
            }
            return *this;
        }

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional_____
        template <class U>
        constexpr option &operator=(std::optional<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T, U>
                                                                     && std::is_nothrow_assignable_v<T &, U>)
            requires (!std::is_same_v<T, U>)
                  && std::is_constructible_v<T, U>
                  && std::is_assignable_v<T &, U>
                  && (!std::is_same_v<std::remove_cvref_t<U>, option<T>>)
                  && (!detail::converts_from_any_cvref<T, std::optional<U>>)
                  && (!std::is_assignable_v<T &, std::optional<U> &>)
                  && (!std::is_assignable_v<T &, std::optional<U> &&>)
                  && (!std::is_assignable_v<T &, const std::optional<U> &>)
                  && (!std::is_assignable_v<T &, const std::optional<U> &&>)
        {
            if (rhs.has_value()) {
                if (storage.has_value()) {
                    storage.get() = *std::move(rhs);
                } else {
                    storage.emplace(*std::move(rhs));
                }
            } else {
                if (storage.has_value()) {
                    storage.reset();
                }
            }
            return *this;
        }

        template <typename U>
        constexpr option &operator=(option<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T, U>
                                                              && std::is_nothrow_assignable_v<T &, U>)
            requires (!std::is_same_v<T, U>)
                  && std::is_constructible_v<T, U>
                  && std::is_assignable_v<T &, U>
                  && (!detail::converts_from_any_cvref<T, option<U>>)
                  && (!std::is_assignable_v<T &, option<U> &>)
                  && (!std::is_assignable_v<T &, option<U> &&>)
                  && (!std::is_assignable_v<T &, const option<U> &>)
                  && (!std::is_assignable_v<T &, const option<U> &&>)
        {
            if (rhs.is_some()) {
                if (storage.has_value()) {
                    storage.get() = std::move(rhs.storage).get();
                } else {
                    storage.emplace(std::move(rhs.storage).get());
                }
            } else {
                storage.reset();
            }
            return *this;
        }

        // https://eel.is/c++draft/optional.assign#lib:emplace,optional
        template <typename... Args>
        constexpr auto emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> T & {
            static_assert(std::constructible_from<T, Args...>);
            reset();
            storage.emplace(std::forward<Args>(args)...);
            return storage.get();
        }

        // https://eel.is/c++draft/optional.assign#lib:emplace,optional_
        //
        // Standard-compliant `emplace` overload with initializer_list support.
        template <class U, class... Args>
        constexpr auto emplace(std::initializer_list<U> il, Args &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Args &&...>) -> T &
            requires std::is_constructible_v<T, std::initializer_list<U> &, Args...>
        {
            reset();
            storage.emplace(il, std::forward<Args>(args)...);
            return storage.get();
        }

        // https://eel.is/c++draft/optional.swap#lib:swap,optional
        constexpr void swap(std::optional<T> &rhs) noexcept(std::is_nothrow_move_constructible_v<T>
                                                            && std::is_nothrow_swappable_v<T>)
            requires detail::cpp17_swappable<T>
        {
            static_assert(std::is_move_constructible_v<T>);

            if (rhs.has_value()) {
                if (storage.has_value()) {
                    std::ranges::swap(storage.get(), *rhs);
                } else {
                    storage.emplace(std::move(*rhs));
                    rhs.reset();
                }
            } else {
                if (storage.has_value()) {
                    rhs.emplace(std::move(storage.get()));
                    storage.reset();
                }
            }
        }

        constexpr void swap(option &rhs) noexcept(std::is_nothrow_move_constructible_v<T>
                                                  && std::is_nothrow_swappable_v<T>)
            requires detail::cpp17_swappable<T>
        {
            if (rhs.is_some()) {
                if (storage.has_value()) {
                    std::ranges::swap(storage.get(), rhs.storage.get());
                } else {
                    storage.emplace(std::move(rhs.storage.get()));
                    rhs.reset();
                }
            } else {
                if (storage.has_value()) {
                    rhs.storage.emplace(std::move(storage.get()));
                    storage.reset();
                }
            }
        }

        // https://eel.is/c++draft/optional.iterators#lib:iterator,optional
        // See above

        // https://eel.is/c++draft/optional.iterators#lib:begin,optional
        constexpr iterator begin() noexcept {
            if (is_some()) {
                return std::addressof(storage.get());
            }
            return nullptr;
        }

        constexpr const_iterator begin() const noexcept {
            if (is_some()) {
                return std::addressof(storage.get());
            }
            return nullptr;
        }

        // https://eel.is/c++draft/optional.iterators#lib:end,optional
        constexpr iterator end() noexcept {
            return begin() + has_value();
        }

        constexpr const_iterator end() const noexcept {
            return begin() + has_value();
        }

        // https://eel.is/c++draft/optional.observe#lib:operator-%3e,optional
        template <class Self>
        hot_path constexpr auto operator->(this Self &&self) noexcept
            -> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const T *, T *> {
            assert(self.has_value());
            return std::addressof(self.storage.get());
        }

        // https://eel.is/c++draft/optional.observe#lib:operator*,optional
        // https://eel.is/c++draft/optional.observe#lib:operator*,optional_
        template <class Self>
        hot_path constexpr auto operator*(this Self &&self) noexcept -> auto && {
            assert(self.has_value());
            return std::forward_like<Self>(self.storage.get());
        }

        // https://eel.is/c++draft/optional.observe#lib:operator_bool,optional
        constexpr explicit operator bool() const noexcept {
            return is_some();
        }

        // https://eel.is/c++draft/optional.observe#lib:has_value,optional
        constexpr bool has_value() const noexcept {
            return is_some();
        }

        // https://eel.is/c++draft/optional.observe#lib:value,optional
        // https://eel.is/c++draft/optional.observe#lib:value,optional_
        template <class Self>
        constexpr auto value(this Self &&self) -> auto && {
            if (self.is_none()) {
                throw std::bad_optional_access{};
            }
            return std::forward_like<Self>(self.storage.get());
        }

        // https://eel.is/c++draft/optional.observe#lib:value_or,optional
        // https://eel.is/c++draft/optional.observe#lib:value_or,optional_
        template <typename Self, typename U = std::remove_cv_t<T>>
        constexpr auto value_or(this Self &&self, U &&v) -> T {
            static_assert(std::is_lvalue_reference_v<Self>
                              ? std::is_copy_constructible_v<T> && std::is_convertible_v<U &&, T>
                              : std::is_move_constructible_v<T> && std::is_convertible_v<U &&, T>);

            if (self.is_some()) {
                return *std::forward<Self>(self);
            }
            return static_cast<T>(std::forward<U>(v));
        }

        // https://eel.is/c++draft/optional.monadic#lib:and_then,optional
        // https://eel.is/c++draft/optional.monadic#lib:and_then,optional_
        template <typename Self, typename F>
        constexpr auto and_then(this Self &&self, F &&f) {
            using U = std::invoke_result_t<F, decltype(*std::forward<Self>(self))>;

            static_assert(detail::option_type<U> || detail::specialization_of<std::remove_cvref_t<U>, std::optional>);

            if (self.is_some()) [[likely]] {
                auto result = std::invoke(std::forward<F>(f), *std::forward<Self>(self));
                if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                    // Convert std::optional to option for consistency
                    using value_type = typename std::remove_cvref_t<U>::value_type;
                    return option<value_type>{ std::move(result) };
                } else {
                    return result;
                }
            }

            if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{};
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        // https://eel.is/c++draft/optional.monadic#lib:transform,optional
        // https://eel.is/c++draft/optional.monadic#lib:transform,optional_
        template <typename Self, typename F>
        constexpr auto transform(this Self &&self, F &&f) {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(*std::forward<Self>(self))>>;

            if (self.is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f), *std::forward<Self>(self));
                    return option<void>{ true };
                } else {
                    return option<U>{ detail::within_invoke, std::forward<F>(f), *std::forward<Self>(self) };
                }
            }
            return option<U>{};
        }

        // https://eel.is/c++draft/optional.monadic#lib:or_else,optional
        template <std::invocable F>
        constexpr option or_else(F &&f) const &
            requires std::copy_constructible<T>
        {
            static_assert(std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, std::optional<T>>
                          || std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, option>);

            if (*this) {
                return *this;
            } else {
                if constexpr (std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, std::optional<T>>) {
                    using value_type = typename std::remove_cvref_t<std::invoke_result_t<F>>::value_type;
                    return option<value_type>{ std::forward<F>(f)() };
                } else {
                    return std::forward<F>(f)();
                }
            }
        }

        // https://eel.is/c++draft/optional.monadic#lib:or_else,optional_
        template <std::invocable F>
        constexpr option or_else(F &&f) &&
            requires std::move_constructible<T>
        {
            static_assert(std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, std::optional<T>>
                          || std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, option>);

            if (*this) {
                return std::move(*this);
            } else {
                if constexpr (std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, std::optional<T>>) {
                    using value_type = typename std::remove_cvref_t<std::invoke_result_t<F>>::value_type;
                    return option<value_type>{ std::forward<F>(f)() };
                } else {
                    return std::forward<F>(f)();
                }
            }
        }

        // https://eel.is/c++draft/optional.mod#lib:reset,optional
        constexpr void reset() noexcept {
            storage.reset();
        }

        // Returns an empty option if this option is empty; otherwise returns `optb`.
        //
        // The argument `optb` is always evaluated before calling this function. If
        // you want lazy evaluation (e.g., passing the result of a function call),
        // consider using `and_then` instead.
        force_inline constexpr auto and_(this auto &&self, const option &optb) -> option {
            return self.is_some() ? optb : option{};
        }

        force_inline constexpr auto and_(this auto &&self, option &&optb) -> option {
            return self.is_some() ? std::move(optb) : option{};
        }

        template <detail::option_like U>
            requires (!std::same_as<std::remove_cvref_t<U>, option>)
        force_inline constexpr auto and_(this auto &&self, U &&optb) -> option {
            return self.is_some() ? option(std::forward<U>(optb)) : option{};
        }

        // Returns an empty option if this option is empty, otherwise calls `f` with
        // the wrapped value and returns the result.
        //
        // Some languages call this operation flatmap.
        // See above

        // Converts an `option<T>` to an `option<const U&>`, where `U` is the result
        // of applying the dereference operator to `T`.
        //
        // Leaves the original `option` in-place, returning a new one holding a const
        // reference to the dereferenced value of the original.
        constexpr auto as_deref() const
            requires requires(const T &v) {
                { *v } -> detail::not_void;
            }
        {
            return as_ref().map([](const auto &self) -> const auto & { return *self; });
        }

        // Converts from `option<T>` to `option<U&>`, where `U` is the result of applying
        // the dereference operator to `T`.
        //
        // Leaves the original `option` in-place, returning a new one holding a reference
        // to the dereferenced value of the original.
        constexpr auto as_deref_mut()
            requires requires(T &v) {
                { *v } -> detail::not_void;
            }
        {
            return as_mut().map([](auto &&self) -> decltype(auto) { return *self; });
        }

        // Converts from `option<T> &` to `option<T&>`.
        // (Creates an option holding a reference to the original value.)
        constexpr auto as_mut() noexcept -> option<T &> {
            if (is_some()) {
                return option<T &>{ storage.get() };
            }
            return option<T &>{};
        }

        // Converts from `option<T> &` to `option<const T&>`.
        // (Creates an option holding a const reference to the original value.)
        constexpr auto as_ref() const noexcept -> option<const T &> {
            if (is_some()) {
                return option<const T &>{ storage.get() };
            }
            return option<const T &>{};
        }

        // Returns the contained value.
        //
        // Throws if the option is empty with a custom message provided by `msg`.
        template <class Self>
        constexpr auto expect(this Self &&self, const char *msg) -> auto && {
            if (self.is_none()) {
                throw option_panic(msg);
            }
            return *std::forward<Self>(self);
        }

        // Returns an empty option if the option is empty, otherwise calls `predicate`
        // with the wrapped value and returns:
        //
        // - `some(t)` if `predicate` returns `true` (where `t` is the wrapped value)
        // - `none` if `predicate` returns `false`
        template <typename Self, typename F>
        constexpr auto filter(this Self &&self, F &&predicate) -> option
            requires std::predicate<F, decltype(self.storage.get())>
        {
            if (self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get())) {
                return option{ self };
            }
            return option{};
        }

        // Converts from `option<option<T>>` to `option<T>`.
        template <typename Self>
        constexpr auto flatten(this Self &&self) -> std::remove_cvref_t<T>
            requires detail::option_type<T>
        {
            if (self.is_some()) {
                return *std::forward<Self>(self);
            }
            return std::remove_cvref_t<T>{};
        }

        // Inserts `value` into the option if it is empty, then returns a reference
        // to the contained value.
        //
        // See also `option::insert`, which updates the value even if the option already
        // contains a value.
        template <typename U>
        constexpr auto get_or_insert(U &&value) -> T &
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            if (is_none()) [[unlikely]] {
                storage.emplace(std::forward<U>(value));
            }
            return storage.get();
        }

        // Inserts the default value into the option if it is empty, then returns a
        // reference to the contained value.
        constexpr auto get_or_insert_default() -> T &
            requires std::default_initializable<T>
        {
            if (is_none()) {
                storage.emplace(T{});
            }
            return storage.get();
        }

        // Inserts a value computed from `f` into the option if it is empty, then returns
        // a reference to the contained value.
        template <std::invocable F>
        constexpr auto get_or_insert_with(F &&f) -> T &
            requires std::constructible_from<T, std::invoke_result_t<F>>
        {
            if (is_none()) [[unlikely]] {
                storage.emplace(std::invoke(std::forward<F>(f)));
            }
            return storage.get();
        }

        // Inserts `value` into the option, then returns a reference to it.
        //
        // If the option already contains a value, the old value is dropped.
        //
        // See also `option::get_or_insert`, which doesn't update the value if the option
        // already contains a value.
        template <typename U>
        constexpr auto insert(U &&value) -> T &
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            storage.reset();
            storage.emplace(std::forward<U>(value));
            return storage.get();
        }

        // Calls a function with a reference to the contained value if the option contains
        // a value.
        //
        // Returns the original option.
        template <typename Self, typename F>
        constexpr auto inspect(this Self &&self, F &&f) -> Self && {
            if (self.is_some()) [[likely]] {
                std::invoke(std::forward<F>(f), self.storage.get());
            }
            return std::forward<Self>(self);
        }

        // Returns `true` if the option is empty.
        constexpr auto is_none() const noexcept -> bool {
            return !is_some();
        }

        // Returns `true` if the option is empty or the value inside matches a predicate.
        template <typename F>
        constexpr auto is_none_or(this auto &&self, F &&f) -> bool
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_none() || std::invoke(std::forward<F>(f), self.storage.get());
        }

        // Returns `true` if the option contains a value.
        hot_path constexpr auto is_some() const noexcept -> bool {
            return storage.has_value();
        }

        // Returns `true` if the option contains a value and the value inside matches a
        // predicate.
        template <typename F>
        constexpr auto is_some_and(this auto &&self, F &&predicate) -> bool
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get());
        }

        // Maps an `option<T>` to `option<U>` by applying a function to a contained value
        // (if the option contains a value) or returns `none` (if the option is empty).
        template <typename Self, typename F>
        constexpr auto map(this Self &&self, F &&f)
            -> option<std::remove_cv_t<std::invoke_result_t<F, decltype(*std::forward<Self>(self))>>> {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(*std::forward<Self>(self))>>;

            if (self.is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f), *std::forward<Self>(self));
                    return option<void>{ true };
                } else {
                    return option<U>{ detail::within_invoke, std::forward<F>(f), *std::forward<Self>(self) };
                }
            }
            return option<U>{};
        }

        // Returns the provided default result (if the option is empty), or applies a
        // function to the contained value (if any).
        //
        // Arguments passed to `map_or` are eagerly evaluated; if you are passing the
        // result of a function call, it is recommended to use `map_or_else`, which is
        // lazily evaluated.
        template <typename Self, typename U, typename F,
                  typename RF = std::invoke_result_t<F, decltype(*std::declval<Self>())>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<RF> && std::is_lvalue_reference_v<U>,
                                                    std::common_reference_t<RF, U>, std::remove_cvref_t<RF>>>
        constexpr Ret map_or(this Self &&self, U &&default_value, F &&f)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && std::invocable<F, decltype(*self)>
                  && (!std::is_lvalue_reference_v<RF> || std::is_lvalue_reference_v<U>)
                  && std::convertible_to<RF, Ret>
                  && std::convertible_to<U, Ret>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), *std::forward<Self>(self));
            }
            return std::forward<U>(default_value);
        }

        // Maps an `option<T>` to a `U` by applying function `f` to the contained value if
        // the option contains a value, otherwise returns the default constructed value of
        // the type `U`.
        template <typename Self, typename F>
        constexpr auto map_or_default(this Self &&self, F &&f)
            -> std::invoke_result_t<F, decltype(*std::forward<Self>(self))>
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && std::invocable<F, decltype(*std::forward<Self>(self))>
                  && std::default_initializable<std::invoke_result_t<F, decltype(*std::forward<Self>(self))>>
        {
            using U = std::invoke_result_t<F, decltype(*std::forward<Self>(self))>;
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), *std::forward<Self>(self));
            }
            return U{};
        }

        // Computes a default function result (if the option is empty), or applies a
        // different function to the contained value (if any).
        template <typename Self, typename D, typename F,
                  typename RF = std::invoke_result_t<F, decltype(*std::declval<Self>())>,
                  typename RD = std::invoke_result_t<D>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<RF> && std::is_lvalue_reference_v<RD>,
                                                    std::common_reference_t<RF, RD>, std::remove_cvref_t<RF>>>
            requires std::invocable<D>
        constexpr Ret map_or_else(this Self &&self, D &&default_f, F &&f)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && std::invocable<F, decltype(*std::forward<Self>(self))>
                  && (!std::is_lvalue_reference_v<RF> || std::is_lvalue_reference_v<RD>)
                  && std::convertible_to<RF, Ret>
                  && std::convertible_to<RD, Ret>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), *std::forward<Self>(self));
            }
            return std::invoke(std::forward<D>(default_f));
        }

        // Transforms the `option<T>` into a `std::expected<T, E>`, mapping `some(v)` to
        // `std::expected<T, E>` with `v` and `none` to `std::expected<T, E>` which with
        // the given error.
        //
        // Arguments passed to `ok_or` are eagerly evaluated; if you are passing the
        // result of a function call, it is recommended to use `ok_or_else`, which is
        // lazily evaluated.
        template <typename Self, typename E>
        constexpr auto ok_or(this Self &&self, E &&err) -> std::expected<T, std::remove_cvref_t<E>>
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && std::constructible_from<std::remove_cvref_t<E>, E>
        {
            using result_type = std::expected<T, std::remove_cvref_t<E>>;
            if (self.is_some()) {
                return result_type{ *std::forward<Self>(self) };
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        // Transforms the `option<T>` into a `std::expected<T, E>`, mapping `some(v)` to
        // `std::expected<T, E>` with `v` and `none` to `std::expected<T, E>` which with
        // the error returned by `err`.
        template <typename Self, std::invocable F>
        constexpr auto ok_or_else(this Self &&self, F &&err)
            -> std::expected<T, std::remove_cvref_t<std::invoke_result_t<F>>>
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F>>, std::invoke_result_t<F>>
        {
            using result_type = std::expected<T, std::remove_cvref_t<std::invoke_result_t<F>>>;
            if (self.is_some()) {
                return result_type{ *std::forward<Self>(self) };
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(err)) };
        }

        // Returns the option if it contains a value, otherwise returns `optb`.
        //
        // Arguments passed to `or_` are eagerly evaluated; if you are passing the result
        // of a function call, it is recommended to use `or_else`, which is lazily
        // evaluated.
        template <detail::option_like U>
        constexpr auto or_(this auto &&self, U &&optb) -> option {
            if (self.is_some()) {
                return self;
            }
            return option(std::forward<U>(optb));
        }

        // Returns the option if it contains a value, otherwise calls `f` and returns
        // the result.
        // `or_else`, See above

        // Replaces the actual value in the option by the value given in parameter,
        // returning the old value if present, leaving a `some` in its place without
        // deinitializing either one.
        template <typename U>
        constexpr auto replace(this auto &&self, U &&value) -> option
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U, T>
        {
            option<T> old{};
            if (self.is_some()) {
                std::ranges::swap(self.storage, old.storage);
            }

            self.storage.reset();
            self.storage.emplace(std::forward<U>(value));

            return old;
        }

        // Takes the value out of the option, leaving an empty option in its place.
        constexpr auto take(this auto &&self) -> option {
            option result{};
            if (self.is_some()) {
                std::ranges::swap(self.storage, result.storage);
            }
            return result;
        }

        // Takes the value out of the option, but only if the predicate evaluates to
        // `true` on a reference to the value.
        //
        // In other words, replaces `self` with an empty option if the predicate returns
        // `true`. This function operates similarly to `option::take` but conditionally.
        template <typename F>
        constexpr auto take_if(this auto &&self, F &&f) -> option
            requires std::predicate<F, decltype(self.storage.get())>
        {
            option result{};
            if (self.is_some() && std::invoke(std::forward<F>(f), self.storage.get())) {
                std::ranges::swap(self.storage, result.storage);
            }
            return result;
        }

        // Transposes an `option` of a `std::expected` into a `std::expected` of an
        // `option`.
        //
        // `none` will be mapped to `std::expected<option<value_type>, error_type>` with
        // an empty `option`. If the `option` contains a `std::expected`, its `value` is
        // wrapped in `option` and returned as `std::expected`, while its `error` is
        // propagated directly.
        constexpr auto transpose(this auto &&self)
            requires detail::expected_type<T>
        {
            using value_type = T::value_type;
            using error_type = T::error_type;

            if (self.is_some()) {
                auto &expected_val = self.storage.get();
                if (expected_val.has_value()) {
                    return std::expected<option<value_type>, error_type>{ option<value_type>{ expected_val.value() } };
                }
                return std::expected<option<value_type>, error_type>{ std::unexpect, expected_val.error() };
            }
            return std::expected<option<value_type>, error_type>{ option<value_type>{} };
        }

        // Returns a reference to the contained value if the option is not empty.
        //
        // Throws an `option_panic` exception if the option is empty.
        template <typename Self>
        constexpr auto unwrap(this Self &&self) -> auto &&
            requires (!std::same_as<std::remove_cv_t<T>, void>)
        {
            if (self.is_none()) [[unlikely]] {
                throw option_panic("Attempted to access value of empty option");
            }
            return *std::forward<Self>(self);
        }

        // Returns the contained value or a provided default.
        //
        // Arguments passed to `unwrap_or` are eagerly evaluated; if you are passing the
        // result of a function call, it is recommended to use `unwrap_or_else`, which is
        // lazily evaluated.
        template <typename Self, typename U = std::remove_cv_t<T>, typename R = decltype(*std::declval<Self>()),
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<R> && std::is_lvalue_reference_v<U>,
                                                    std::common_reference_t<R, U>, std::remove_cvref_t<R>>>
        constexpr Ret unwrap_or(this Self &&self, U &&default_value)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && std::convertible_to<R, Ret>
                  && std::convertible_to<U, Ret>
        {
            if (self.is_some()) [[likely]] {
                return *std::forward<Self>(self);
            }
            return std::forward<U>(default_value);
        }

        // Returns the contained Some value or a default.
        //
        // If the option contains a value, returns the contained value; otherwise, returns
        // the default value for that type.
        template <typename Self>
        constexpr auto unwrap_or_default(this Self &&self) -> T
            requires std::default_initializable<T>
        {
            if (self.is_some()) [[likely]] {
                return *std::forward<Self>(self);
            }
            return T{};
        }

        // Returns the contained value or computes it from a function.
        template <typename Self, std::invocable F, typename R = decltype(*std::declval<Self>()),
                  typename RF = std::invoke_result_t<F>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<R> && std::is_lvalue_reference_v<RF>,
                                                    std::common_reference_t<R, RF>, std::remove_cvref_t<R>>>
        constexpr Ret unwrap_or_else(this Self &&self, F &&f)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && (!std::is_lvalue_reference_v<RF> || std::is_lvalue_reference_v<R>)
                  && std::convertible_to<R, Ret>
                  && std::convertible_to<RF, Ret>
        {
            if (self.is_some()) {
                return *std::forward<Self>(self);
            }
            return std::invoke(std::forward<F>(f));
        }

        // Returns the contained value, without checking that the value is not empty.
        hot_path constexpr auto unwrap_unchecked(this auto &&self) noexcept -> auto && {
            return *std::forward<decltype(self)>(self);
        }

        // Unzips an option containing a tuple-like value.
        //
        // If `self` is `some(tuple{ a, b, ... })` this function returns `tuple{
        // some(a), some(b), ... }`. Otherwise, `tuple{ none, none, ... }` is returned.
        // For pair-like types, returns a `std::pair` instead of `std::tuple`.
        template <typename Self>
        constexpr auto unzip(this Self &&self)
            requires detail::tuple_like<std::remove_cvref_t<T>>
        {
            using tuple_type = std::remove_cvref_t<T>;
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                if (self.is_some()) {
                    auto &&t = *std::forward<Self>(self);
                    if constexpr (detail::pair_like<tuple_type>) {
                        return std::pair{ option<std::tuple_element_t<Is, tuple_type>>{
                            std::get<Is>(std::forward<decltype(t)>(t)) }... };
                    } else {
                        return std::tuple{ option<std::tuple_element_t<Is, tuple_type>>{
                            std::get<Is>(std::forward<decltype(t)>(t)) }... };
                    }
                }
                if constexpr (detail::pair_like<tuple_type>) {
                    return std::pair{ option<std::tuple_element_t<Is, tuple_type>>{}... };
                } else {
                    return std::tuple{ option<std::tuple_element_t<Is, tuple_type>>{}... };
                }
            }(std::make_index_sequence<std::tuple_size_v<tuple_type>>{});
        }

        // Returns `some` if exactly one of `self`, `optb` is `some`, otherwise returns an
        // empty option.
        template <typename Self>
        force_inline constexpr auto xor_(this Self &&self, const option &optb)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
        {
            const bool self_some = self.is_some();
            const bool optb_some = optb.is_some();

            if (self_some != optb_some) {
                return self_some ? self : optb;
            }
            return option{};
        }

        template <typename Self>
        force_inline constexpr auto xor_(this Self &&self, option &&optb)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
        {
            const bool self_some = self.is_some();
            const bool optb_some = optb.is_some();

            if (self_some != optb_some) {
                return self_some ? self : std::move(optb);
            }
            return option{};
        }

        template <typename Self, typename U>
            requires detail::option_like<U>
                  && (!std::same_as<std::remove_cvref_t<U>, option>)
                     force_inline constexpr auto xor_(this Self &&self, U &&optb)
                         requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T>
                                                                    : std::move_constructible<T>)
        {
            auto optb_option = option(std::forward<U>(optb));
            const bool self_some = self.is_some();
            const bool optb_some = optb_option.is_some();

            if (self_some != optb_some) {
                return self_some ? self : optb_option;
            }
            return option{};
        }

        // Zips `self` with another `option`.
        //
        // If `self` is `some(s)` and `other` is `some(o)`, this function returns
        // `some(std::pair{ s, o })`. Otherwise, `none` is returned.
        template <typename Self, detail::option_type U>
        constexpr auto zip(this Self &&self, U &&optb)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<U>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
        {
            if (self.is_some() && optb.is_some()) {
                return option<std::pair<T, typename std::remove_cvref_t<U>::value_type>>{
                    { *std::forward<Self>(self), *std::forward<U>(optb) }
                };
            }
            return option<std::pair<T, typename std::remove_cvref_t<U>::value_type>>{};
        }

        // Zips `self` and another `option` with function `f`.
        //
        // If `self` is `some(s)` and `other` is `some(o)`, this function returns
        // `some(f(s, o))`. Otherwise, `none` is returned.
        template <typename Self, detail::option_type U, typename F>
        constexpr auto zip_with(this Self &&self, U &&other, F &&f)
            requires (std::is_lvalue_reference_v<Self> ? std::copy_constructible<T> : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<U>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
                  && std::invocable<F, decltype(*self), decltype(*other)>
        {
            using result_type = std::remove_cv_t<std::invoke_result_t<F, decltype(*self), decltype(*other)>>;
            if (self.is_some() && other.is_some()) {
                return option<result_type>{ std::invoke(std::forward<F>(f), *std::forward<Self>(self),
                                                        *std::forward<U>(other)) };
            }
            return option<result_type>{};
        }

        constexpr auto clone(this auto &&self) noexcept(detail::noexcept_cloneable<T>) -> option
            requires detail::cloneable<T>
        {
            if (self.is_some()) {
                return option{ detail::clone(*self) };
            }
            return option{};
        }

        constexpr void clone_from(this auto &&self, const option &source) noexcept(detail::noexcept_cloneable<T>)
            requires detail::cloneable<T> && std::is_copy_assignable_v<T>
        {
            if (source.is_some()) {
                if (self.is_some()) {
                    if constexpr (detail::has_clone<T>) {
                        self.storage.get() = detail::clone(*source);
                    } else {
                        self.storage.get() = *source;
                    }
                } else {
                    self.emplace(detail::clone(*source));
                }
            } else {
                self.reset();
            }
        }

        static constexpr auto default_() noexcept -> option {
            return option{};
        }

        // Creates a new option by moving or copying `value` into it.
        template <typename U>
        static constexpr auto from(U &&value) -> option
            requires std::constructible_from<T, U>
        {
            if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
                return option{ std::forward<U>(value) };
            }
            return option{ value };
        }

        // Compares `self` with another `option` for ordering using three-way comparison.
        //
        // If both options contain values, compares the contained values using their `<=>` operator.
        // If one option is empty and the other contains a value, the empty option is considered less.
        // If both options are empty, they are considered equal.
        //
        // This function is equivalent to using the `<=>` operator directly on the options.
        constexpr auto cmp(const option &other) const noexcept(requires(const option &lhs, const option &rhs) {
            { lhs <=> rhs } noexcept;
        }) // -> decltype(*this <=> other)
            requires std::three_way_comparable<T>
        {
            return *this <=> other;
        }

        // Returns the option with the maximum value.
        //
        // If both options contain values, returns the option with the greater value.
        // If only one option contains a value, returns that option.
        // If both are empty, returns an empty option.
        constexpr auto(max)(this auto &&self,
                            const option<T> &other) noexcept(noexcept(static_cast<bool>(self > other))) -> option<T>
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self > other ? self : other;
            }
            return self.is_some() ? self : other;
        }

        constexpr auto(max)(this auto &&self, option<T> &&other) noexcept(noexcept(static_cast<bool>(self > other)))
            -> option<T>
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self > other ? self : std::move(other);
            }
            return self.is_some() ? self : std::move(other);
        }

        // Returns the option with the minimum value.
        //
        // If both options contain values, returns the option with the smaller value.
        // If only one option is empty, returns that option.
        // If both are empty, returns an empty option.
        constexpr auto(min)(this auto &&self,
                            const option<T> &other) noexcept(noexcept(static_cast<bool>(self < other))) -> option<T>
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self < other ? self : other;
            }
            return self.is_none() ? self : other;
        }

        constexpr auto(min)(this auto &&self, option<T> &&other) noexcept(noexcept(static_cast<bool>(self < other)))
            -> option<T>
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self < other ? self : std::move(other);
            }
            return self.is_none() ? self : std::move(other);
        }

        // Clamps the value in the option to the given bounds.
        //
        // If the option contains a value, clamps it between `min` and `max`.
        // If the option is empty, returns an empty option.
        // Returns `min` if the value is less than `min`,
        // Returns `max` if the value is greater than `max`,
        // Otherwise returns `self`.
        constexpr auto clamp(this auto &&self, const option<T> &min,
                             const option<T> &max) noexcept(noexcept(static_cast<bool>(self < min))
                                                            && noexcept(static_cast<bool>(self > max))) -> option<T>
            requires std::three_way_comparable<T>
        {
            if (self.is_some()) {
                if (min.is_some() && self < min) {
                    return min;
                }
                if (max.is_some() && self > max) {
                    return max;
                }
                return self;
            }
            return option<T>{};
        }

        constexpr auto clamp(this auto &&self, option<T> &&min,
                             option<T> &&max) noexcept(noexcept(static_cast<bool>(self < min))
                                                       && noexcept(static_cast<bool>(self > max))) -> option<T>
            requires std::three_way_comparable<T>
        {
            if (self.is_some()) {
                if (min.is_some() && self < min) {
                    return min;
                }
                if (max.is_some() && self > max) {
                    return max;
                }
                return self;
            }
            return option<T>{};
        }

        // Tests for equality between two options.
        //
        // Returns `true` if both options are empty or both contain equal values.
        // This is equivalent to using the `==` operator.
        constexpr auto eq(this auto &&self, const option<T> &other) noexcept(noexcept(static_cast<bool>(self == other)))
            -> bool
            requires std::equality_comparable<T>
        {
            return self == other;
        }

        // Tests for inequality between two options.
        //
        // Returns `true` if one option is empty while the other contains a value,
        // or if both contain values that are not equal.
        // This is equivalent to using the `!=` operator.
        constexpr auto ne(this auto &&self, const option<T> &other) noexcept(noexcept(static_cast<bool>(self != other)))
            -> bool
            requires std::equality_comparable<T>
        {
            return self != other;
        }

        // Performs partial comparison between two options.
        //
        // Returns the result of three-way comparison.
        // This is equivalent to calling `cmp` function.
        constexpr auto partial_cmp(this auto &&self, const option<T> &other) noexcept(noexcept(self.cmp(other)))
            -> decltype(self.cmp(other))
            requires std::three_way_comparable<T>
        {
            return self.cmp(other);
        }

        // Tests if this option is less than another option.
        //
        // Returns `true` if this option is less than the other option.
        // Empty options are considered less than any non-empty option.
        // This is equivalent to using the `<` operator.
        constexpr auto lt(this auto &&self, const option<T> &other) noexcept(noexcept(static_cast<bool>(self < other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self < other;
        }

        // Tests if this option is less than or equal to another option.
        //
        // Returns `true` if this option is less than or equal to the other option.
        // Empty options are considered less than any non-empty option.
        // This is equivalent to using the `<=` operator.
        constexpr auto le(this auto &&self, const option<T> &other) noexcept(noexcept(static_cast<bool>(self <= other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self <= other;
        }

        // Tests if this option is greater than another option.
        //
        // Returns `true` if this option is greater than the other option.
        // Non-empty options are considered greater than any empty option.
        // This is equivalent to using the `>` operator.
        constexpr auto gt(this auto &&self, const option<T> &other) noexcept(noexcept(static_cast<bool>(self > other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self > other;
        }

        // Tests if this option is greater than or equal to another option.
        //
        // Returns `true` if this option is greater than or equal to the other option.
        // Non-empty options are considered greater than any empty option.
        // This is equivalent to using the `>=` operator.
        constexpr auto ge(this auto &&self, const option<T> &other) noexcept(noexcept(static_cast<bool>(self >= other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self >= other;
        }

        constexpr auto as_mut_slice() = delete;
        constexpr auto as_pin_mut() = delete;
        constexpr auto as_pin_ref() = delete;
        constexpr auto as_slice() = delete;
        constexpr auto as_span(this auto &&self) -> decltype(std::span{ self.begin(), self.has_value() }) {
            return std::span{ self.begin(), self.has_value() };
        }
        auto iter() = delete;
        auto iter_mut() = delete;
        static auto from_iter() = delete;
        static auto from_residual() = delete;
        auto into_iter() = delete;
    };

    namespace detail {
        template <typename T>
        struct option_lref_iterator_base {};

        template <typename T>
            requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
        struct option_lref_iterator_base<T> {
            using iterator = T *;
        };
    } // namespace detail

    template <typename T>
    class option<T &> : public detail::option_lref_iterator_base<T> {
    private:
        detail::option_storage<T &> storage;

        template <class F, class... Ts>
        constexpr option(detail::within_invoke_t, F &&f, Ts &&...args) {
            T &r = std::invoke(std::forward<F>(f), std::forward<Ts>(args)...);
            storage.ptr = std::addressof(r);
        }

    public:
        template <typename U>
        friend class option;

        static_assert(!detail::option_prohibited_type<T &>);

        // https://eel.is/c++draft/optional.optional.ref.general
        using value_type = T;

        constexpr option() noexcept = default;
        constexpr option(std::nullopt_t) noexcept : option() {}
        constexpr option(none_t) noexcept : option() {}

#if has_cpp_lib_optional_ref
        template <class U = T>
        constexpr option(const std::optional<U &> &rhs) noexcept {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*rhs);
            }
        }
#endif

        constexpr option(const option &rhs) noexcept = default;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:1
        template <class Arg>
        constexpr explicit option(std::in_place_t, Arg &&arg) noexcept(std::is_nothrow_constructible_v<T &, Arg>)
            requires std::is_constructible_v<T &, Arg>
                  && (!detail::cpp23_reference_constructs_from_temporary_v<T &, Arg>)
        {
            storage.convert_ref_init_val(std::forward<Arg>(arg));
        }

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:2
        template <class U>
        constexpr explicit(!std::convertible_to<U, T &>) option(U &&u) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires detail::non_std_optional_of<U, T>
                  && (!std::is_same_v<std::remove_cvref_t<U>, option>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                  && std::is_constructible_v<T &, U>
        {
            storage.convert_ref_init_val(std::forward<U>(u));
        }

        template <class U>
        constexpr explicit(!std::convertible_to<U, T &>) option(U &&) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires detail::non_std_optional_of<U, T>
                      && (!std::is_same_v<std::remove_cvref_t<U>, option>)
                      && (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                      && std::is_constructible_v<T &, U>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, U>
        = delete;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:3
#if has_cpp_lib_optional_ref
        template <class U>
        constexpr explicit(!std::is_convertible_v<U &, T &>)
            option(std::optional<U> &rhs) noexcept(std::is_nothrow_constructible_v<T &, U &>)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, U &>
        {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*rhs);
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<U &, T &>)
            option(std::optional<U> &rhs) noexcept(std::is_nothrow_constructible_v<T &, U &>)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, U &>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, U &>
        = delete;
#endif

        template <class U>
        constexpr explicit(!std::is_convertible_v<U &, T &>)
            option(option<U> &rhs) noexcept(std::is_nothrow_constructible_v<T &, U &>)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, U &>
        {
            if (rhs.is_some()) {
                storage.convert_ref_init_val(rhs.storage.get());
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<U &, T &>) option(option<U> &rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, U &>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, U &>
        = delete;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:4
#if has_cpp_lib_optional_ref
        template <class U>
        constexpr explicit(!std::is_convertible_v<const U &, T &>)
            option(const std::optional<U> &rhs) noexcept(std::is_nothrow_constructible_v<T &, const U &>)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, const U &>
        {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*rhs);
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<const U &, T &>) option(const std::optional<U> &rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, const U &>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, const U &>
        = delete;
#endif

        template <class U>
        constexpr explicit(!std::is_convertible_v<const U &, T &>)
            option(const option<U> &rhs) noexcept(std::is_nothrow_constructible_v<T &, const U &>)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, const U &>
        {
            if (rhs.is_some()) {
                storage.convert_ref_init_val(rhs.storage.get());
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<const U &, T &>) option(const option<U> &rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, const U &>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, const U &>
        = delete;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:5
#if has_cpp_lib_optional_ref
        template <class U>
        constexpr explicit(!std::is_convertible_v<U, T &>)
            option(std::optional<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, U>
        {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*std::move(rhs));
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<U, T &>) option(std::optional<U> &&rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, U>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, U>
        = delete;
#endif

        template <class U>
        constexpr explicit(!std::is_convertible_v<U, T &>)
            option(option<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, U>
        {
            if (rhs.is_some()) {
                storage.convert_ref_init_val(std::move(rhs.storage.get()));
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<U, T &>) option(option<U> &&rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, U>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, U>
        = delete;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:6
#if has_cpp_lib_optional_ref
        template <class U>
        constexpr explicit(!std::is_convertible_v<const U, T &>)
            option(const std::optional<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T &, const U>)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, const U>
        {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*std::move(rhs));
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<const U, T &>) option(const std::optional<U> &&rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, std::optional<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, const U>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, const U>
        = delete;
#endif

        template <class U>
        constexpr explicit(!std::is_convertible_v<const U, T &>)
            option(const option<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T &, const U>)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                  && (!std::is_same_v<T &, U>)
                  && std::is_constructible_v<T &, const U>
        {
            if (rhs.is_some()) {
                storage.convert_ref_init_val(std::move(rhs.storage.get()));
            }
        }

        template <class U>
        constexpr explicit(!std::is_convertible_v<const U, T &>) option(const option<U> &&rhs)
            requires (!std::is_same_v<std::remove_cv_t<T>, option<U>>)
                      && (!std::is_same_v<T &, U>)
                      && std::is_constructible_v<T &, const U>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, const U>
        = delete;

#if has_cpp_lib_optional_ref
        constexpr option &operator=(const std::optional<T> &rhs) noexcept {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*rhs);
            } else {
                storage.reset();
            }
            return *this;
        }
#endif

        constexpr option &operator=(const option &) noexcept = default;

        // https://eel.is/c++draft/optional.ref.assign#itemdecl:1
        constexpr option &operator=(std::nullopt_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option &operator=(none_t) noexcept {
            storage.reset();
            return *this;
        }

        // https://eel.is/c++draft/optional.ref.assign#itemdecl:2
        template <typename U>
        constexpr auto &emplace(U &&u) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires std::is_constructible_v<T &, U> && (!detail::cpp23_reference_constructs_from_temporary_v<T &, U>)
        {
            storage.convert_ref_init_val(std::forward<U>(u));
            return storage.get();
        }

        // https://eel.is/c++draft/optional.ref.swap
#if has_cpp_lib_optional_ref
        constexpr void swap(std::optional<T> &rhs) noexcept {
            if (rhs.has_value()) {
                if (storage.has_value()) {
                    T *ptr = storage.ptr;
                    storage.reset();
                    storage.convert_ref_init_val(std::addressof(rhs.value()));
                    rhs.reset();
                    rhs.emplace(*ptr);
                } else {
                    T *their_ptr = &rhs.value();
                    rhs.reset();
                    storage.convert_ref_init_val(*their_ptr);
                }
            } else {
                if (storage.has_value()) {
                    T *our_ptr = storage.ptr;
                    storage.reset();
                    rhs.emplace(*our_ptr);
                }
            }
        }
#endif

        constexpr void swap(option &rhs) noexcept {
            std::ranges::swap(storage.ptr, rhs.storage.ptr);
        }

        // https://eel.is/c++draft/optional.ref.iterators#itemdecl:1
        // inherited from option_lref_iterator_base<T>

        // https://eel.is/c++draft/optional.ref.iterators#itemdecl:2
        constexpr auto begin() const noexcept -> T *
            requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
        {
            if (is_some()) {
                return storage.ptr;
            }
            return nullptr;
        }

        // https://eel.is/c++draft/optional.ref.iterators#itemdecl:3
        constexpr auto end() const noexcept -> T *
            requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
        {
            return begin() + has_value();
        }

        // https://eel.is/c++draft/optional.ref.observe#itemdecl:1
        constexpr T *operator->() const noexcept {
            return storage.ptr;
        }

        // https://eel.is/c++draft/optional.ref.observe#itemdecl:2
        constexpr T &operator*() const noexcept {
            return storage.get();
        }

        // https://eel.is/c++draft/optional.ref.observe#itemdecl:3
        constexpr explicit operator bool() const noexcept {
            return is_some();
        }

        // https://eel.is/c++draft/optional.ref.observe#itemdecl:4
        constexpr bool has_value() const noexcept {
            return is_some();
        }

        // https://eel.is/c++draft/optional.ref.observe#itemdecl:5
        constexpr T &value() const {
            return is_some() ? storage.get() : throw std::bad_optional_access();
        }

        // https://eel.is/c++draft/optional.ref.observe#itemdecl:6
        template <class U = std::remove_cv_t<T>>
        constexpr std::decay_t<T> value_or(U &&u) const
            requires (!std::is_array_v<T>) && std::is_object_v<T>
        {
            static_assert(std::is_constructible_v<std::remove_cv_t<T>, T &>
                          && std::is_convertible_v<U, std::remove_cv_t<T>>);
            return is_some() ? storage.get() : static_cast<std::decay_t<T>>(std::forward<U>(u));
        }

        // https://eel.is/c++draft/optional.ref.monadic#itemdecl:1
        template <class F>
        constexpr auto and_then(F &&f) const {
            using U = std::invoke_result_t<F, T &>;
            static_assert(detail::specialization_of<std::remove_cvref_t<U>, std::optional>
                          || detail::specialization_of<std::remove_cvref_t<U>, option>);

            if (is_some()) {
                auto result = std::invoke(std::forward<F>(f), storage.get());
                if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                    // Convert std::optional to option for consistency
                    using value_type = std::remove_cvref_t<U>::value_type;
                    return option<value_type>{ std::move(result) };
                } else {
                    return result;
                }
            }

            if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                // Convert std::optional to option for consistency
                using value_type = std::remove_cvref_t<U>::value_type;
                return option<value_type>{};
            } else {
                return std::remove_cvref_t<U>();
            }
        }

        // https://eel.is/c++draft/optional.ref.monadic#itemdecl:2
        template <class F>
        constexpr auto transform(F &&f) const {
            using U = std::remove_cv_t<std::invoke_result_t<F, T &>>;

            if (is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f), storage.get());
                    return option<void>{ true };
                } else {
                    return option<U>{ detail::within_invoke, std::forward<F>(f), storage.get() };
                }
            }
            return option<U>{};
        }

        // https://eel.is/c++draft/optional.ref.monadic#itemdecl:3
        template <std::invocable F>
        constexpr option or_else(F &&f) const {
            using U = std::invoke_result_t<F>;
            static_assert(detail::specialization_of<std::remove_cvref_t<U>, std::optional>
                          || std::is_same_v<std::remove_cvref_t<U>, option>);

            if (is_some()) {
                return storage.get();
            }

            if constexpr (detail::specialization_of<std::remove_cvref_t<U>, std::optional>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{ std::forward<F>(f)() };
            } else {
                return std::forward<F>(f)();
            }
        }

        // https://eel.is/c++draft/optional.mod#lib:reset,optional
        constexpr void reset() noexcept {
            storage.reset();
        }

        force_inline constexpr auto and_(this auto &&self, const option &optb) -> option {
            return self.is_some() ? optb : option{};
        }

        force_inline constexpr auto and_(this auto &&self, option &&optb) -> option {
            return self.is_some() ? std::move(optb) : option{};
        }

        template <detail::option_like U>
            requires (!std::same_as<std::remove_cvref_t<U>, option>)
        force_inline constexpr auto and_(this auto &&self, U &&optb) -> option {
            return self.is_some() ? option(std::forward<U>(optb)) : option{};
        }

        constexpr auto as_deref() const
            requires requires(const T &v) {
                { *v } -> detail::not_void;
            }
        {
            return as_ref().map([](const auto &self) -> const auto & { return *self; });
        }

        constexpr auto as_deref_mut()
            requires requires(T &v) {
                { *v } -> detail::not_void;
            }
        {
            return as_mut().map([](auto &&self) -> decltype(auto) { return *self; });
        }

        // Converts from `option<T> &` to `option<T&>`.
        // (Creates an option holding a reference to the original value.)
        constexpr auto as_mut() noexcept -> option<T &> {
            if (is_some()) {
                return option<T &>{ storage.get() };
            }
            return option<T &>{};
        }

        // Converts from `option<T&> &` to `option<const T&>`.
        // (Creates an option holding a const reference to the original value.)
        constexpr auto as_ref() const noexcept -> option<const T &> {
            if (is_some()) {
                return option<const T &>{ storage.get() };
            }
            return option<const T &>{};
        }

        // Maps an `option<(const) T&>` to an `option<T>` by cloning the contents of the
        // option.
        constexpr auto cloned(this auto &&self) noexcept(
            detail::noexcept_cloneable<std::remove_cvref_t<decltype(self.storage.get())>>)
            requires detail::cloneable<std::remove_cvref_t<decltype(self.storage.get())>>
        {
            if (self.is_some()) {
                return option<std::remove_cvref_t<decltype(self.storage.get())>>{ detail::clone(self.storage.get()) };
            }
            return option<std::remove_cvref_t<decltype(self.storage.get())>>{};
        }

        // Maps an `option<T&>` to an `option<T>` by copying the contents of the option.
        constexpr auto copied() const noexcept(std::is_nothrow_copy_constructible_v<std::remove_reference_t<T>>)
            requires std::copy_constructible<std::remove_reference_t<T>>
        {
            if (is_some()) {
                return option<std::remove_cvref_t<T>>{ storage.get() };
            }
            return option<std::remove_cvref_t<T>>{};
        }

        constexpr auto expect(const char *msg) const -> T & {
            if (is_none()) {
                throw option_panic(msg);
            }
            return storage.get();
        }

        template <typename F>
        constexpr auto filter(F &&predicate) const -> option<T &>
            requires std::predicate<F, T &>
        {
            if (is_some() && std::invoke(std::forward<F>(predicate), storage.get())) {
                return option<T &>{ storage.get() };
            }
            return option<T &>{};
        }

        template <typename U>
        constexpr auto get_or_insert(U &&value) -> T &
            requires std::convertible_to<U, T &>
        {
            if (is_none()) {
                storage.convert_ref_init_val(std::forward<U>(value));
            }
            return storage.get();
        }

        template <typename U>
        constexpr auto insert(U &&value) -> T &
            requires std::convertible_to<U, T &>
        {
            storage.convert_ref_init_val(std::forward<U>(value));
            return storage.get();
        }

        template <typename Self, typename F>
        constexpr auto inspect(this Self &&self, F &&f) -> Self && {
            if (self.is_some()) [[likely]] {
                std::invoke(std::forward<F>(f), self.storage.get());
            }
            return std::forward<Self>(self);
        }

        constexpr auto is_none() const noexcept -> bool {
            return !is_some();
        }

        template <typename F>
        constexpr auto is_none_or(F &&predicate) const -> bool
            requires std::predicate<F, T &>
        {
            return is_none() || std::invoke(std::forward<F>(predicate), storage.get());
        }

        constexpr auto is_some() const noexcept -> bool {
            return storage.has_value();
        }

        template <typename F>
        constexpr auto is_some_and(F &&predicate) const -> bool
            requires std::predicate<F, T &>
        {
            return is_some() && std::invoke(std::forward<F>(predicate), storage.get());
        }

        template <typename F>
        constexpr auto map(F &&f) const -> option<std::remove_cv_t<std::invoke_result_t<F, T &>>> {
            using U = std::remove_cv_t<std::invoke_result_t<F, T &>>;

            if (is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f), storage.get());
                    return option<void>{ true };
                } else {
                    return option<U>{ detail::within_invoke, std::forward<F>(f), storage.get() };
                }
            }

            return option<U>{};
        }

        template <typename F, typename U, typename R = std::invoke_result_t<F, T &>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<R> && std::is_lvalue_reference_v<U>,
                                                    std::common_reference_t<R, U>, std::remove_cvref_t<R>>>
        constexpr Ret map_or(U &&default_value, F &&f) const
            requires std::invocable<F, T &>
                  && (!std::is_lvalue_reference_v<R> || std::is_lvalue_reference_v<U>)
                  && std::convertible_to<R, Ret>
                  && std::convertible_to<U, Ret>
        {
            if (is_some()) {
                return std::invoke(std::forward<F>(f), storage.get());
            }
            return std::forward<U>(default_value);
        }

        template <typename D, typename F, typename RF = std::invoke_result_t<F, T &>,
                  typename RD = std::invoke_result_t<D>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<RF> && std::is_lvalue_reference_v<RD>,
                                                    std::common_reference_t<RF, RD>, std::remove_cvref_t<RF>>>
            requires std::invocable<D>
        constexpr Ret map_or_else(D &&default_fn, F &&f) const
            requires std::invocable<F, T &>
                  && (!std::is_lvalue_reference_v<RF> || std::is_lvalue_reference_v<RD>)
                  && std::convertible_to<RF, Ret>
                  && std::convertible_to<RD, Ret>
        {
            if (is_some()) {
                return std::invoke(std::forward<F>(f), storage.get());
            }
            return std::invoke(std::forward<D>(default_fn));
        }

        template <typename F>
        constexpr auto map_or_default(F &&f) const -> std::invoke_result_t<F, T &>
            requires std::invocable<F, T &> && std::default_initializable<std::invoke_result_t<F, T &>>
        {
            using U = std::invoke_result_t<F, T &>;
            if (is_some()) {
                return std::invoke(std::forward<F>(f), storage.get());
            }
            return U{};
        }

        template <typename E>
        constexpr auto ok_or(E &&err) const -> std::expected<T &, std::remove_cvref_t<E>>
            requires std::constructible_from<std::remove_cvref_t<E>, E>
        {
            using result_type = std::expected<T &, std::remove_cvref_t<E>>;
            if (is_some()) {
                return result_type{ storage.get() };
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        template <std::invocable F>
        constexpr auto ok_or_else(F &&f) const -> std::expected<T &, std::remove_cvref_t<std::invoke_result_t<F>>>
            requires std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F>>, std::invoke_result_t<F>>
        {
            using result_type = std::expected<T &, std::remove_cvref_t<std::invoke_result_t<F>>>;
            if (is_some()) {
                return result_type{ storage.get() };
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(f)) };
        }

        template <detail::option_like U>
        constexpr auto or_(U &&optb) const -> option<T &> {
            if (is_some()) {
                return option<T &>{ storage.get() };
            }
            return option(std::forward<U>(optb));
        }

        template <typename U>
        constexpr auto replace(U &&new_ref) -> option<T &>
            requires std::convertible_to<U, T &>
        {
            auto old = take();
            storage.convert_ref_init_val(std::forward<U>(new_ref));
            return old;
        }

        constexpr auto take() -> option<T &> {
            if (is_some()) {
                auto result = option<T &>{ storage.get() };
                reset();
                return result;
            }
            return option<T &>{};
        }

        template <typename F>
        constexpr auto take_if(F &&predicate) -> option<T &>
            requires std::predicate<F, T &>
        {
            if (is_some() && std::invoke(std::forward<F>(predicate), storage.get())) {
                return take();
            }
            return option<T &>{};
        }

        constexpr auto unwrap() const -> T & {
            if (is_none()) {
                throw option_panic("called `option::unwrap()` on a `none` value");
            }
            return storage.get();
        }

        template <typename U = std::remove_cv_t<T>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<U>, std::common_reference_t<T &, U>,
                                                    std::remove_cvref_t<T>>>
        constexpr Ret unwrap_or(U &&default_value) const
            requires std::convertible_to<T &, Ret> && std::convertible_to<U, Ret>
        {
            if (is_some()) [[likely]] {
                return storage.get();
            }
            return std::forward<U>(default_value);
        }

        constexpr auto unwrap_or_default() const = delete;

        template <std::invocable F, typename RF = std::invoke_result_t<F>,
                  typename Ret = std::conditional_t<std::is_lvalue_reference_v<RF>, std::common_reference_t<T &, RF>,
                                                    std::remove_cvref_t<T>>>
        constexpr Ret unwrap_or_else(F &&f) const
            requires std::convertible_to<T &, Ret> && std::convertible_to<RF, Ret>
        {
            if (is_some()) {
                return storage.get();
            }
            return std::invoke(std::forward<F>(f));
        }

        constexpr auto unwrap_unchecked() const noexcept -> T & {
            return storage.get();
        }

        force_inline constexpr auto xor_(this auto &&self, const option &optb) -> option {
            const bool self_some = self.is_some();
            const bool optb_some = optb.is_some();
            if (self_some != optb_some) {
                return self_some ? self : optb;
            }
            return option{};
        }

        force_inline constexpr auto xor_(this auto &&self, option &&optb) -> option {
            const bool self_some = self.is_some();
            const bool optb_some = optb.is_some();
            if (self_some != optb_some) {
                return self_some ? self : std::move(optb);
            }
            return option{};
        }

        template <typename U>
            requires detail::option_like<U> && (!std::same_as<std::remove_cvref_t<U>, option>)
        force_inline constexpr auto xor_(this auto &&self, U &&optb) -> option {
            auto optb_option = option(std::forward<U>(optb));
            const bool self_some = self.is_some();
            const bool optb_some = optb_option.is_some();
            if (self_some != optb_some) {
                return self_some ? self : optb_option;
            }
            return option{};
        }

        template <detail::option_type U>
        constexpr auto zip(U &&other) const
            requires (std::is_lvalue_reference_v<U>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
        {
            using pair_type = std::pair<T &, typename std::remove_cvref_t<U>::value_type>;
            if (is_some() && other.is_some()) {
                return option<pair_type>{ std::in_place, storage.get(), *std::forward<U>(other) };
            }
            return option<pair_type>{};
        }

        template <detail::option_type U, typename F>
        constexpr auto zip_with(U &&other, F &&f) const
            requires (std::is_lvalue_reference_v<U>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
                  && std::invocable<F, T &, decltype(*std::forward<U>(other))>
        {
            using result_type = std::remove_cv_t<std::invoke_result_t<F, T &, decltype(*std::forward<U>(other))>>;
            if (is_some() && other.is_some()) {
                return option<result_type>{ std::invoke(std::forward<F>(f), storage.get(), *std::forward<U>(other)) };
            }
            return option<result_type>{};
        }

        static constexpr auto default_() noexcept -> option {
            return option{};
        }

        constexpr auto cmp(const option &other) const noexcept(requires(const option &lhs, const option &rhs) {
            { lhs <=> rhs } noexcept;
        })
            requires std::three_way_comparable<T>
        {
            return *this <=> other;
        }

        constexpr auto(max)(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self > other)))
            -> option
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self > other ? self : other;
            }
            return self.is_some() ? self : other;
        }

        constexpr auto(max)(this auto &&self, option &&other) noexcept(noexcept(static_cast<bool>(self > other)))
            -> option
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self > other ? self : std::move(other);
            }
            return self.is_some() ? self : std::move(other);
        }

        constexpr auto(min)(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self < other)))
            -> option
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self < other ? self : other;
            }
            return self.is_none() ? self : other;
        }

        constexpr auto(min)(this auto &&self, option &&other) noexcept(noexcept(static_cast<bool>(self < other)))
            -> option
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self < other ? self : std::move(other);
            }
            return self.is_none() ? self : std::move(other);
        }

        constexpr auto clamp(this auto &&self, const option &min,
                             const option &max) noexcept(noexcept(static_cast<bool>(self < min))
                                                         && noexcept(static_cast<bool>(self > max))) -> option
            requires std::three_way_comparable<T>
        {
            if (self.is_some()) {
                if (min.is_some() && self < min) {
                    return min;
                }
                if (max.is_some() && self > max) {
                    return max;
                }
                return self;
            }
            return option{};
        }

        constexpr auto clamp(this auto &&self, option &&min,
                             option &&max) noexcept(noexcept(static_cast<bool>(self < min))
                                                    && noexcept(static_cast<bool>(self > max))) -> option
            requires std::three_way_comparable<T>
        {
            if (self.is_some()) {
                if (min.is_some() && self < min) {
                    return min;
                }
                if (max.is_some() && self > max) {
                    return max;
                }
                return self;
            }
            return option{};
        }

        constexpr auto eq(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self == other)))
            -> bool
            requires std::equality_comparable<T>
        {
            return self == other;
        }

        constexpr auto ne(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self != other)))
            -> bool
            requires std::equality_comparable<T>
        {
            return self != other;
        }

        constexpr auto partial_cmp(this auto &&self, const option &other) noexcept(noexcept(self.cmp(other)))
            -> decltype(self.cmp(other))
            requires std::three_way_comparable<T>
        {
            return self.cmp(other);
        }

        constexpr auto lt(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self < other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self < other;
        }

        constexpr auto le(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self <= other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self <= other;
        }

        constexpr auto gt(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self > other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self > other;
        }

        constexpr auto ge(this auto &&self, const option &other) noexcept(noexcept(static_cast<bool>(self >= other)))
            -> bool
            requires std::three_way_comparable<T>
        {
            return self >= other;
        }

        // Converts from `option<T> &` to `option<T&>`.
        template <typename U>
        static constexpr auto from(option<U> &other)
            requires std::convertible_to<U &, T>
        {
            if (other.is_some()) {
                return option<T>{ *other };
            }
            return option<T>{};
        }

        // Converts from `const option<T> &` to `option<const T&>`.
        template <typename U>
        static constexpr auto from(const option<U> &other)
            requires std::convertible_to<const U &, T>
        {
            if (other.is_some()) {
                return option<T>{ *other };
            }
            return option<T>{};
        }
    };
} // namespace opt

#pragma pop_macro("force_inline")
#pragma pop_macro("hot_path")
#pragma pop_macro("cpp20_no_unique_address")
#pragma pop_macro("has_cpp_lib_optional_ref")
