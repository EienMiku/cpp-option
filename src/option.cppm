export module option;

import std;

export namespace opt {
    struct none_t;

    namespace detail {
        template <typename T>
        concept option_prohibited_type = (std::same_as<std::remove_cvref_t<T>, none_t>)
                                      || (std::same_as<std::remove_cvref_t<T>, std::nullopt_t>)
                                      || (std::same_as<std::remove_cvref_t<T>, std::in_place_t>)
                                      || (std::is_array_v<T>)
                                      || std::is_rvalue_reference_v<T>;
    } // namespace detail

    template <typename T>
        requires (!detail::option_prohibited_type<T>)
    class option;

    namespace detail {
        template <template <typename...> typename Template, typename T>
        inline constexpr bool is_specialization_of_v = false;

        template <template <typename...> class Template, typename... Ts>
        inline constexpr bool is_specialization_of_v<Template, Template<Ts...>> = true;

        template <template <typename...> class Template, typename T>
        concept specialization_of = is_specialization_of_v<Template, T>;

        template <typename T>
        concept expected_type = specialization_of<std::expected, std::remove_cv_t<T>>;

        template <typename T>
        concept pair_type = specialization_of<std::pair, std::remove_cvref_t<T>>;

        template <typename T>
        concept option_type = specialization_of<option, std::remove_cvref_t<T>>;

        template <typename T>
        concept option_like = option_type<T>
                           || std::same_as<std::remove_cvref_t<T>, none_t>
                           || std::same_as<std::remove_cvref_t<T>, std::nullopt_t>;

        template <typename T>
        concept has_clone = requires(const T &t) {
            { t.clone() } -> std::convertible_to<T>;
        };

        template <typename T>
        concept cloneable = has_clone<T> || std::is_trivially_copyable_v<T>;

        template <typename T>
        concept noexcept_cloneable = requires(const T &v) {
            { v.clone() } noexcept;
        } || (std::is_trivially_copyable_v<T> && std::is_nothrow_copy_constructible_v<T>);

        template <cloneable T>
        constexpr T clone_value(const T &value) noexcept(noexcept_cloneable<T>) {
            if constexpr (has_clone<T>) {
                return value.clone();
            } else {
                static_assert(
                    std::is_trivially_copyable_v<T>,
                    "Type must either have a .clone() member function or be trivially copyable, because it is "
                    "impossible to determine whether a user-defined copy constructor performs a deep copy.");
                return value;
            }
        }

        // https://eel.is/c++draft/optional.ctor#1
        template <typename T, typename W>
        concept converts_from_any_cvref = std::constructible_from<T, W &>
                                       || std::convertible_to<W &, T>
                                       || std::constructible_from<T, W>
                                       || std::convertible_to<W, T>
                                       || std::constructible_from<T, const W &>
                                       || std::convertible_to<const W &, T>
                                       || std::constructible_from<T, const W>
                                       || std::convertible_to<const W, T>;

#if defined(__cpp_lib_reference_from_temporary) && __cpp_lib_reference_from_temporary >= 202202L
        template <class T, class U>
        inline constexpr bool cpp23_reference_constructs_from_temporary_v =
            std::reference_constructs_from_temporary_v<T, U>;
#elif defined(__clang__) && __has_builtin(__reference_constructs_from_temporary)
        template <class T, class U>
        inline constexpr bool cpp23_reference_constructs_from_temporary_v = __reference_constructs_from_temporary(T, U);
#else
        // Fallback
        template <class T, class U>
        inline constexpr bool cpp23_reference_constructs_from_temporary_v = false;
#endif

#if defined(__clang__) || defined (__GNUC__)
        #define force_inline [[gnu::always_inline]]
#else
        #define force_inline [[msvc::forceinline]]
#endif

#if defined(_MSC_VER)
        #define cpp20_no_unique_address [[msvc::no_unique_address]]
#else
        #define cpp20_no_unique_address [[no_unique_address]]
#endif
    } // namespace detail

    struct none_t {
        template <detail::option_type T>
        constexpr auto and_(T &&) const noexcept {
            return T{};
        }

        template <detail::option_type T>
        constexpr auto or_(T &&optb) const noexcept {
            if (optb.is_some()) {
                return std::forward<decltype(optb)>(optb);
            }
            return T{};
        }

        template <detail::option_type T>
        constexpr auto xor_(T &&optb) const noexcept {
            if (optb.is_some()) {
                return std::forward<decltype(optb)>(optb);
            }
            return T{};
        }

        constexpr auto eq(const none_t &) const noexcept {
            return true;
        }

        template <typename T>
        constexpr auto eq(const option<T> &other) const noexcept {
            return *this == other;
        }

        constexpr auto ne(const none_t &) const noexcept {
            return false;
        }

        template <typename T>
        constexpr auto ne(const option<T> &other) const noexcept {
            return *this != other;
        }

        constexpr auto lt(const none_t &) const noexcept {
            return false;
        }

        template <typename T>
        constexpr auto lt(const option<T> &other) const noexcept {
            return *this < other;
        }

        constexpr auto le(const none_t &) const noexcept {
            return true;
        }

        template <typename T>
        constexpr auto le(const option<T> &other) const noexcept {
            return *this <= other;
        }

        constexpr auto gt(const none_t &) const noexcept {
            return false;
        }

        template <typename T>
        constexpr auto gt(const option<T> &other) const noexcept {
            return *this > other;
        }

        constexpr auto ge(const none_t &) const noexcept {
            return true;
        }

        template <typename T>
        constexpr auto ge(const option<T> &other) const noexcept {
            return *this >= other;
        }
    };

    class option_panic : public std::exception {
    public:
        option_panic(const char *message) noexcept : message{ message } {}

        const char *what() const noexcept override {
            return message;
        }

    private:
        const char *message;
    };

    namespace detail {
        struct empty_byte {};
        
        template <typename T>
        struct option_storage {
            union {
                empty_byte empty;
                T value;
            };
            bool has_value_ = false;

            constexpr option_storage() noexcept : empty{} {}

            constexpr option_storage(const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) :
                value{ val }, has_value_{ true } {}

            constexpr option_storage(T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) :
                value{ std::move(val) }, has_value_{ true } {}

            constexpr option_storage(const option_storage &other) noexcept(std::is_nothrow_copy_constructible_v<T>)
                requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>)
                : has_value_{ other.has_value_ } {
                if (has_value_) {
                    std::construct_at(&value, other.value);
                }
            }

            constexpr option_storage(const option_storage &other) noexcept
                requires std::is_trivially_copy_constructible_v<T>
            = default;

            constexpr option_storage(option_storage &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
                requires std::move_constructible<T> && (!std::is_trivially_move_constructible_v<T>)
                : has_value_{ other.has_value_ } {
                if (has_value_) {
                    std::construct_at(&value, std::move(other.value));
                    other.has_value_ = false;
                }
            }

            constexpr option_storage(option_storage &&other) noexcept
                requires std::is_trivially_move_constructible_v<T>
            = default;

            constexpr option_storage &operator=(const option_storage &other) noexcept(
                std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_destructible_v<T>)
                requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>)
            {
                if (this != &other) {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        if (has_value_) {
                            std::destroy_at(&value);
                            has_value_ = false;
                        }
                    }
                    if (other.has_value_) {
                        std::construct_at(&value, other.value);
                        has_value_ = true;
                    }
                }
                return *this;
            }

            constexpr option_storage &operator=(const option_storage &other) noexcept
                requires std::is_trivially_copy_constructible_v<T>
            = default;

            constexpr option_storage &operator=(option_storage &&other) noexcept(std::is_nothrow_move_constructible_v<T>
                                                                                    && std::is_nothrow_destructible_v<T>)
                requires std::move_constructible<T> && (!std::is_trivially_move_constructible_v<T>)
            {
                if (this != &other) {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        if (has_value_) {
                            std::destroy_at(&value);
                            has_value_ = false;
                        }
                    }
                    if (other.has_value_) {
                        std::construct_at(&value, std::move(other.value));
                        has_value_ = true;
                        other.has_value_ = false;
                    }
                }
                return *this;
            }

            constexpr option_storage &operator=(option_storage &&other) noexcept
                requires std::is_trivially_move_constructible_v<T>
            = default;

            constexpr ~option_storage() noexcept
                requires (!std::is_trivially_destructible_v<T>)
            {
                if (has_value_) {
                    std::destroy_at(&value);
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
                        std::destroy_at(&value);
                        has_value_ = false;
                    }
                }
            }

            constexpr auto &&get(this auto &&self) noexcept {
                return self.value;
            }

            constexpr bool has_value() const noexcept {
                return has_value_;
            }

            template <typename... Ts>
            constexpr void emplace(Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    if (has_value_) {
                        std::destroy_at(&value);
                        has_value_ = false;
                    }
                }
                std::construct_at(&value, std::forward<Ts>(args)...);
                has_value_ = true;
            }

            template <typename U, typename... Ts>
            constexpr void emplace(std::initializer_list<U> il, Ts &&...args) noexcept(
                std::is_nothrow_constructible_v<T, std::initializer_list<U>, Ts...>) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    if (has_value_) {
                        std::destroy_at(&value);
                        has_value_ = false;
                    }
                }
                std::construct_at(&value, il, std::forward<Ts>(args)...);
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
            requires std::is_empty_v<T> && std::is_trivially_constructible_v<T>
        struct option_storage<T> {
            cpp20_no_unique_address T value;
            bool has_value_ = false;

            constexpr option_storage() noexcept = default;

            constexpr option_storage(const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) :
                value{ val }, has_value_{ true } {}

            constexpr option_storage(T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) :
                value{ std::move(val) }, has_value_{ true } {}

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
                value      = T{ std::forward<Ts>(args)... };
                has_value_ = true;
            }

            template <typename U, typename... Ts>
            constexpr void emplace(std::initializer_list<U> il, Ts &&...args) noexcept(
                std::is_nothrow_constructible_v<T, std::initializer_list<U>, Ts...>) {
                value      = T{ il, std::forward<Ts>(args)... };
                has_value_ = true;
            }
        };
    } // namespace detail

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
        constexpr option(option &&)      = default;

        constexpr option &operator=(std::nullopt_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option &operator=(none_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option &operator=(const option &) = default;
        constexpr option &operator=(option &&)      = default;

        constexpr void emplace() noexcept {
            storage.emplace();
        }

        constexpr void swap(option &rhs) noexcept {
            std::swap(storage, rhs.storage);
        }

        // iterator

        constexpr auto operator->(this auto &&) noexcept  = delete;
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

            static_assert(detail::option_type<U> || detail::specialization_of<std::optional, std::remove_cvref_t<U>>);

            if (is_some()) {
                auto result = std::invoke(std::forward<F>(f));
                if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                    // Convert std::optional to option for consistency
                    using value_type = typename std::remove_cvref_t<U>::value_type;
                    return option<value_type>{ std::move(result) };
                } else {
                    return result;
                }
            }

            if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{};
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        template <std::invocable F>
        constexpr auto transform(F &&f) const {
            return map(std::forward<F>(f));
        }

        template <std::invocable F>
        constexpr auto or_else(F &&f) const
            requires (detail::option_type<std::invoke_result_t<F>>
                      || detail::specialization_of<std::optional, std::remove_cvref_t<std::invoke_result_t<F>>>)
        {
            using U = std::invoke_result_t<F>;

            if (is_some()) {
                return *this;
            }

            auto result = std::invoke(std::forward<F>(f));
            if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
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
        constexpr auto and_(U &&optb) const noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<U>>) {
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

        constexpr auto expect(const char *msg) const {
            if (is_none()) {
                throw option_panic(msg);
            }
        }

        template <std::predicate F>
        constexpr auto filter(F &&f) const {
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

        constexpr auto &insert() {
            return *this;
        }

        template <std::invocable F>
        constexpr auto inspect(F &&f) const {
            if (is_some()) {
                std::invoke(std::forward<F>(f));
            }
            return *this;
        }

        constexpr bool is_none() const noexcept {
            return !is_some();
        }

        template <std::predicate F>
        constexpr auto is_none_or(F &&f) const {
            return is_none() || std::invoke(std::forward<F>(f));
        }

        constexpr bool is_some() const noexcept {
            return storage.has_value();
        }

        template <std::predicate F>
        constexpr auto is_some_and(F &&f) const {
            return is_some() && std::invoke(std::forward<F>(f));
        }

        template <std::invocable F>
        constexpr auto map(F &&f) const {
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

        template <std::invocable F, typename U>
        constexpr auto map_or(U &&default_value, F &&f) const {
            if (is_some()) {
                return std::invoke(std::forward<F>(f));
            }
            return std::forward<U>(default_value);
        }

        template <std::invocable D, std::invocable F>
        constexpr auto map_or_else(D &&default_f, F &&f) const
            requires std::same_as<std::invoke_result_t<F>, std::invoke_result_t<D>>
        {
            if (is_some()) {
                return std::invoke(std::forward<F>(f));
            }
            return std::invoke(std::forward<D>(default_f));
        }

        template <typename E>
        constexpr auto ok_or(E &&err) const {
            using result_type = std::expected<value_type, std::remove_cvref_t<E>>;
            if (is_some()) {
                return result_type{};
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        template <std::invocable F>
        constexpr auto ok_or_else(F &&err) const {
            using result_type = std::expected<value_type, std::invoke_result_t<F>>;
            if (is_some()) {
                return result_type{};
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(err)) };
        }

        template <detail::option_like U>
        constexpr auto or_(U &&optb) const {
            if (is_some()) {
                return *this;
            }
            return option(std::forward<U>(optb));
        }

        constexpr auto replace() const {
            return *this;
        }

        constexpr auto take() {
            option result{};
            if (is_some()) {
                std::ranges::swap(storage, result.storage);
            }
            return result;
        }

        template <typename F>
        constexpr auto take_if(F &&f) {
            option result{};
            if (is_some() && std::invoke(std::forward<F>(f))) {
                std::ranges::swap(storage, result.storage);
            }
            return result;
        }

        constexpr auto unwrap() const {
            if (is_none()) {
                throw option_panic("Attempted to access value of empty option");
            }
        }

        constexpr auto unwrap_or() const {}

        constexpr auto unwrap_or_default() const {}

        template <std::invocable F>
        constexpr auto unwrap_or_else(F &&f) const {
            if (is_some()) {
                return *this;
            }
            return std::invoke(std::forward<F>(f));
        }

        constexpr auto unwrap_unchecked() const {}

        template <detail::option_like U>
            requires (!detail::option_type<U>) || std::same_as<typename U::value_type, value_type>
        constexpr auto xor_(U &&optb) const {
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
        constexpr auto zip(auto &&) const               = delete;
        constexpr auto zip_with(auto &&, auto &&) const = delete;

        static constexpr auto default_() noexcept {
            return option{};
        }

        static constexpr auto from(auto &&) = delete;

        constexpr auto cmp(const option &other) const {
            if (is_some() && other.is_some()) {
                return std::strong_ordering::equal;
            } else {
                return is_some() <=> other.is_some();
            }
        }

        constexpr auto max(const option &other) const noexcept {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_some() ? *this : other;
        }

        constexpr auto max(option &&other) const noexcept {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_some() ? *this : other;
        }

        constexpr auto min(const option &other) const noexcept {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_none() ? *this : other;
        }

        constexpr auto min(option &&other) const noexcept {
            if (is_some() && other.is_some()) {
                return *this;
            }
            return is_none() ? *this : other;
        }

        constexpr auto clamp(const option &, const option &) const noexcept {
            return *this;
        }

        constexpr auto clamp(option &&, option &&) const noexcept {
            return *this;
        }

        constexpr auto eq(const option &other) const noexcept {
            return is_some() == other.is_some();
        }

        constexpr auto ne(const option &other) const noexcept {
            return is_some() != other.is_some();
        }

        constexpr auto partial_cmp(const option &other) const noexcept {
            return cmp(other);
        }

        constexpr auto lt(const option &other) const noexcept {
            return is_none() && other.is_some();
        }

        constexpr auto le(const option &other) const noexcept {
            return is_none() || other.is_some();
        }

        constexpr auto gt(const option &other) const noexcept {
            return is_some() && other.is_none();
        }

        constexpr auto ge(const option &other) const noexcept {
            return is_some() || other.is_none();
        }

        constexpr auto clone() const noexcept {
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
        requires (!detail::option_prohibited_type<T>)
    class option {
    private:
        detail::option_storage<T> storage;

    public:
        constexpr explicit operator std::optional<T>() const noexcept(std::is_nothrow_copy_constructible_v<T>) {
            if (is_some()) {
                return std::optional<T>{ storage.get() };
            }
            return std::nullopt;
        }

        // https://eel.is/c++draft/optional.optional.general
        using value_type     = T;
        using iterator       = T *;
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

        constexpr option(const std::optional<T> &rhs)
            requires (!std::is_copy_constructible_v<T>)
        = delete;

        constexpr option(const option &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires (!std::is_trivially_copy_constructible_v<T>)
        {
            if (rhs.is_some()) {
                storage.emplace(rhs.storage.get());
            }
        }

        constexpr option(const option &) noexcept(std::is_nothrow_copy_constructible_v<T>)
            requires std::is_trivially_copy_constructible_v<T>
        = default;

        constexpr option(const option &)
            requires (!std::copy_constructible<T>)
        = delete;

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

        constexpr option(option &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::is_move_constructible_v<T> && (!std::is_trivially_move_constructible_v<T>)
        {
            if (rhs.is_some()) {
                storage.emplace(std::move(rhs.storage.get()));
            }
        }

        constexpr option(option &&) noexcept(std::is_nothrow_move_constructible_v<T>)
            requires std::is_trivially_move_constructible_v<T>
        = default;

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor___
        template <class... Args>
        constexpr explicit option(std::in_place_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
            requires std::is_constructible_v<T, Args...>
            : storage{ T(std::forward<Args>(args)...) } {}

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor____
        template <class U, class... Args>
        constexpr explicit option(std::in_place_t, std::initializer_list<U> il, Args &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Args...>)
            requires std::is_constructible_v<T, std::initializer_list<U>, Args...>
            :
            storage{
                T{ il, std::forward<Args>(args)... }
        } {}

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor_____
        template <class U = std::remove_cv_t<T>>
        constexpr explicit(!std::convertible_to<U, T>) option(U &&v) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires std::is_constructible_v<T, U>
                  && (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, std::optional<T>>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, option>)
                  && ((!std::same_as<std::remove_cv_t<T>, bool>)
                      || !detail::specialization_of<std::optional, std::remove_cvref_t<U>>)
        {
            storage.emplace(std::forward<U>(v));
        }

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor______
        template <class U>
        constexpr explicit(!std::is_convertible_v<const U &, T>)
            option(const std::optional<U> &rhs) noexcept(std::is_nothrow_constructible_v<T, const U &>)
            requires std::is_constructible_v<T, const U &>
                  && (std::same_as<T, bool> || !detail::converts_from_any_cvref<T, std::optional<U>>)
        {
            if (rhs.has_value()) {
                storage.emplace(*rhs);
            }
        }

        template <typename U>
        constexpr explicit(!std::convertible_to<const U &, T>)
            option(const option<U> &rhs) noexcept(std::is_nothrow_constructible_v<T, const U &>)
            requires std::is_constructible_v<T, const U &>
                  && (std::same_as<T, bool> || !detail::converts_from_any_cvref<T, option<U>>)
        {
            if (rhs.is_some()) {
                storage.emplace(*rhs);
            }
        }

        // https://eel.is/c++draft/optional.ctor#lib:optional,constructor_______
        template <class U>
        constexpr explicit(!std::is_convertible_v<U, T>)
            option(std::optional<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires std::is_constructible_v<T, U>
                  && (std::same_as<T, bool> || !detail::converts_from_any_cvref<T, std::optional<U>>)
        {
            if (rhs.has_value()) {
                storage.emplace(*std::move(rhs));
            }
        }

        template <typename U>
        constexpr explicit(!std::convertible_to<U, T>)
            option(option<U> &&rhs) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires std::constructible_from<T, U>
                  && (std::same_as<T, bool> || !detail::converts_from_any_cvref<T, option<U>>)
        {
            if (rhs.is_some()) {
                storage.emplace(std::move(*rhs));
            }
        }

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional
        constexpr option &operator=(std::nullopt_t) noexcept {
            storage.reset();
            return *this;
        }

        constexpr option<T> &operator=(none_t) noexcept {
            storage.reset();
            return *this;
        }

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional_
        //
        // Unlike `std::optional`, `option` uses a different storage mechanism, making
        // this assignment non-trivial even if `T` is trivially copyable, trivially copy
        // assignable, and trivially destructible.
        // For a trivial assignment, consider using the `option` type directly.
        constexpr option &operator=(const std::optional<T> &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>
                                                                          && std::is_nothrow_destructible_v<T>) {
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

        constexpr option &operator=(const std::optional<T> &)
            requires ((!std::is_copy_constructible_v<T>) || (!std::is_copy_assignable_v<T>))
        = delete;

        constexpr option &operator=(const option &rhs) noexcept(std::is_nothrow_copy_constructible_v<T>
                                                                && std::is_nothrow_destructible_v<T>)
            requires (!(std::is_trivially_copy_constructible_v<T>
                        && std::is_trivially_copy_assignable_v<T>
                        && std::is_trivially_destructible_v<T>))
        {
            if (this == &rhs) {
                return *this;
            }

            if (rhs.has_value()) {
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

        constexpr option &operator=(const option &)
            requires std::is_trivially_copy_constructible_v<T>
                      && std::is_trivially_copy_assignable_v<T>
                      && std::is_trivially_destructible_v<T>
        = default;

        constexpr option &operator=(const option &)
            requires ((!std::is_copy_constructible_v<T>) || (!std::is_copy_assignable_v<T>))
        = delete;

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional__
        //
        // Unlike `std::optional`, `option` uses a different storage mechanism, making
        // this assignment non-trivial even if `T` is trivially move constructible,
        // trivially move assignable, and trivially destructible.
        // For a trivial assignment, consider using the `option` type directly.
        constexpr option &operator=(std::optional<T> &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>
                                                                     && std::is_nothrow_destructible_v<T>)
            requires std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
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

        constexpr option &operator=(option &&rhs) noexcept(std::is_nothrow_move_constructible_v<T>
                                                           && std::is_nothrow_destructible_v<T>)
            requires std::is_move_constructible_v<T>
                  && std::is_move_assignable_v<T>
                  && (!(std::is_trivially_move_constructible_v<T>
                        && std::is_trivially_move_assignable_v<T>
                        && std::is_trivially_destructible_v<T>))
        {
            if (rhs.is_some()) {
                if (storage.has_value()) {
                    storage.get() = std::move(rhs.storage.get());
                } else {
                    storage.emplace(std::move(rhs.storage.get()));
                }
            } else {
                if (storage.has_value()) {
                    storage.reset();
                }
            }
            return *this;
        }

        constexpr option &operator=(option &&)
            requires std::is_trivially_move_constructible_v<T>
                      && std::is_trivially_move_assignable_v<T>
                      && std::is_trivially_destructible_v<T>
        = default;

        // https://eel.is/c++draft/optional.assign#lib:operator=,optional___
        template <class U = std::remove_cv_t<T>>
        constexpr option &operator=(U &&v) noexcept(std::is_nothrow_constructible_v<T, U>
                                                    && std::is_nothrow_assignable_v<T &, U>)
            requires (!std::conjunction_v<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>>)
                  && std::is_constructible_v<T, U>
                  && std::is_assignable_v<T &, U>
                  && (!std::is_same_v<std::remove_cvref_t<U>, option>)
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
            requires std::is_constructible_v<T, const U &>
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
            requires std::is_constructible_v<T, const U &>
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
            requires std::is_constructible_v<T, U>
                  && std::is_assignable_v<T &, U>
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
            requires std::is_constructible_v<T, U>
                  && std::is_assignable_v<T &, U>
                  && (!detail::converts_from_any_cvref<T, option<U>>)
                  && (!std::is_assignable_v<T &, option<U> &>)
                  && (!std::is_assignable_v<T &, option<U> &&>)
                  && (!std::is_assignable_v<T &, const option<U> &>)
                  && (!std::is_assignable_v<T &, const option<U> &&>)
        {
            if (rhs.is_some()) {
                if (storage.has_value()) {
                    storage.get() = std::move(rhs.storage.get());
                } else {
                    storage.emplace(std::move(rhs.storage.get()));
                }
            } else {
                storage.reset();
            }
            return *this;
        }

        // https://eel.is/c++draft/optional.assign#lib:emplace,optional
        template <typename... Args>
        constexpr auto &emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
            static_assert(std::is_constructible_v<T, Args...>);
            reset();
            storage.emplace(std::forward<Args>(args)...);
            return storage.get();
        }

        // https://eel.is/c++draft/optional.assign#lib:emplace,optional_
        //
        // Standard-compliant `emplace` overload with initializer_list support.
        template <class U, class... Args>
        constexpr auto &emplace(std::initializer_list<U> il, Args &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Args &&...>)
            requires std::is_constructible_v<T, std::initializer_list<U> &, Args...>
        {
            reset();
            storage.emplace(il, std::forward<Args>(args)...);
            return storage.get();
        }

        // https://eel.is/c++draft/optional.swap#lib:swap,optional
        constexpr void swap(std::optional<T> &rhs) noexcept(std::is_nothrow_move_constructible_v<T>
                                                            && std::is_nothrow_swappable_v<T>)
            requires std::swappable<T>
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
            requires std::swappable<T>
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
        constexpr auto operator->(this auto &&self) noexcept {
            assert(self.has_value());
            return std::addressof(self.storage.get());
        }

        // https://eel.is/c++draft/optional.observe#lib:operator*,optional
        // https://eel.is/c++draft/optional.observe#lib:operator*,optional_
        constexpr auto &&operator*(this auto &&self) noexcept {
            assert(self.has_value());
            return self.storage.get();
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
        constexpr auto &&value(this auto &&self) {
            if (self.is_none()) {
                throw std::bad_optional_access{};
            }
            return self.storage.get();
        }

        // https://eel.is/c++draft/optional.observe#lib:value_or,optional
        // https://eel.is/c++draft/optional.observe#lib:value_or,optional_
        template <typename U = std::remove_cv_t<T>>
        constexpr auto value_or(this auto &&self, U &&v) {
            static_assert(std::is_copy_constructible_v<T> && std::is_convertible_v<U &&, T>);

            if (self.is_some()) {
                return std::forward_like<decltype(self)>(*self);
            }
            return static_cast<T>(std::forward<U>(v));
        }

        // https://eel.is/c++draft/optional.monadic#lib:and_then,optional
        // https://eel.is/c++draft/optional.monadic#lib:and_then,optional_
        template <typename F>
        constexpr auto and_then(this auto &&self, F &&f) {
            using U = std::invoke_result_t<F, decltype(std::forward_like<decltype(self)>(*self))>;

            static_assert(detail::option_type<U> || detail::specialization_of<std::optional, std::remove_cvref_t<U>>);

            if (self.is_some()) {
                auto result = std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(*self));
                if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                    // Convert std::optional to option for consistency
                    using value_type = typename std::remove_cvref_t<U>::value_type;
                    return option<value_type>{ std::move(result) };
                } else {
                    return result;
                }
            }

            if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{};
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        // https://eel.is/c++draft/optional.monadic#lib:transform,optional
        // https://eel.is/c++draft/optional.monadic#lib:transform,optional_
        template <class F>
        constexpr auto transform(this auto &&self, F &&f) {
            return self.map(std::forward<F>(f));
        }

        // https://eel.is/c++draft/optional.monadic#lib:or_else,optional
        // https://eel.is/c++draft/optional.monadic#lib:or_else,optional_
        template <std::invocable F>
        constexpr auto or_else(this auto &&self, F &&f)
            requires std::copy_constructible<T>
                  && (detail::option_type<std::invoke_result_t<F>>
                      || detail::specialization_of<std::optional, std::remove_cvref_t<std::invoke_result_t<F>>>)
        {
            using U = std::invoke_result_t<F>;

            if (self.is_some()) {
                return self;
            }

            auto result = std::forward<F>(f)();
            if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                // Convert std::optional to option for consistency
                using value_type = typename std::remove_cvref_t<U>::value_type;
                return option<value_type>{ std::move(result) };
            } else {
                return result;
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
        force_inline constexpr auto and_(this auto &&self, const option<T> &optb) {
            return self.is_some() ? optb : option{};
        }

        force_inline constexpr auto and_(this auto &&self, option<T> &&optb) {
            return self.is_some() ? std::move(optb) : option{};
        }
        
        template <detail::option_like U>
            requires (!std::same_as<std::remove_cvref_t<U>, option<T>>)
        force_inline constexpr auto and_(this auto &&self, U &&optb) {
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
            requires requires(T v) { *v; }
        {
            return as_ref().map([](const auto &self) -> const auto & { return *self; });
        }

        // Converts from `option<T>` to `option<U&>`, where `U` is the result of applying
        // the dereference operator to `T`.
        //
        // Leaves the original `option` in-place, returning a new one holding a reference
        // to the dereferenced value of the original.
        constexpr auto as_deref_mut()
            requires requires(T v) { *v; }
        {
            return as_mut().map([](auto &&self) -> decltype(auto) { return *self; });
        }

        // Converts from `option<T> &` to `option<T&>`.
        // (Creates an option holding a reference to the original value.)
        constexpr auto as_mut() noexcept {
            if (is_some()) {
                return option<T &>{ storage.get() };
            }
            return option<T &>{};
        }

        // Converts from `option<T> &` to `option<const T&>`.
        // (Creates an option holding a const reference to the original value.)
        constexpr auto as_ref() const noexcept {
            if (is_some()) {
                return option<const T &>{ storage.get() };
            }
            return option<const T &>{};
        }

        // Returns the contained value.
        //
        // Throws if the option is empty with a custom message provided by `msg`.
        constexpr auto expect(const char *msg) const {
            if (is_none()) {
                throw option_panic(msg);
            }
            return storage.get();
        }

        // Returns an empty option if the option is empty, otherwise calls `predicate`
        // with the wrapped value and returns:
        //
        // - `some(t)` if `predicate` returns `true` (where `t` is the wrapped value)
        // - `none` if `predicate` returns `false`
        template <typename F>
        constexpr auto filter(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            if (self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get())) {
                return std::remove_cvref_t<decltype(self)>{ self };
            }
            return std::remove_cvref_t<decltype(self)>{};
        }

        // Converts from `option<option<T>>` to `option<T>`.
        constexpr auto flatten(this auto &&self)
            requires detail::option_type<T>
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(*self);
            }
            return std::remove_cvref_t<T>{};
        }

        // Inserts `value` into the option if it is empty, then returns a reference
        // to the contained value.
        //
        // See also `option::insert`, which updates the value even if the option already
        // contains a value.
        template <typename U>
        constexpr auto &get_or_insert(this auto &&self, U &&value)
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            if (self.is_none()) {
                self.storage.emplace(std::forward_like<decltype(self)>(value));
            }
            return self.storage.get();
        }

        // Inserts the default value into the option if it is empty, then returns a
        // reference to the contained value.
        constexpr auto &get_or_insert_default()
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
        constexpr auto &get_or_insert_with(this auto &&self, F &&f)
            requires std::constructible_from<T, std::invoke_result_t<F>>
        {
            if (self.is_none()) {
                self.storage.emplace(std::invoke(std::forward<F>(f)));
            }
            return self.storage.get();
        }

        // Inserts `value` into the option, then returns a reference to it.
        //
        // If the option already contains a value, the old value is dropped.
        //
        // See also `option::get_or_insert`, which doesn't update the value if the option
        // already contains a value.
        template <typename U>
        constexpr auto &insert(this auto &&self, U &&value)
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            self.storage.reset();
            self.storage.emplace(std::forward_like<decltype(self)>(value));
            return self.storage.get();
        }

        // Calls a function with a reference to the contained value if the option contains
        // a value.
        //
        // Returns the original option.
        template <typename F>
        constexpr auto inspect(this auto &&self, F &&f) {
            if (self.is_some()) {
                std::invoke(std::forward<F>(f), self.storage.get());
            }
            return self;
        }

        // Returns `true` if the option is empty.
        constexpr bool is_none() const noexcept {
            return !is_some();
        }

        // Returns `true` if the option is empty or the value inside matches a predicate.
        template <typename F>
        constexpr auto is_none_or(this auto &&self, F &&f)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_none() || std::invoke(std::forward<F>(f), self.storage.get());
        }

        // Returns `true` if the option contains a value.
        constexpr bool is_some() const noexcept {
            return storage.has_value();
        }

        // Returns `true` if the option contains a value and the value inside matches a
        // predicate.
        template <typename F>
        constexpr auto is_some_and(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get());
        }

        // Maps an `option<T>` to `option<U>` by applying a function to a contained value
        // (if the option contains a value) or returns `none` (if the option is empty).
        template <typename F>
        constexpr auto map(this auto &&self, F &&f) {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(std::forward_like<decltype(self)>(*self))>>;

            if (self.is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(*self));
                    return option<void>{ true };
                } else {
                    return option<U>{ std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(*self)) };
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
        template <typename F, typename U>
        constexpr auto map_or(this auto &&self, U &&default_value, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward_like<decltype(self)>(self.unwrap_unchecked()))>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap_unchecked()));
            }
            return std::forward<U>(default_value);
        }

        // Maps an `option<T>` to a `U` by applying function `f` to the contained value if
        // the option contains a value, otherwise returns the default constructed value of
        // the type `U`.
        template <typename F>
        constexpr auto map_or_default(this auto &&self, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward_like<decltype(self)>(self.unwrap_unchecked()))>
        {
            using U = std::invoke_result_t<F, decltype(storage.get())>;
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap_unchecked()));
            }
            return U{};
        }

        // Computes a default function result (if the option is empty), or applies a
        // different function to the contained value (if any).
        template <std::invocable D, typename F>
        constexpr auto map_or_else(this auto &&self, D &&default_f, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward_like<decltype(self)>(self.unwrap_unchecked()))>
                  && std::same_as<
                         std::invoke_result_t<D>,
                         std::invoke_result_t<F, decltype(std::forward_like<decltype(self)>(self.unwrap_unchecked()))>>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap_unchecked()));
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
        template <typename E>
        constexpr auto ok_or(this auto &&self, E &&err)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::constructible_from<std::remove_cvref_t<E>, E>
        {
            using result_type = std::expected<T, std::remove_cvref_t<E>>;
            if (self.is_some()) {
                return result_type{ std::forward_like<decltype(self)>(self.unwrap_unchecked()) };
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        // Transforms the `option<T>` into a `std::expected<T, E>`, mapping `some(v)` to
        // `std::expected<T, E>` with `v` and `none` to `std::expected<T, E>` which with
        // the error returned by `err`.
        template <std::invocable F>
        constexpr auto ok_or_else(this auto &&self, F &&err)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F>>, std::invoke_result_t<F>>
        {
            using result_type = std::expected<T, std::invoke_result_t<F>>;
            if (self.is_some()) {
                return result_type{ std::forward_like<decltype(self)>(self.unwrap_unchecked()) };
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(err)) };
        }

        // Returns the option if it contains a value, otherwise returns `optb`.
        //
        // Arguments passed to `or_` are eagerly evaluated; if you are passing the result
        // of a function call, it is recommended to use `or_else`, which is lazily
        // evaluated.
        template <detail::option_like U>
        constexpr auto or_(this auto &&self, U &&optb) {
            if (self.is_some()) {
                return self;
            }
            return option(std::forward<U>(optb));
        }

        // Returns the option if it contains a value, otherwise calls `f` and returns
        // the result.
        // See above

        // Replaces the actual value in the option by the value given in parameter,
        // returning the old value if present, leaving a `some` in its place without
        // deinitializing either one.
        template <typename U>
        constexpr auto replace(this auto &&self, U &&value)
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            option<T> old{};
            if (self.is_some()) {
                std::ranges::swap(self.storage, old.storage);
            }

            self.storage.reset();
            self.storage.emplace(std::forward_like<decltype(self)>(value));

            return old;
        }

        // Takes the value out of the option, leaving an empty option in its place.
        constexpr auto take(this auto &&self) {
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
        constexpr auto take_if(this auto &&self, F &&f)
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
        constexpr auto &&unwrap(this auto &&self)
            requires (!std::same_as<std::remove_cv_t<T>, void>)
        {
            if (self.is_none()) {
                throw option_panic("Attempted to access value of empty option");
            }
            return self.storage.get();
        }

        // Returns the contained value or a provided default.
        //
        // Arguments passed to `unwrap_or` are eagerly evaluated; if you are passing the
        // result of a function call, it is recommended to use `unwrap_or_else`, which is
        // lazily evaluated.
        template <typename U = std::remove_cv_t<T>>
        constexpr auto unwrap_or(this auto &&self, U &&default_value)
            requires (std::is_lvalue_reference_v<decltype(self)>
                          ? (std::copy_constructible<T> && std::convertible_to<const U &, T>)
                          : (std::move_constructible<T> && std::convertible_to<U &&, T>))
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap_unchecked());
            }
            return static_cast<T>(std::forward<U>(default_value));
        }

        // Returns the contained Some value or a default.
        //
        // If the option contains a value, returns the contained value; otherwise, returns
        // the default value for that type.
        constexpr auto unwrap_or_default(this auto &&self)
            requires std::default_initializable<T>
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap_unchecked());
            }
            return T{};
        }

        // Returns the contained value or computes it from a function.
        template <std::invocable F>
        constexpr auto unwrap_or_else(this auto &&self, F &&f) -> std::common_type_t<T, std::invoke_result_t<F>>
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::convertible_to<std::invoke_result_t<F>, T>
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap_unchecked());
            }
            return std::invoke(std::forward<F>(f));
        }

        // Returns the contained value, without checking that the value is not empty.
        constexpr auto &&unwrap_unchecked(this auto &&self) noexcept {
            return self.storage.get();
        }

        // Unzips an option containing a pair of two values.
        //
        // If `self` is `some(std::pair{ a, b })` this function returns `std::pair{
        // some(a), some(b) }`. Otherwise, `std::pair{ none, none }` is returned.
        constexpr auto unzip(this auto &&self)
            requires detail::pair_type<typename std::remove_cvref_t<T>>
        {
            using first_type  = decltype(self.unwrap_unchecked().first);
            using second_type = decltype(self.unwrap_unchecked().second);

            if (self.is_some()) {
                auto &&pair = std::forward_like<decltype(self)>(self.unwrap_unchecked());
                return std::pair{ option<first_type>{ std::forward_like<decltype(pair)>(pair.first) },
                                  option<second_type>{ std::forward_like<decltype(pair)>(pair.second) } };
            }
            return std::pair{ option<first_type>{}, option<second_type>{} };
        }

        // Returns `some` if exactly one of `self`, `optb` is `some`, otherwise returns an
        // empty option.
        force_inline constexpr auto xor_(this auto &&self, const option<T> &optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
        {
            const bool self_some = self.is_some();
            const bool optb_some = optb.is_some();
            
            if (self_some != optb_some) {
                return self_some ? self : optb;
            }
            return option{};
        }
        
        force_inline constexpr auto xor_(this auto &&self, option<T> &&optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
        {
            const bool self_some = self.is_some();
            const bool optb_some = optb.is_some();
            
            if (self_some != optb_some) {
                return self_some ? self : std::move(optb);
            }
            return option{};
        }
        
        template <detail::option_like U>
            requires (!std::same_as<std::remove_cvref_t<U>, option<T>>) 
        force_inline constexpr auto xor_(this auto &&self, U &&optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
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
        template <detail::option_type U>
        constexpr auto zip(this auto &&self, U &&optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(optb)>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
        {
            if (self.is_some() && optb.is_some()) {
                return option<std::pair<T, typename std::remove_cvref_t<U>::value_type>>{
                    { std::forward_like<decltype(self)>(self.unwrap_unchecked()),
                     std::forward_like<decltype(optb)>(optb.unwrap_unchecked()) }
                };
            }
            return option<std::pair<T, typename std::remove_cvref_t<U>::value_type>>{};
        }

        // Zips `self` and another `option` with function `f`.
        //
        // If `self` is `some(s)` and `other` is `some(o)`, this function returns
        // `some(f(s, o))`. Otherwise, `none` is returned.
        template <detail::option_type U, typename F>
        constexpr auto zip_with(this auto &&self, U &&other, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(other)>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
                  && std::invocable<F, decltype(self.unwrap_unchecked()), decltype(other.unwrap_unchecked())>
        {
            using result_type =
                std::invoke_result_t<F, decltype(self.unwrap_unchecked()), decltype(other.unwrap_unchecked())>;
            if (self.is_some() && other.is_some()) {
                return option<result_type>{ std::invoke(std::forward<F>(f),
                                                        std::forward_like<decltype(self)>(self.unwrap_unchecked()),
                                                        std::forward_like<decltype(other)>(other.unwrap_unchecked())) };
            }
            return option<result_type>{};
        }

        constexpr auto clone(this auto &&self) noexcept(detail::noexcept_cloneable<T>)
            requires detail::cloneable<T>
        {
            if (self.is_some()) {
                return option{ detail::clone_value(self.unwrap_unchecked()) };
            }
            return option{};
        }

        constexpr void clone_from(this auto &&self, const option<T> &source) noexcept(detail::noexcept_cloneable<T>)
            requires detail::cloneable<T> && std::is_copy_assignable_v<T>
        {
            if (source.is_some()) {
                if (self.is_some()) {
                    if constexpr (detail::has_clone<T>) {
                        self.storage.get() = detail::clone_value(source.unwrap_unchecked());
                    } else {
                        self.storage.get() = source.unwrap_unchecked();
                    }
                } else {
                    self.emplace(detail::clone_value(source.unwrap_unchecked()));
                }
            } else {
                self.reset();
            }
        }

        static constexpr auto default_() noexcept {
            return option{};
        }

        // Creates a new option by moving or copying `value` into it.
        template <typename U>
        static constexpr auto from(U &&value)
            requires std::constructible_from<T, U>
        {
            if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
                return option<T>{ std::forward<U>(value) };
            }
            return option<T>{ value };
        }

        // Compares `self` with another `option` for ordering using three-way comparison.
        //
        // If both options contain values, compares the contained values using their `<=>` operator.
        // If one option is empty and the other contains a value, the empty option is considered less.
        // If both options are empty, they are considered equal.
        //
        // This function is equivalent to using the `<=>` operator directly on the options.
        constexpr auto cmp(const option &other) const
            noexcept(noexcept(std::declval<option<T>>() <=> std::declval<option<T>>()))
            requires std::three_way_comparable<T>
        {
            return *this <=> other;
        }

        // Returns the option with the maximum value.
        //
        // If both options contain values, returns the option with the greater value.
        // If only one option contains a value, returns that option.
        // If both are empty, returns an empty option.
        constexpr auto max(this auto &&self, const option<T> &other) noexcept(noexcept(self > other))
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self > other ? self : other;
            }
            return self.is_some() ? self : other;
        }

        constexpr auto max(this auto &&self, option<T> &&other) noexcept(noexcept(self > other))
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
        constexpr auto min(this auto &&self, const option<T> &other) noexcept(noexcept(self < other))
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self < other ? self : other;
            }
            return self.is_none() ? self : other;
        }

        constexpr auto min(this auto &&self, option<T> &&other) noexcept(noexcept(self < other))
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
                             const option<T> &max) noexcept(noexcept(self < min) && noexcept(self > max))
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
                             option<T> &&max) noexcept(noexcept(self < min) && noexcept(self > max))
            requires std::three_way_comparable<T>
        {
            if (self.is_some()) {
                if (min.is_some() && self < min) {
                    return std::move(min);
                }
                if (max.is_some() && self > max) {
                    return std::move(max);
                }
                return self;
            }
            return option<T>{};
        }

        // Tests for equality between two options.
        //
        // Returns `true` if both options are empty or both contain equal values.
        // This is equivalent to using the `==` operator.
        constexpr auto eq(this auto &&self, const option<T> &other) noexcept(noexcept(self == other))
            requires std::equality_comparable<T>
        {
            return self == other;
        }

        // Tests for inequality between two options.
        //
        // Returns `true` if one option is empty while the other contains a value,
        // or if both contain values that are not equal.
        // This is equivalent to using the `!=` operator.
        constexpr auto ne(this auto &&self, const option<T> &other) noexcept(noexcept(self != other))
            requires std::equality_comparable<T>
        {
            return self != other;
        }

        // Performs partial comparison between two options.
        //
        // Returns the result of three-way comparison.
        // This is equivalent to calling `cmp` function.
        constexpr auto partial_cmp(this auto &&self, const option<T> &other) noexcept(noexcept(self.cmp(other)))
            requires std::three_way_comparable<T>
        {
            return self.cmp(other);
        }

        // Tests if this option is less than another option.
        //
        // Returns `true` if this option is less than the other option.
        // Empty options are considered less than any non-empty option.
        // This is equivalent to using the `<` operator.
        constexpr auto lt(this auto &&self, const option<T> &other) noexcept(noexcept(self < other))
            requires std::three_way_comparable<T>
        {
            return self < other;
        }

        // Tests if this option is less than or equal to another option.
        //
        // Returns `true` if this option is less than or equal to the other option.
        // Empty options are considered less than any non-empty option.
        // This is equivalent to using the `<=` operator.
        constexpr auto le(this auto &&self, const option<T> &other) noexcept(noexcept(self <= other))
            requires std::three_way_comparable<T>
        {
            return self <= other;
        }

        // Tests if this option is greater than another option.
        //
        // Returns `true` if this option is greater than the other option.
        // Non-empty options are considered greater than any empty option.
        // This is equivalent to using the `>` operator.
        constexpr auto gt(this auto &&self, const option<T> &other) noexcept(noexcept(self > other))
            requires std::three_way_comparable<T>
        {
            return self > other;
        }

        // Tests if this option is greater than or equal to another option.
        //
        // Returns `true` if this option is greater than or equal to the other option.
        // Non-empty options are considered greater than any empty option.
        // This is equivalent to using the `>=` operator.
        constexpr auto ge(this auto &&self, const option<T> &other) noexcept(noexcept(self >= other))
            requires std::three_way_comparable<T>
        {
            return self >= other;
        }

        constexpr auto as_mut_slice() = delete;
        constexpr auto as_pin_mut()   = delete;
        constexpr auto as_pin_ref()   = delete;
        constexpr auto as_slice()     = delete;
        auto iter()                   = delete;
        auto iter_mut()               = delete;
        static auto from_iter()       = delete;
        static auto from_residual()   = delete;
        auto into_iter()              = delete;
    };

    template <typename T>
    class option<T &> {
    private:
        detail::option_storage<T &> storage;

    public:
        // https://eel.is/c++draft/optional.optional.ref.general
        using value_type = T;

        constexpr option() noexcept = default;
        constexpr option(std::nullopt_t) noexcept : option() {}
        constexpr option(none_t) noexcept : option() {}
        template <class U = T>
        constexpr option(const std::optional<U &> &rhs) noexcept {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*rhs);
            }
        }
        constexpr option(const option &rhs) noexcept = default;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:1
        template <class Arg>
        constexpr explicit option(std::in_place_t, Arg &&arg)
            requires std::is_constructible_v<T &, Arg>
                  && (!detail::cpp23_reference_constructs_from_temporary_v<T &, Arg>)
        {
            storage.convert_ref_init_val(std::forward<Arg>(arg));
        }

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:2
        template <class U>
        constexpr explicit(!std::convertible_to<U, T &>) option(U &&u) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires (!std::is_same_v<std::remove_cvref_t<U>, std::optional<T>>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, option>)
                  && (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                  && std::is_constructible_v<T &, U>
        {
            storage.convert_ref_init_val(std::forward<U>(u));
        }

        template <class U>
        constexpr explicit(!std::convertible_to<U, T &>) option(U &&u) noexcept(std::is_nothrow_constructible_v<T &, U>)
            requires (!std::is_same_v<std::remove_cvref_t<U>, std::optional<T>>)
                      && (!std::is_same_v<std::remove_cvref_t<U>, option>)
                      && (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>)
                      && std::is_constructible_v<T &, U>
                      && detail::cpp23_reference_constructs_from_temporary_v<T &, U>
        = delete;

        // https://eel.is/c++draft/optional.ref.ctor#itemdecl:3
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

        constexpr option &operator=(const std::optional<T> &rhs) noexcept {
            if (rhs.has_value()) {
                storage.convert_ref_init_val(*rhs);
            } else {
                storage.reset();
            }
            return *this;
        }
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
        constexpr auto &emplace(U &&u) noexcept(std::is_nothrow_constructible_v<T, U>)
            requires std::is_constructible_v<T, U> && (!detail::cpp23_reference_constructs_from_temporary_v<T &, U>)
        {
            reset();
            storage.convert_ref_init_val(std::forward<U>(u));
            return storage.get();
        }

        // https://eel.is/c++draft/optional.ref.swap
        constexpr void swap(std::optional<T> &rhs) noexcept {
            if (rhs.has_value()) {
                if (storage.has_value()) {
                    T *ptr = storage.ptr;
                    storage.reset();
                    storage.convert_ref_init_val(std::addressof(&rhs.value()));
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

        constexpr void swap(option &rhs) noexcept {
            std::ranges::swap(storage.ptr, rhs.storage.ptr);
        }

        // https://eel.is/c++draft/optional.ref.iterators#itemdecl:1
        using iterator = T *;

        // https://eel.is/c++draft/optional.ref.iterators#itemdecl:2
        constexpr iterator begin() const noexcept {
            if (is_some()) {
                return storage.ptr;
            }
            return nullptr;
        }

        // https://eel.is/c++draft/optional.ref.iterators#itemdecl:3
        constexpr iterator end() const noexcept {
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
        constexpr std::remove_cv_t<T> value_or(U &&u) const {
            static_assert(std::is_constructible_v<std::remove_cv_t<T>, T &>
                          && std::is_convertible_v<U, std::remove_cv_t<T>>);
            return is_some() ? storage.get() : static_cast<std::remove_cv_t<T>>(std::forward<U>(u));
        }

        // https://eel.is/c++draft/optional.ref.monadic#itemdecl:1
        template <class F>
        constexpr auto and_then(F &&f) const {
            using U = std::invoke_result_t<F, T &>;
            static_assert(detail::specialization_of<std::optional, std::remove_cvref_t<U>>
                          || detail::specialization_of<option, std::remove_cvref_t<U>>);

            if (is_some()) {
                auto result = std::invoke(std::forward<F>(f), storage.get());
                if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                    // Convert std::optional to option for consistency
                    using value_type = std::remove_cvref_t<U>::value_type;
                    return option<value_type>{ std::move(result) };
                } else {
                    return result;
                }
            }

            if constexpr (detail::specialization_of<std::optional, std::remove_cvref_t<U>>) {
                // Convert std::optional to option for consistency
                using value_type = std::remove_cvref_t<U>::value_type;
                return option<value_type>{};
            } else {
                return std::remove_cvref_t<U>();
            }
        }

        // https://eel.is/c++draft/optional.ref.monadic#itemdecl:2
        template <class F>
        constexpr option<std::remove_cv_t<std::invoke_result_t<F, T &>>> transform(F &&f) const {
            using U = std::remove_cv_t<std::invoke_result_t<F, T &>>;

            if (is_some()) {
                auto &&result = std::invoke(std::forward<F>(f), storage.get());
                return option<U>{ std::forward<decltype(result)>(result) };
            }
            return option<U>();
        }

        // https://eel.is/c++draft/optional.ref.monadic#itemdecl:3
        template <std::invocable F>
        constexpr option or_else(F &&f) const {
            static_assert(std::is_same_v<std::remove_cvref_t<std::invoke_result_t<F>>, option>);

            if (is_some()) {
                return storage.get();
            }
            return std::forward<F>(f)();
        }

        // https://eel.is/c++draft/optional.mod#lib:reset,optional
        constexpr void reset() noexcept {
            storage.reset();
        }

        template <detail::option_like U>
        constexpr auto and_(U &&optb) const {
            if (is_some()) {
                return option(std::forward<U>(optb));
            }
            return option{};
        }

        constexpr auto as_deref() const
            requires std::is_pointer_v<std::remove_reference_t<T>>
        {
            if (is_some()) {
                using pointee_type = std::remove_pointer_t<std::remove_reference_t<T>>;
                return option<const pointee_type &>{ *storage.get() };
            }
            using pointee_type = std::remove_pointer_t<std::remove_reference_t<T>>;
            return option<const pointee_type &>{};
        }

        constexpr auto as_deref_mut()
            requires std::is_pointer_v<std::remove_reference_t<T>>
        {
            if (is_some()) {
                using pointee_type = std::remove_pointer_t<std::remove_reference_t<T>>;
                return option<pointee_type &>{ *storage.get() };
            }
            using pointee_type = std::remove_pointer_t<std::remove_reference_t<T>>;
            return option<pointee_type &>{};
        }

        // Converts from `option<T&>` to `option<T&>`.
        // (Creates an option holding a reference to the original value.)
        constexpr auto as_mut() noexcept {
            if (is_some()) {
                return option<T &>{ storage.get() };
            }
            return option<T &>{};
        }

        // Converts from `option<T&> &` to `option<const T&>`.
        // (Creates an option holding a const reference to the original value.)
        constexpr auto as_ref() const noexcept {
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
                return option<std::remove_cvref_t<decltype(self.storage.get())>>{ detail::clone_value(
                    self.storage.get()) };
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

        constexpr auto expect(const char *msg) const {
            if (is_none()) {
                throw option_panic(msg);
            }
            return storage.get();
        }

        template <typename F>
        constexpr auto filter(F &&predicate) const
            requires std::predicate<F, T &>
        {
            if (is_some() && std::invoke(std::forward<F>(predicate), storage.get())) {
                return option<T &>{ storage.get() };
            }
            return option<T &>{};
        }

        template <typename U>
        constexpr auto &get_or_insert(U &&value)
            requires std::convertible_to<U, T &>
        {
            if (is_none()) {
                storage.convert_ref_init_val(std::forward<U>(value));
            }
            return storage.get();
        }

        template <typename U>
        constexpr auto &insert(U &&value)
            requires std::convertible_to<U, T &>
        {
            storage.convert_ref_init_val(std::forward<U>(value));
            return storage.get();
        }

        template <typename F>
        constexpr auto inspect(F &&f) const {
            if (is_some()) {
                std::invoke(std::forward<F>(f), storage.get());
            }
            return *this;
        }

        constexpr bool is_none() const noexcept {
            return !is_some();
        }

        template <typename F>
        constexpr bool is_none_or(F &&predicate) const
            requires std::predicate<F, T &>
        {
            return is_none() || std::invoke(std::forward<F>(predicate), storage.get());
        }

        constexpr bool is_some() const noexcept {
            return storage.has_value();
        }

        template <typename F>
        constexpr bool is_some_and(F &&predicate) const
            requires std::predicate<F, T &>
        {
            return is_some() && std::invoke(std::forward<F>(predicate), storage.get());
        }

        template <typename F>
        constexpr auto map(F &&f) const {
            using U = std::remove_cv_t<std::invoke_result_t<F, T &>>;

            if (is_some()) {
                if constexpr (std::same_as<U, void>) {
                    std::invoke(std::forward<F>(f), storage.get());
                    return option<void>{ true };
                } else {
                    return option<U>{ std::invoke(std::forward<F>(f), storage.get()) };
                }
            }

            if constexpr (std::same_as<U, void>) {
                return option<void>{};
            } else {
                return option<U>{};
            }
        }

        template <typename F, typename U>
        constexpr auto map_or(U &&default_value, F &&f) const {
            using R = std::invoke_result_t<F, T &>;
            if (is_some()) {
                return static_cast<R>(std::invoke(std::forward<F>(f), storage.get()));
            }
            return static_cast<R>(std::forward<U>(default_value));
        }

        template <std::invocable D, typename F>
        constexpr auto map_or_else(D &&default_fn, F &&f) const {
            using R = std::invoke_result_t<F, T &>;
            if (is_some()) {
                return static_cast<R>(std::invoke(std::forward<F>(f), storage.get()));
            }
            return static_cast<R>(std::invoke(std::forward<D>(default_fn)));
        }

        template <typename E>
        constexpr auto ok_or(E &&err) const {
            if (is_some()) {
                return std::expected<T &, std::remove_cvref_t<E>>{ storage.get() };
            }
            return std::expected<T &, std::remove_cvref_t<E>>{ std::unexpect, std::forward<E>(err) };
        }

        template <std::invocable F>
        constexpr auto ok_or_else(F &&f) const {
            using E = std::invoke_result_t<F>;
            if (is_some()) {
                return std::expected<T &, E>{ storage.get() };
            }
            return std::expected<T &, E>{ std::unexpect, std::invoke(std::forward<F>(f)) };
        }

        template <detail::option_like U>
        constexpr auto or_(U &&optb) const {
            if (is_some()) {
                return option<T &>{ storage.get() };
            }
            // 依赖转换运算符
            return option(std::forward<U>(optb));
        }

        template <typename U>
        constexpr auto replace(U &&new_ref)
            requires std::convertible_to<U, T &>
        {
            auto old = take();
            storage.convert_ref_init_val(std::forward<U>(new_ref));
            return old;
        }

        constexpr auto take() {
            if (is_some()) {
                auto result = option<T &>{ storage.get() };
                reset();
                return result;
            }
            return option<T &>{};
        }

        template <typename F>
        constexpr auto take_if(F &&predicate)
            requires std::predicate<F, T &>
        {
            if (is_some() && std::invoke(std::forward<F>(predicate), storage.get())) {
                return take();
            }
            return option<T &>{};
        }

        constexpr auto &&unwrap() const {
            if (is_none()) {
                throw option_panic("called `option::unwrap()` on a `none` value");
            }
            return storage.get();
        }

        template <class U = std::remove_cv_t<T>>
        constexpr std::remove_cv_t<T> unwrap_or(U &&u) const {
            static_assert(std::is_constructible_v<std::remove_cv_t<T>, T &>
                          && std::is_convertible_v<U, std::remove_cv_t<T>>);
            return is_some() ? storage.get() : static_cast<std::remove_cv_t<T>>(std::forward<U>(u));
        }

        constexpr auto unwrap_or_default() const
            requires std::default_initializable<std::remove_reference_t<T>>
        {
            if (is_some()) {
                return storage.get();
            } else {
                static std::remove_reference_t<T> default_value{};
                return static_cast<T &>(default_value);
            }
        }

        template <std::invocable F>
        constexpr auto unwrap_or_else(F &&f) const {
            if (is_some()) {
                return storage.get();
            }
            return std::invoke(std::forward<F>(f));
        }

        constexpr auto &&unwrap_unchecked() const noexcept {
            return storage.get();
        }

        template <detail::option_like U>
        constexpr auto xor_(this auto &&self, U &&optb) {
            auto optb_reformed = option(std::forward<U>(optb));
            if (self.is_some() && optb_reformed.is_none()) {
                return self;
            }
            if (self.is_none() && optb_reformed.is_some()) {
                return optb_reformed;
            }
            return option{};
        }

        template <detail::option_type U>
        constexpr auto zip(U &&other) const {
            if (is_some() && other.is_some()) {
                return option<std::pair<T &, decltype(other.unwrap_unchecked())>>{ std::in_place, storage.get(),
                                                                                   other.unwrap_unchecked() };
            }
            return option<std::pair<T &, decltype(other.unwrap_unchecked())>>{};
        }

        template <detail::option_type U, typename F>
        constexpr auto zip_with(U &&other, F &&f) const {
            using R = std::invoke_result_t<F, T &, decltype(other.unwrap_unchecked())>;
            if (is_some() && other.is_some()) {
                return option<R>{ std::in_place,
                                  std::invoke(std::forward<F>(f), storage.get(), other.unwrap_unchecked()) };
            }
            return option<R>{};
        }

        // Converts from `option<T> &` to `option<T&>`.
        template <typename U>
        static constexpr auto from(option<U> &other)
            requires std::convertible_to<U &, T>
        {
            if (other.is_some()) {
                return option<T>{ other.unwrap_unchecked() };
            }
            return option<T>{};
        }

        // Converts from `const option<T> &` to `option<const T&>`.
        template <typename U>
        static constexpr auto from(const option<U> &other)
            requires std::convertible_to<const U &, T>
        {
            if (other.is_some()) {
                return option<T>{ other.unwrap_unchecked() };
            }
            return option<T>{};
        }
    };

} // namespace opt

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
    // https://eel.is/c++draft/optional.relops#lib:operator!=,optional
    template <class T, class U>
    constexpr bool operator==(const option<T> &x, const std::optional<U> &y)
        requires requires {
            { x.unwrap_unchecked() == *y } -> std::convertible_to<bool>;
        }
    {
        if (x.is_some() != y.has_value()) {
            return false;
        } else if (x.is_some() == false) {
            return true;
        } else {
            return x.unwrap_unchecked() == *y;
        }
    }

    template <typename T, class U>
    constexpr bool operator==(const option<T> &x,
                              const option<U> &y) noexcept(noexcept(std::declval<T>() == std::declval<U>()))
        requires std::equality_comparable_with<T, U>
    {
        if (x.is_some() != y.is_some()) {
            return false;
        } else if (x.is_some() == false) {
            return true;
        } else {
            return *x == *y;
        }
    }

    template <typename T, typename U>
        requires std::same_as<std::remove_cv_t<T>, void> && std::same_as<std::remove_cv_t<U>, void>
    constexpr bool operator==(const option<T> &x, const option<U> &y) noexcept {
        if (x.is_some() != y.is_some()) {
            return false;
        }
        return true;
    }

    // https://eel.is/c++draft/optional.relops#lib:operator%3c=%3e,optional
    // https://eel.is/c++draft/optional.relops#lib:operator%3c,optional
    // https://eel.is/c++draft/optional.relops#lib:operator%3e,optional
    // https://eel.is/c++draft/optional.relops#lib:operator%3c=,optional
    // https://eel.is/c++draft/optional.relops#lib:operator%3c=,optional
    template <class T, std::three_way_comparable_with<T> U>
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const option<T> &x, const std::optional<U> &y) {
        if (x && y) {
            return x.unwrap_unchecked() <=> *y;
        } else {
            return x.is_some() <=> y.has_value();
        }
    }

    template <class T, std::three_way_comparable_with<T> U>
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const option<T> &x, const option<U> &y) {
        if (x && y) {
            return x.unwrap_unchecked() <=> y.unwrap_unchecked();
        } else {
            return x.is_some() <=> y.is_some();
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
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator==,optional_
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator!=,optional
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator!=,optional_
    template <class T, class U>
    constexpr bool operator==(const option<T> &x, const U &v)
        requires (!detail::specialization_of<std::optional, U>) && (!detail::specialization_of<option, U>) && requires {
            { *x == v } -> std::convertible_to<bool>;
        }
    {
        return x.is_some() ? *x == v : false;
    }

    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e=,optional_
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c,optional
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c,optional_
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e,optional
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e,optional_
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c=,optional
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3c=,optional_
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e=,optional
    // https://eel.is/c++draft/optional.comp.with.t#lib:operator%3e=,optional_
    template <class T, class U>
        requires (!detail::is_derived_from_optional<U>) && std::three_way_comparable_with<T, U>
    constexpr std::compare_three_way_result_t<T, U> operator<=>(const option<T> &x, const U &v);

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
        requires (!detail::specialization_of<std::reference_wrapper, std::decay_t<T>>)
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
        requires detail::specialization_of<std::reference_wrapper, T>
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
        return option<T>{
            std::in_place, T{ il, std::forward<Ts>(args)... }
        };
    }

    constexpr option<void> some() noexcept {
        return option<void>{ true };
    }

    template <typename T>
    constexpr option<T> none_opt() noexcept {
        return option<T>{};
    }

    template <typename T>
        requires (!detail::specialization_of<std::optional, T>)
    option(T) -> option<T>;

    template <typename T>
    option(std::reference_wrapper<T>) -> option<T &>;

    template <typename T>
        requires detail::specialization_of<std::optional, std::remove_cvref_t<T>>
    option(T &&) -> option<typename std::remove_cvref_t<T>::value_type>;
} // namespace opt

export namespace opt::detail {
    template <typename T>
    concept hash_enabled = requires { std::hash<T>{}(std::declval<T>()); };

    constexpr size_t unspecified_hash_value = 0;
} // namespace opt::detail

// https://eel.is/c++draft/optional.hash#lib:hash,optional
export template <typename T>
    requires opt::detail::hash_enabled<std::remove_const_t<T>> || std::same_as<std::remove_cv_t<T>, void>
struct std::hash<opt::option<T>> {
    size_t operator()(const opt::option<T> &o) const
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