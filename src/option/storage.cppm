export module option:storage;

import std;
import :fwd;

#pragma push_macro("cpp20_no_unique_address")
#undef cpp20_no_unique_address
#if defined(_MSC_VER)
    #define cpp20_no_unique_address [[msvc::no_unique_address]]
#else
    #define cpp20_no_unique_address [[no_unique_address]]
#endif

export namespace opt::detail {
    struct empty_byte {};

    template <typename T>
    struct option_storage {
        using stored_type = std::remove_cv_t<T>;

        union {
            empty_byte empty;
            stored_type value;
        };
        bool has_value_ = false;

        constexpr option_storage() noexcept : empty{} {}

        constexpr option_storage(const T &val) noexcept(std::is_nothrow_copy_constructible_v<stored_type>) :
            value{ val }, has_value_{ true } {}

        constexpr option_storage(T &&val) noexcept(std::is_nothrow_move_constructible_v<stored_type>) :
            value{ std::move(val) }, has_value_{ true } {}

        constexpr option_storage(const option_storage &other) noexcept(
            std::is_nothrow_copy_constructible_v<stored_type>)
            requires std::is_copy_constructible_v<stored_type>
                  && (!std::is_trivially_copy_constructible_v<stored_type>)
            : has_value_{ other.has_value_ } {
            if (has_value_) {
                std::construct_at(std::addressof(value), other.value);
            }
        }

        constexpr option_storage(const option_storage &other) noexcept
            requires std::is_trivially_copy_constructible_v<stored_type>
        = default;

        constexpr option_storage(option_storage &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::move_constructible<T> && (!std::is_trivially_move_constructible_v<T>)
            : has_value_{ other.has_value_ } {
            if (has_value_) {
                if constexpr (std::is_const_v<T>) {
                    std::construct_at(std::addressof(value), std::forward<T>(other.value));
                } else {
                    std::construct_at(std::addressof(value), std::move(other.value));
                }
            }
        }

        constexpr option_storage(option_storage &&other) noexcept
            requires std::is_trivially_move_constructible_v<T>
        = default;

        template <class... Ts>
        constexpr option_storage(std::in_place_t,
                                 Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) :
            value(std::forward<Ts>(args)...), has_value_{ true } {}

        template <class F, class... Ts>
        constexpr option_storage(within_invoke_t, F &&f, Ts &&...args) noexcept(
            std::is_nothrow_constructible_v<T,
                                            decltype(std::invoke(std::forward<F>(f), std::forward<Ts>(args)...))>) :
            value{ std::invoke(std::forward<F>(f), std::forward<Ts>(args)...) }, has_value_{ true } {}

        constexpr option_storage &operator=(const option_storage &other)
            requires (std::is_trivially_copy_assignable_v<T> && std::is_trivially_copy_constructible_v<T>)
        = default;

        constexpr option_storage &operator=(const option_storage &other) noexcept(
            std::is_nothrow_copy_assignable_v<T>
            && (std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_destructible_v<T>))
            requires (!(std::is_trivially_copy_assignable_v<T> && std::is_trivially_copy_constructible_v<T>)
                      && (std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>))
        {
            if (this == &other) {
                return *this;
            }

            if (has_value_ && other.has_value_) {
                if constexpr (std::is_copy_assignable_v<T>) {
                    value = other.value;
                } else {
                    std::destroy_at(std::addressof(value));
                    std::construct_at(std::addressof(value), other.value);
                }
            } else if (has_value_ && !other.has_value_) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    std::destroy_at(std::addressof(value));
                }
                has_value_ = false;
            } else if (!has_value_ && other.has_value_) {
                static_assert(std::is_copy_constructible_v<T>);
                std::construct_at(std::addressof(value), other.value);
                has_value_ = true;
            }

            return *this;
        }

        constexpr option_storage &operator=(option_storage &&other)
            requires (std::is_trivially_move_assignable_v<T> && std::is_trivially_move_constructible_v<T>)
        = default;

        constexpr option_storage &operator=(option_storage &&other) noexcept(
            std::is_nothrow_move_assignable_v<T>
            && (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>))
            requires (!(std::is_trivially_move_assignable_v<T> && std::is_trivially_move_constructible_v<T>)
                      && (std::is_move_constructible_v<T> || std::is_move_assignable_v<T>))
        {
            if (this == &other) {
                return *this;
            }

            if (has_value_ && other.has_value_) {
                if constexpr (std::is_move_assignable_v<T>) {
                    value = std::move(other.value);
                } else {
                    std::destroy_at(std::addressof(value));
                    std::construct_at(std::addressof(value), std::move(other.value));
                }
            } else if (has_value_ && !other.has_value_) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    std::destroy_at(std::addressof(value));
                }
                has_value_ = false;
            } else if (!has_value_ && other.has_value_) {
                static_assert(std::is_move_constructible_v<T>);
                std::construct_at(std::addressof(value), std::move(other.value));
                has_value_ = true;
            }

            return *this;
        }

        constexpr ~option_storage() noexcept
            requires (!std::is_trivially_destructible_v<T>)
        {
            if (has_value_) {
                std::destroy_at(std::addressof(value));
            }
        }

        constexpr ~option_storage() noexcept
            requires std::is_trivially_destructible_v<T>
        = default;

        constexpr void reset() noexcept {
            if constexpr (std::is_trivially_destructible_v<T>) {
                has_value_ = false;
            } else {
                if (has_value_) {
                    std::destroy_at(std::addressof(value));
                    has_value_ = false;
                }
            }
        }

        constexpr auto &&get(this auto &&self) noexcept {
            if constexpr (std::is_const_v<T>) {
                return std::as_const(std::forward_like<decltype(self)>(self.value));
            } else {
                return std::forward_like<decltype(self)>(self.value);
            }
        }

        constexpr bool has_value() const noexcept {
            return has_value_;
        }

        template <typename... Ts>
        constexpr void emplace(Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                if (has_value_) {
                    std::destroy_at(std::addressof(value));
                    has_value_ = false;
                }
            }
            std::construct_at(std::addressof(value), std::forward<Ts>(args)...);
            has_value_ = true;
        }

        template <typename U, typename... Ts>
        constexpr void emplace(std::initializer_list<U> il, Ts &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Ts...>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                if (has_value_) {
                    std::destroy_at(std::addressof(value));
                    has_value_ = false;
                }
            }
            std::construct_at(std::addressof(value), il, std::forward<Ts>(args)...);
            has_value_ = true;
        }
    };

    template <typename T>
    struct option_storage<T *> {
        using pointer_t = T *;
        pointer_t ptr   = nullptr;
        bool has_value_ = false;

        constexpr option_storage() noexcept = default;
        constexpr option_storage(pointer_t val) noexcept : ptr{ val }, has_value_{ true } {}

        constexpr option_storage(const option_storage &other)            = default;
        constexpr option_storage(option_storage &&other)                 = default;
        constexpr option_storage &operator=(const option_storage &other) = default;
        constexpr option_storage &operator=(option_storage &&other)      = default;

        constexpr option_storage(std::in_place_t, pointer_t val) noexcept : ptr{ val }, has_value_{ true } {}

        constexpr auto &&get(this auto &&self) noexcept {
            return self.ptr;
        }

        constexpr bool has_value() const noexcept {
            return has_value_;
        }
        constexpr void reset() noexcept {
            ptr        = nullptr;
            has_value_ = false;
        }

        template <typename... Ts>
        constexpr void emplace(Ts &&...args) noexcept(std::is_nothrow_constructible_v<pointer_t, Ts...>) {
            ptr        = pointer_t(std::forward<Ts>(args)...);
            has_value_ = true;
        }
    };

    template <typename T>
    struct option_storage<T &> {
        T *ptr = nullptr;

        constexpr option_storage() noexcept = default;
        constexpr option_storage(T &val) noexcept : ptr{ &val } {}

        constexpr option_storage(const option_storage &other)            = default;
        constexpr option_storage(option_storage &&other)                 = default;
        constexpr option_storage &operator=(const option_storage &other) = default;
        constexpr option_storage &operator=(option_storage &&other)      = default;

        constexpr T &get() const noexcept {
            return *ptr;
        }

        constexpr bool has_value() const noexcept {
            return ptr != nullptr;
        }
        constexpr void reset() noexcept {
            ptr = nullptr;
        }

        // https://eel.is/c++draft/optional.optional.ref#optional.ref.expos-itemdecl:1
        template <typename U>
        constexpr void convert_ref_init_val(U &&u) {
            T &r = std::forward<U>(u);
            ptr  = std::addressof(r);
        }

        template <typename U>
        constexpr void emplace(U &&value) noexcept
            requires std::is_constructible_v<T, U> && (!detail::cpp23_reference_constructs_from_temporary_v<T, U>)
        {
            convert_ref_init_val(std::forward<U>(value));
        }
    };

    template <>
    struct option_storage<void> {
        bool has_value_ = false;

        constexpr option_storage() = default;
        constexpr option_storage(bool has_val) noexcept : has_value_{ has_val } {}

        constexpr void get() const noexcept {}
        constexpr bool has_value() const noexcept {
            return has_value_;
        }
        constexpr void reset() noexcept {
            has_value_ = false;
        }

        constexpr void emplace() noexcept {
            has_value_ = true;
        }
    };

    template <>
    struct option_storage<const void> : option_storage<void> {
        using option_storage<void>::option_storage;
    };

    template <>
    struct option_storage<volatile void> : option_storage<void> {
        using option_storage<void>::option_storage;
    };

    template <>
    struct option_storage<const volatile void> : option_storage<void> {
        using option_storage<void>::option_storage;
    };

    template <typename T>
        requires std::is_empty_v<T> && std::is_trivial_v<T> && std::is_default_constructible_v<T>
    struct option_storage<T> {
        using stored_type = std::remove_cv_t<T>;
        cpp20_no_unique_address stored_type value;
        bool has_value_ = false;

        constexpr option_storage() noexcept = default;

        constexpr option_storage(const option_storage &)
            requires std::is_copy_constructible_v<T>
        = default;

        constexpr option_storage(option_storage &&)
            requires std::is_move_constructible_v<T>
        = default;

        constexpr option_storage(const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) :
            value{ val }, has_value_{ true } {}

        constexpr option_storage(T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) :
            value{ std::move(val) }, has_value_{ true } {}

        template <class... Ts>
        constexpr option_storage(std::in_place_t,
                                 Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) :
            value(std::forward<Ts>(args)...), has_value_{ true } {}

        template <class F, class... Ts>
        constexpr option_storage(within_invoke_t, F &&f, Ts &&...args) noexcept(
            std::is_nothrow_constructible_v<T,
                                            decltype(std::invoke(std::forward<F>(f), std::forward<Ts>(args)...))>) :
            value{ std::invoke(std::forward<F>(f), std::forward<Ts>(args)...) }, has_value_{ true } {}

        constexpr option_storage &operator=(const option_storage &)
            requires std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>
        = default;

        constexpr option_storage &operator=(option_storage &&)
            requires std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
        = default;

        constexpr auto &&get(this auto &&self) noexcept {
            return self.value;
        }

        constexpr bool has_value() const noexcept {
            return has_value_;
        }

        constexpr void reset() noexcept {
            has_value_ = false;
        }

        template <typename... Ts>
        constexpr void emplace(Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) {
            std::construct_at(std::addressof(value), std::forward<Ts>(args)...);
            has_value_ = true;
        }

        template <typename U, typename... Ts>
        constexpr void emplace(std::initializer_list<U> il, Ts &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Ts...>) {
            value      = T{ il, std::forward<Ts>(args)... };
            has_value_ = true;
        }
    };
} // namespace opt::detail

#pragma pop_macro("cpp20_no_unique_address")
