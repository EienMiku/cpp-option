/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                 *
 *  To enable pointer optimization for `opt::option<T *>`, define  *
 *  `OPT_OPTION_PTR_OPTIMIZATION` before importing this module.    *
 *  Disabled by default for portability and constexpr support.     *
 *  `#define OPT_OPTION_PTR_OPTIMIZATION`                          *
 *                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
export module option;

import std;

static_assert(__cpp_explicit_this_parameter >= 202110L, "Deducing `this` is required");

static_assert(__cpp_lib_expected >= 202202L, "`std::expected` is required");

export namespace opt {
    struct none_t;

    namespace detail {
        template <typename T>
        concept option_prohibited_type = (std::same_as<std::remove_cvref_t<T>, none_t>)
                                      || (std::same_as<std::remove_cvref_t<T>, std::nullopt_t>)
                                      || (std::same_as<std::remove_cvref_t<T>, std::in_place_t>);
    }

    using detail::option_prohibited_type;

    template <typename T>
        requires (!detail::option_prohibited_type<T>)
    class option;

    namespace detail {
        template <template <typename...> typename Template, typename T>
        struct is_specialization_of : std::false_type {};

        template <template <typename...> class Template, typename... Ts>
        struct is_specialization_of<Template, Template<Ts...>> : std::true_type {};

        template <template <typename...> class Template, typename T>
        concept specialization_of = is_specialization_of<Template, std::remove_cvref_t<T>>::value;

        template <typename T>
        concept expected_type = specialization_of<std::expected, T>;

        template <typename T>
        concept pair_type = specialization_of<std::pair, T>;

        template <typename T>
        concept option_type = specialization_of<option, T>;

        template <typename T, typename U>
        concept option_constructible_or_convertible = std::constructible_from<T, option<U> &>
                                                   || std::constructible_from<T, const option<U> &>
                                                   || std::constructible_from<T, option<U> &&>
                                                   || std::constructible_from<T, const option<U> &&>
                                                   || std::convertible_to<option<U> &, T>
                                                   || std::convertible_to<const option<U> &, T>
                                                   || std::convertible_to<option<U> &&, T>
                                                   || std::convertible_to<const option<U> &&, T>;
    } // namespace detail

    using detail::expected_type;
    using detail::option_constructible_or_convertible;
    using detail::option_type;
    using detail::pair_type;
    using detail::specialization_of;

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
        template <typename T>
        struct option_storage {
            union {
                T value;
            };
            bool has_value_ = false;

            constexpr option_storage() noexcept : has_value_{ false } {}

            constexpr option_storage(const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) :
                value{ val }, has_value_{ true } {}

            constexpr option_storage(T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) :
                value{ std::move(val) }, has_value_{ true } {}

            constexpr option_storage(const option_storage &other) noexcept(std::is_nothrow_copy_constructible_v<T>)
                requires std::copy_constructible<T> && (!std::is_trivially_copy_constructible_v<T>)
                : has_value_{ other.has_value_ } {
                if (has_value_) {
                    new (&value) T(other.value);
                }
            }

            constexpr option_storage(const option_storage &other) noexcept
                requires std::is_trivially_copy_constructible_v<T>
            = default;

            constexpr option_storage(option_storage &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
                requires std::move_constructible<T> && (!std::is_trivially_move_constructible_v<T>)
                : has_value_{ other.has_value_ } {
                if (has_value_) {
                    new (&value) T(std::move(other.value));
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
                            value.~T();
                        }
                    }
                    has_value_ = other.has_value_;
                    if (has_value_) {
                        new (&value) T(other.value);
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
                            value.~T();
                        }
                    }
                    has_value_ = other.has_value_;
                    if (has_value_) {
                        new (&value) T(std::move(other.value));
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
                    value.~T();
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
                        value.~T();
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
        };

#ifdef OPT_OPTION_PTR_OPTIMIZATION
        /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         *                                                                                         *
         *                 Specialized storage for pointer types with optimization                 *
         *                                                                                         *
         *  This implementation uses a sentinel value (`reinterpret_cast<uintptr_t>(-1)`) to       *
         *  represent the empty state.                                                             *
         *                                                                                         *
         *  This is technically UNDEFINED BEHAVIOR according to the C++ standard, as casting       *
         *  -1 to a pointer is not guaranteed to be valid or safe on all platforms or compilers.   *
         *  It works in practice on most modern platforms (such as x86_64 and aarch64), but may    *
         *  break on exotic architectures or with aggressive compiler optimizations (e.g.,         *
         *  pointer sanitization, CHERI, etc.).                                                    *
         *                                                                                         *
         *  Potential issues:                                                                      *
         *    - If user code ever legitimately uses the sentinel value as a valid pointer,         *
         *      has_value() will be incorrect.                                                     *
         *    - Some static analyzers or sanitizers may report this as UB or dangerous.            *
         *    - Future C++ standards or platforms may make this pattern non-portable.              *
         *                                                                                         *
         *  Use with caution and when you are sure your platform and toolchain are compatible.     *
         *                                                                                         *
         * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

        /* * * * * * * * * * * * * * * * *
         *  NO POSSIBLE `constexpr` now  *
         * * * * * * * * * * * * * * * * */
        template <typename T>
        struct option_storage<T *> {
            using pointer_t = T *;
            // constexpr
            inline static pointer_t sentinel = reinterpret_cast<T *>(static_cast<uintptr_t>(-1));

            pointer_t ptr = sentinel;

            // constexpr
            option_storage() noexcept : ptr{ sentinel } {}
            // constexpr
            option_storage(pointer_t val) noexcept : ptr{ val } {}

            // constexpr
            option_storage(const option_storage &other) = default;
            // constexpr
            option_storage(option_storage &&other) = default;
            // constexpr
            option_storage &operator=(const option_storage &other) = default;
            // constexpr
            option_storage &operator=(option_storage &&other) = default;

            // constexpr
            bool has_value() const noexcept {
                return ptr != sentinel;
            }

            // constexpr
            auto &&get(this auto &&self) noexcept {
                return self.ptr;
            }

            // constexpr
            void reset() noexcept {
                ptr = sentinel;
            }
        };
#else
        template <typename T>
        struct option_storage<T *> {
            using pointer_t = T *;
            pointer_t ptr   = nullptr;
            bool has_value_ = false;

            constexpr option_storage() noexcept : ptr{ nullptr }, has_value_{ false } {}
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
        };
#endif

        template <typename T>
        struct option_storage<T &> {
            T *ptr          = nullptr;
            bool has_value_ = false;

            constexpr option_storage() noexcept : ptr{ nullptr }, has_value_{ false } {}
            constexpr option_storage(T &val) noexcept : ptr{ &val }, has_value_{ true } {}

            constexpr option_storage(const option_storage &other)            = default;
            constexpr option_storage(option_storage &&other)                 = default;
            constexpr option_storage &operator=(const option_storage &other) = default;
            constexpr option_storage &operator=(option_storage &&other)      = default;

            constexpr T &get() const noexcept {
                return *ptr;
            }
            constexpr bool has_value() const noexcept {
                return has_value_;
            }
            constexpr void reset() noexcept {
                ptr        = nullptr;
                has_value_ = false;
            }
        };
    } // namespace detail

    namespace detail {
        template <>
        struct option_storage<void> {
            bool has_value_ = false;

            constexpr option_storage() noexcept : has_value_{ false } {}
            constexpr option_storage(bool has_val) noexcept : has_value_{ has_val } {}

            constexpr void get() const noexcept {}
            constexpr bool has_value() const noexcept {
                return has_value_;
            }
            constexpr void reset() noexcept {
                has_value_ = false;
            }
        };
    } // namespace detail

    struct none_t {
        constexpr none_t() noexcept = default;
    };

    constexpr bool operator==(none_t, none_t) noexcept {
        return true;
    }

    constexpr std::strong_ordering operator<=>(none_t, none_t) noexcept {
        return std::strong_ordering::equal;
    }

    template <typename T>
        requires (!detail::option_prohibited_type<T>)
    class option {
    private:
        detail::option_storage<T> storage;

    public:
        using value_type = T;

        constexpr option() noexcept = default;

        constexpr option(const option &)
            requires std::copy_constructible<T>
        = default;
        constexpr option(const option &)
            requires (!std::copy_constructible<T>)
        = delete;

        constexpr option(option &&)
            requires std::move_constructible<T>
        = default;

        template <typename U>
        constexpr explicit(!std::convertible_to<const U &, T>) option(const option<U> &other)
            requires std::constructible_from<T, const U &>
                  && (std::same_as<T, bool> || !option_constructible_or_convertible<T, U>)
        {
            if (other.is_some()) {
                storage = detail::option_storage<T>(other.unwrap());
            }
        }

        template <typename U>
        constexpr explicit(!std::convertible_to<U, T>) option(option<U> &&other)
            requires std::constructible_from<T, U>
                  && (std::same_as<T, bool> || !option_constructible_or_convertible<T, U>)
        {
            if (other.is_some()) {
                storage = detail::option_storage<T>(std::move(other.unwrap()));
            }
        }

        constexpr option &operator=(const option &)
            requires std::copy_constructible<T>
        = default;

        constexpr option &operator=(option &&)
            requires std::move_constructible<T>
        = default;

        constexpr option(bool has_value) noexcept
            requires std::same_as<T, void>
            : storage{ has_value } {}

        template <typename U = std::remove_cv_t<T>>
        constexpr option(U &&value) noexcept(std::is_nothrow_constructible_v<detail::option_storage<T>, U &&>)
            requires (!std::is_reference_v<T>)
                  && (!std::same_as<std::remove_cvref_t<U>, option<T>>)
                  && (!std::same_as<T, void>)
                  && (!opt::specialization_of<std::optional, U>)
                  && std::constructible_from<detail::option_storage<T>, U &&>
            : storage{ std::forward<U>(value) } {}

        template <typename U = std::remove_cv_t<T>>
        constexpr option(std::remove_reference_t<U> &value) noexcept
            requires std::is_reference_v<T>
            : storage{ value } {}

        constexpr option(none_t) noexcept {
            storage = detail::option_storage<T>{};
        }

        constexpr option<T> &operator=(none_t) noexcept {
            storage.reset();
            return *this;
        }

        /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         *                                                                   *
         *                Standard Library Compatibility Section             *
         *                                                                   *
         *  The following constructors, assignment operators, conversions,   *
         *  etc. provide interoperability with `std::optional`.              *
         *  These APIs are designed to match the standard library semantics  *
         *  as closely as possible.                                          *
         *                                                                   *
         * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
        constexpr option(std::nullopt_t) noexcept : storage{} {}

        template <typename U = std::remove_cv_t<T>>
        constexpr option(const std::optional<U> &std_opt) noexcept(std::is_nothrow_copy_constructible_v<U>)
            requires (!std::is_void_v<U>)
                  && (!std::is_reference_v<U>)
                  && std::constructible_from<T, U>
                  && (std::same_as<T, bool> || !option_constructible_or_convertible<T, U>)
        {
            if (std_opt.has_value()) {
                storage = detail::option_storage<T>(*std_opt);
            }
        }

        template <typename U = std::remove_cv_t<T>>
        constexpr option(std::optional<U> &&std_opt) noexcept(std::is_nothrow_move_constructible_v<U>)
            requires (!std::is_void_v<U>)
                  && (!std::is_reference_v<U>)
                  && std::constructible_from<T, U>
                  && (std::same_as<T, bool> || !option_constructible_or_convertible<T, U>)
        {
            if (std_opt.has_value()) {
                storage = detail::option_storage<T>(std::move(*std_opt));
            }
        }

        template <typename... Ts>
        constexpr explicit option(std::in_place_t, Ts &&...args) noexcept(std::is_nothrow_constructible_v<T, Ts &&...>)
            requires std::constructible_from<T, Ts &&...> && (!std::is_void_v<T>)
            : storage{ T(std::forward<Ts>(args)...) } {}

        template <typename U, typename... Ts>
        constexpr explicit option(std::in_place_t, std::initializer_list<U> ilist, Ts &&...args) noexcept(
            std::is_nothrow_constructible_v<T, std::initializer_list<U>, Ts &&...>)
            requires std::constructible_from<T, std::initializer_list<U>, Ts &&...> && (!std::is_void_v<T>)
            :
            storage{
                T{ ilist, std::forward<Ts>(args)... }
        } {}

        constexpr option &operator=(const std::optional<T> &opt) noexcept(std::is_nothrow_copy_constructible_v<T>
                                                                          && std::is_nothrow_destructible_v<T>) {
            if (opt.has_value()) {
                storage = detail::option_storage<T>(*opt);
            } else {
                storage = detail::option_storage<T>{};
            }
            return *this;
        }

        constexpr option &operator=(std::optional<T> &&opt) noexcept(std::is_nothrow_move_constructible_v<T>
                                                                     && std::is_nothrow_destructible_v<T>) {
            if (opt.has_value()) {
                storage = detail::option_storage<T>(std::move(*opt));
            } else {
                storage = detail::option_storage<T>{};
            }
            return *this;
        }

        constexpr explicit operator std::optional<T>() const noexcept(std::is_nothrow_copy_constructible_v<T>) {
            if (is_some()) {
                return std::optional<T>{ storage.get() };
            }
            return std::nullopt;
        }

        constexpr auto operator->(this auto &&self) noexcept {
            // no check to keep the behavior consistent with std::optional
            return &self.storage.get();
        }

        constexpr auto operator*(this auto &&self) noexcept {
            // no check to keep the behavior consistent with std::optional
            return self.storage.get();
        }

        constexpr bool has_value() const noexcept {
            return is_some();
        }

        constexpr explicit operator bool() const noexcept {
            return is_some();
        }

        // iterator
        struct iterator {
            using iterator_category = std::input_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = T *;
            using reference         = T &;

            option<T> *opt;

            constexpr iterator(option<T> *opt) noexcept : opt{ opt } {}

            constexpr reference operator*() const noexcept {
                return opt->storage.get();
            }
            constexpr pointer operator->() const noexcept {
                return &opt->storage.get();
            }
            constexpr iterator &operator++() noexcept {
                opt = nullptr;
                return *this;
            }
            constexpr bool operator==(const iterator &other) const noexcept {
                return opt == other.opt;
            }
        };

        struct const_iterator {
            using iterator_category = std::input_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const T *;
            using reference         = const T &;

            const option<T> *opt;

            constexpr const_iterator(const option<T> *opt) noexcept : opt{ opt } {}

            constexpr reference operator*() const noexcept {
                return opt->storage.get();
            }
            constexpr pointer operator->() const noexcept {
                return &opt->storage.get();
            }
            constexpr const_iterator &operator++() noexcept {
                opt = nullptr;
                return *this;
            }
            constexpr bool operator==(const const_iterator &other) const noexcept {
                return opt == other.opt;
            }
        };

        constexpr iterator begin() noexcept {
            if (is_some())
                return iterator{ this };
            return end();
        }

        constexpr const_iterator begin() const noexcept {
            if (is_some())
                return const_iterator{ this };
            return end();
        }

        constexpr iterator end() noexcept {
            return iterator{ nullptr };
        }

        constexpr const_iterator end() const noexcept {
            return const_iterator{ nullptr };
        }

        constexpr auto &&value(this auto &&self)
            requires (!std::same_as<T, void>)
        {
            if (!self.storage.has_value())
                throw std::bad_optional_access{};
            return self.storage.get();
        }

        template <typename U = std::remove_cv_t<T>>
        constexpr auto value_or(this auto &&self, U &&default_value)
            requires (!std::same_as<T, void>)
        {
            if (self.storage.has_value()) {
                return std::forward_like<decltype(self)>(self.unwrap());
            }
            return static_cast<T>(std::forward<U>(default_value));
        }

        // and_then => see below

        // restricts to non-void types
        template <typename F>
        constexpr auto transform(this auto &&self, F &&f)
            requires (!std::same_as<T, void>)
        {
            return self.map(std::forward<F>(f));
        }

        // or_else => see below

        constexpr auto swap(this auto &&self, option<T> &other) noexcept(std::is_nothrow_move_constructible_v<T>
                                                                         && std::is_nothrow_swappable_v<T>)
            requires std::move_constructible<T> && std::swappable<T>
        {
            std::ranges::swap(self.storage, other.storage);
        }

        constexpr auto reset() noexcept {
            storage.reset();
        }

        template <typename U = std::remove_cvref_t<T>, typename... Ts>
        constexpr U &emplace(Ts &&...args) noexcept(std::is_nothrow_constructible_v<U, Ts &&...>)
            requires (!std::is_void_v<U>)
        {
            reset();
            storage = detail::option_storage<T>(U{ std::forward<Ts>(args)... });
            return storage.get();
        }

        template <typename U = std::remove_cvref_t<T>, class V, class... Ts>
        constexpr U &emplace(std::initializer_list<V> ilist, Ts &&...args) noexcept(
            std::is_nothrow_constructible_v<U, std::initializer_list<V>, Ts &&...>)
            requires (!std::is_void_v<U>)
        {
            reset();
            storage = detail::option_storage<T>(U{ ilist, std::forward<Ts>(args)... });
            return storage.get();
        }

        /* * * * * * * * * * * * * * * * * * * * * * * * * *
         *  End of standard library compatibility section  *
         * * * * * * * * * * * * * * * * * * * * * * * * * */

        template <option_type U>
        constexpr auto and_(this auto &&self,
                            U &&optb) noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<U>>) {
            if (self.is_some()) {
                return std::forward<U>(optb);
            }
            return std::remove_cvref_t<U>{};
        }

        template <typename F>
        constexpr auto and_then(this auto &&self, F &&f)
            requires (!std::same_as<T, void>)
                  && std::invocable<F, decltype(std::forward_like<decltype(self)>(self.unwrap()))>
                  && option_type<std::invoke_result_t<F, decltype(std::forward_like<decltype(self)>(self.unwrap()))>>
        {
            using result_type = std::invoke_result_t<F, decltype(std::forward_like<decltype(self)>(self.unwrap()))>;
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap()));
            }
            return std::remove_cvref_t<result_type>{};
        }

        template <typename F>
        constexpr auto and_then(this auto &&self, F &&f)
            requires std::same_as<T, void> && std::invocable<F>
        {
            using result_type = std::invoke_result_t<F>;
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f));
            }
            return std::remove_cvref_t<result_type>{};
        }

        auto as_deref() const {
            if constexpr (std::is_pointer_v<T>) {
                using deref_type = const std::remove_pointer_t<T> &;
                if (is_some()) {
                    return option<deref_type>{ *storage.get() };
                }
                return option<deref_type>{};
            }
            return as_ref().map([](const auto &self) -> const auto & { return *self; });
        }

        auto as_deref_mut() {
            return as_mut().map([](auto &&self) -> decltype(auto) { return *self; });
        }

        constexpr auto as_mut() noexcept {
            using mut_ref_type = std::conditional_t<std::is_reference_v<T>, std::remove_reference_t<T> &, T &>;
            if (is_some()) {
                return option<mut_ref_type>{ storage.get() };
            }
            return option<mut_ref_type>{};
        }

        constexpr auto as_ref() const noexcept {
            using const_ref_type =
                std::conditional_t<std::is_reference_v<T>, const std::remove_reference_t<T> &, const T &>;
            if (is_some()) {
                return option<const_ref_type>{ storage.get() };
            }
            return option<const_ref_type>{};
        }

        auto cloned() const
            requires requires(const T &t) { t.clone(); } && (!std::is_reference_v<T>)
        {
            if (is_some()) {
                return option<T>{ storage.get().clone() };
            }
            return option<T>{};
        }

        constexpr auto copied() const noexcept(std::is_nothrow_copy_constructible_v<option<T>>)
            requires std::copy_constructible<T> && (!std::is_reference_v<T>)
        {
            return *this;
        }

        constexpr auto expect(const char *msg) const
            requires (!std::same_as<T, void>)
        {
            if (!storage.has_value()) {
                throw option_panic(msg);
            }
            return storage.get();
        }

        constexpr void expect(const char *msg) const
            requires std::same_as<T, void>
        {
            if (!storage.has_value()) {
                throw option_panic(msg);
            }
            storage.get();
        }

        template <typename F>
        constexpr auto filter(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            if (self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get())) {
                return std::remove_cvref_t<decltype(self)>{ self };
            }
            return std::remove_cvref_t<decltype(self)>{};
        }

        constexpr auto flatten(this auto &&self)
            requires option_type<T>
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap());
            }
            return std::remove_cvref_t<T>{};
        }

        template <typename U>
        constexpr auto &get_or_insert(this auto &&self, U &&value)
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            if (!self.storage.has_value()) {
                if constexpr (std::is_lvalue_reference_v<decltype(value)>)
                    self.storage = detail::option_storage<T>(value);
                else
                    self.storage = detail::option_storage<T>(std::forward<U>(value));
            }
            return self.storage.get();
        }

        constexpr auto &get_or_insert_default()
            requires std::default_initializable<T>
        {
            if (!storage.has_value()) {
                storage = detail::option_storage<T>(T{});
            }
            return storage.get();
        }

        template <typename F>
        constexpr auto &get_or_insert_with(this auto &&self, F &&f)
            requires std::invocable<F> && std::same_as<std::invoke_result_t<F>, T>
        {
            if (!self.storage.has_value()) {
                self.storage = detail::option_storage<T>(std::invoke(std::forward<F>(f)));
            }
            return self.storage.get();
        }

        template <typename U>
        constexpr auto &insert(this auto &&self, U &&value)
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            if constexpr (std::is_lvalue_reference_v<decltype(value)>)
                self.storage = detail::option_storage<T>(value);
            else
                self.storage = detail::option_storage<T>(std::forward<U>(value));
            return self.storage.get();
        }

        template <typename F>
        constexpr auto inspect(this auto &&self, F &&f) {
            if (self.is_some()) {
                if constexpr (std::same_as<T, void>) {
                    std::invoke(std::forward<F>(f));
                } else {
                    std::invoke(std::forward<F>(f), self.storage.get());
                }
            }
            return self;
        }

        constexpr bool is_none() const noexcept {
            return !is_some();
        }

        template <typename F>
        constexpr auto is_none_or(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_none() || std::invoke(std::forward<F>(predicate), self.storage.get());
        }

        constexpr bool is_some() const noexcept {
            return storage.has_value();
        }

        template <typename F>
        constexpr auto is_some_and(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get());
        }

        template <typename F>
        constexpr auto map(this auto &&self, F &&f)
            requires (std::same_as<T, void>
                      || (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                     : std::move_constructible<T>))
        {
            if constexpr (std::same_as<T, void>) {
                using result_type = std::invoke_result_t<F>;
                if (self.is_some()) {
                    if constexpr (std::same_as<result_type, void>) {
                        std::invoke(std::forward<F>(f));
                        return option<void>{ true };
                    } else {
                        return option<result_type>{ std::invoke(std::forward<F>(f)) };
                    }
                }
                return option<result_type>{};
            } else {
                using result_type = std::invoke_result_t<F, decltype(std::forward_like<decltype(self)>(self.unwrap()))>;
                if (self.is_some()) {
                    if constexpr (std::same_as<result_type, void>) {
                        std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap()));
                        return option<void>{ true };
                    } else {
                        return option<result_type>{ std::invoke(std::forward<F>(f),
                                                                std::forward_like<decltype(self)>(self.unwrap())) };
                    }
                }
                return option<result_type>{};
            }
        }

        template <typename F, typename U>
        constexpr auto map_or(this auto &&self, U &&default_value, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward_like<decltype(self)>(self.unwrap()))>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap()));
            }
            return static_cast<T>(std::forward<U>(default_value));
        }

        template <typename D, typename F>
        constexpr auto map_or_else(this auto &&self, D &&default_f, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward_like<decltype(self)>(self.unwrap()))>
                  && std::invocable<D>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward_like<decltype(self)>(self.unwrap()));
            }
            return std::invoke(std::forward<D>(default_f));
        }

        template <typename E>
        constexpr auto ok_or(this auto &&self, E &&err)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::constructible_from<std::remove_cvref_t<E>, E>
        {
            using result_type = std::expected<T, std::remove_cvref_t<E>>;
            if (self.is_some()) {
                return result_type{ std::forward_like<decltype(self)>(self.unwrap()) };
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        template <typename F>
        constexpr auto ok_or_else(this auto &&self, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F>
                  && std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F>>, std::invoke_result_t<F>>
        {
            using result_type = std::expected<T, std::invoke_result_t<F>>;
            if (self.is_some()) {
                return result_type{ std::forward_like<decltype(self)>(self.unwrap()) };
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(f)) };
        }

        template <option_type U>
        constexpr auto or_(this auto &&self, U &&optb) {
            if (self.is_some()) {
                using self_type = std::remove_cvref_t<decltype(self)>;
                return self_type{ std::forward_like<decltype(self)>(self.unwrap()) };
            }
            return std::forward<U>(optb);
        }

        template <typename F>
        constexpr auto or_else(this auto &&self, F &&f)
            requires option_type<std::invoke_result_t<F>>
        {
            if (self.is_some()) {
                return self;
            }
            return std::invoke(std::forward<F>(f));
        }

        template <typename U>
        constexpr auto replace(this auto &&self, U &&value)
            requires (std::copy_constructible<T> || std::move_constructible<T>) && std::convertible_to<U &&, T>
        {
            option<T> old{};
            if (self.storage.has_value()) {
                std::ranges::swap(self.storage, old.storage);
            }

            if constexpr (std::is_lvalue_reference_v<decltype(value)>)
                self.storage = detail::option_storage<T>(value);
            else
                self.storage = detail::option_storage<T>(std::forward<U>(value));

            return old;
        }

        constexpr auto take(this auto &&self) {
            option<T> result{};
            if (self.storage.has_value()) {
                std::ranges::swap(self.storage, result.storage);
            }
            return result;
        }

        template <typename F>
        constexpr auto take_if(this auto &&self, F &&f)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            option<T> result{};
            if (self.storage.has_value() && std::invoke(std::forward<F>(f), self.storage.get())) {
                std::ranges::swap(self.storage, result.storage);
            }
            return result;
        }

        constexpr auto transpose(this auto &&self)
            requires expected_type<T>
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

        constexpr auto &&unwrap(this auto &&self)
            requires (!std::same_as<T, void>)
        {
            if (!self.storage.has_value())
                throw option_panic("Attempted to access value of empty option");
            return self.storage.get();
        }

        constexpr void unwrap(this auto &&self)
            requires std::same_as<T, void>
        {
            if (!self.storage.has_value())
                throw option_panic("Attempted to access value of empty option");
            self.storage.get();
        }

        template <typename U = std::remove_cv_t<T>>
        constexpr auto unwrap_or(this auto &&self, U &&default_value)
            requires (!std::same_as<T, void>)
                  && (std::is_lvalue_reference_v<decltype(self)>
                          ? (std::copy_constructible<T> && std::convertible_to<const U &, T>)
                          : (std::move_constructible<T> && std::convertible_to<U &&, T>))
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap());
            }
            return static_cast<T>(std::forward<U>(default_value));
        }

        constexpr auto unwrap_or_default(this auto &&self)
            requires (!std::same_as<T, void>) && std::default_initializable<T>
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap());
            }
            return T{};
        }

        constexpr auto unwrap_unchecked(this auto &&self) noexcept -> decltype(self.storage.get()) {
            if (self.storage.has_value()) {
                return self.storage.get();
            }
            std::unreachable();
        }

        constexpr auto unzip(this auto &&self)
            requires pair_type<typename std::remove_cvref_t<T>>
        {
            using first_type  = decltype(self.unwrap().first);
            using second_type = decltype(self.unwrap().second);

            if (self.is_some()) {
                auto &&pair = std::forward_like<decltype(self)>(self.unwrap());
                return std::pair{ option<first_type>{ std::forward_like<decltype(pair)>(pair.first) },
                                  option<second_type>{ std::forward_like<decltype(pair)>(pair.second) } };
            }
            return std::pair{ option<first_type>{}, option<second_type>{} };
        }

        template <option_type U>
        constexpr auto xor_(this auto &&self, U &&optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(optb)>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
        {
            if (self.is_some() && optb.is_none()) {
                return self;
            }
            if (self.is_none() && optb.is_some()) {
                return std::forward<U>(optb);
            }
            return option<T>{};
        }

        template <option_type U>
        constexpr auto zip(this auto &&self, U &&optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(optb)>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
        {
            if (self.is_some() && optb.is_some()) {
                return option<std::pair<T, typename std::remove_cvref_t<U>::value_type>>{
                    { std::forward_like<decltype(self)>(self.unwrap()),
                     std::forward_like<decltype(optb)>(optb.unwrap()) }
                };
            }
            return option<std::pair<T, typename std::remove_cvref_t<U>::value_type>>{};
        }

        template <option_type U, typename F>
        constexpr auto zip_with(this auto &&self, U &&other, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(other)>
                          ? std::copy_constructible<typename std::remove_cvref_t<U>::value_type>
                          : std::move_constructible<typename std::remove_cvref_t<U>::value_type>)
                  && std::invocable<F, decltype(self.unwrap()), decltype(other.unwrap())>
        {
            using result_type = std::invoke_result_t<F, decltype(self.unwrap()), decltype(other.unwrap())>;
            if (self.is_some() && other.is_some()) {
                return option<result_type>{ std::invoke(std::forward<F>(f),
                                                        std::forward_like<decltype(self)>(self.unwrap()),
                                                        std::forward_like<decltype(other)>(other.unwrap())) };
            }
            return option<result_type>{};
        }

        template <typename F>
        constexpr auto unwrap_or_else(this auto &&self, F &&f) -> std::common_type_t<T, std::invoke_result_t<F>>
            requires (!std::same_as<T, void>)
                  && (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::convertible_to<std::invoke_result_t<F>, T>
        {
            if (self.is_some()) {
                return std::forward_like<decltype(self)>(self.unwrap());
            }
            return std::invoke(std::forward<F>(f));
        }

        static constexpr auto default_() noexcept {
            return option<T>{};
        }

        constexpr bool operator==(const option &other) const
            noexcept(noexcept(std::declval<T>() == std::declval<T>())) {
            if (is_some() && other.is_some()) {
                return storage.get() == other.storage.get();
            }
            return is_none() && other.is_none();
        }

        template <typename U>
        static constexpr auto from(option<U> &other)
            requires std::is_reference_v<T> && std::same_as<T, U &>
        {
            if (other.is_some()) {
                return option<T>{ other.unwrap() };
            }
            return option<T>{};
        }

        template <typename U>
        static constexpr auto from(U &&value)
            requires std::constructible_from<T, U>
        {
            if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
                return option<T>{ std::move(value) };
            }
            return option<T>{ value };
        }

        constexpr auto cmp(const option<T> &other) const
            noexcept(noexcept(std::declval<option<T>>() <=> std::declval<option<T>>()))
            requires std::three_way_comparable<T>
        {
            return *this <=> other;
        }

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

        constexpr auto eq(this auto &&self, const option<T> &other) noexcept(noexcept(self == other))
            requires std::equality_comparable<T>
        {
            return self == other;
        }

        constexpr auto ne(this auto &&self, const option<T> &other) noexcept(noexcept(self != other))
            requires std::equality_comparable<T>
        {
            return self != other;
        }

        constexpr auto partial_cmp(this auto &&self, const option<T> &other) noexcept(noexcept(self.cmp(other)))
            requires std::three_way_comparable<T>
        {
            return self.cmp(other);
        }

        constexpr auto lt(this auto &&self, const option<T> &other) noexcept(noexcept(self < other))
            requires std::three_way_comparable<T>
        {
            return self < other;
        }

        constexpr auto le(this auto &&self, const option<T> &other) noexcept(noexcept(self <= other))
            requires std::three_way_comparable<T>
        {
            return self <= other;
        }

        constexpr auto gt(this auto &&self, const option<T> &other) noexcept(noexcept(self > other))
            requires std::three_way_comparable<T>
        {
            return self > other;
        }

        constexpr auto ge(this auto &&self, const option<T> &other) noexcept(noexcept(self >= other))
            requires std::three_way_comparable<T>
        {
            return self >= other;
        }

        /* * * * * * * * * * * * * * * * * * * * * * * * * ** * * * * * * * * * * * * * * * *
         *                                                                                  *
         *  There is no equivalent of Rust's `slice` or `pin` in the C++ standard library.  *
         *  The container and pointer models in C++ are fundamentally different from Rust.  *
         *  C++ does not have a built-in safe slice type, nor any built-in memory pinning   *
         *  mechanism. Furthermore, the C++ iterator model is essentially different from    *
         *  Rust's iterator and slice model: C++ iterators are generic, can be invalidated, *
         *  and may support random access, while Rust's slice/iter are restricted and safe. *
         *  Therefore, these APIs do not make sense to implement in C++.                    *
         *                                                                                  *
         * * * * * * * * * * * * * * * * * * * * * * ** * * * * * * * * * * * * * * * * * * */
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
    constexpr auto operator<=>(const option<T> &lhs,
                               const option<T> &rhs) noexcept(noexcept(std::declval<T>() <=> std::declval<T>()))
        requires std::three_way_comparable<T>
    {
        if (lhs.is_some() && rhs.is_some()) {
            return lhs.unwrap() <=> rhs.unwrap();
        }
        if (lhs.is_some()) {
            return std::strong_ordering::greater;
        }
        if (rhs.is_some()) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::equal;
    }

    // option<T> <=> none_t
    template <typename T>
    constexpr std::strong_ordering operator<=>(const option<T> &opt, none_t) noexcept {
        return opt.is_some() ? std::strong_ordering::greater : std::strong_ordering::equal;
    }

    template <typename T>
    constexpr std::strong_ordering operator<=>(none_t, const option<T> &opt) noexcept {
        return opt.is_some() ? std::strong_ordering::less : std::strong_ordering::equal;
    }

    template <typename T>
    constexpr option<std::decay_t<T>> some(T &&value) noexcept(
        std::is_nothrow_constructible_v<option<std::decay_t<T>>, T &&>)
        requires (!specialization_of<std::reference_wrapper, std::decay_t<T>>)
                && (!option_prohibited_type<std::decay_t<T>>)
    {
        return option<std::decay_t<T>>{ std::forward<T>(value) };
    }

    // needs to specify `<T &>`
    template <typename T>
    constexpr option<T> some(std::type_identity_t<T> value_ref)
        requires std::is_lvalue_reference_v<T> && (!option_prohibited_type<T>)
    {
        return option<T>{ std::in_place, value_ref };
    }

    // std::ref => T &
    template <typename T>
    constexpr option<typename T::type &> some(T value_ref) noexcept
        requires specialization_of<std::reference_wrapper, T> && (!option_prohibited_type<typename T::type>)
    {
        return option<typename T::type &>{ value_ref.get() };
    }

    template <typename T, typename... Ts>
    constexpr option<T> some(Ts &&...args) noexcept(std::is_nothrow_constructible_v<option<T>, Ts &&...>)
        requires std::constructible_from<T, Ts &&...> && (!option_prohibited_type<T>)
    {
        return option<T>{ std::in_place, T(std::forward<Ts>(args)...) };
    }

    template <typename T, typename U, typename... Ts>
    constexpr option<T> some(std::initializer_list<U> il, Ts &&...args) noexcept(
        std::is_nothrow_constructible_v<option<T>, std::initializer_list<U> &, Ts &&...>)
        requires std::constructible_from<T, std::initializer_list<U>, Ts &&...> && (!option_prohibited_type<T>)
    {
        return option<T>{
            std::in_place, T{ il, std::forward<Ts>(args)... }
        };
    }

    constexpr option<void> some() noexcept {
        return option<void>{ true };
    }

    template <typename T>
    constexpr auto none_opt() {
        return option<T>{};
    }

    inline constexpr none_t none{};

    template <typename T>
        requires (!specialization_of<std::optional, T>)
    option(T) -> option<T>;

    template <typename T>
    option(std::reference_wrapper<T>) -> option<T &>;

    template <typename T>
        requires specialization_of<std::optional, std::remove_cvref_t<T>>
    option(T &&) -> option<std::remove_cvref_t<typename std::remove_cvref_t<T>::value_type>>;

    /* * * * * * * * * * * * * * * * * * * * * *
     *  standard library compatibility (maybe) *
     *      make_optional => make_option       *
     * * * * * * * * * * * * * * * * * * * * * */
    template <typename... Ts>
    constexpr auto make_option(Ts &&...args) {
        return some(std::forward<Ts>(args)...);
    }
} // namespace opt

export template <typename T>
struct std::formatter<opt::option<T>> {
    constexpr auto parse(format_parse_context &ctx) noexcept {
        return ctx.begin();
    }

    auto format(const opt::option<T> &opt, auto &ctx) const {
        if constexpr (std::same_as<T, void>) {
            if (opt.is_some()) {
                return std::format_to(ctx.out(), "some()");
            }
            return std::format_to(ctx.out(), "none");
        } else {
            if (opt.is_some()) {
                return std::format_to(ctx.out(), "some({})", opt.unwrap());
            }
            return std::format_to(ctx.out(), "none");
        }
    }
};

export template <typename T>
struct std::hash<opt::option<T>> {
    size_t operator()(const opt::option<T> &opt) const noexcept {
        if (opt.is_some()) {
            if constexpr (std::same_as<T, void>) {
                return std::hash<bool>{}(true);
            } else {
                return std::hash<T>{}(opt.unwrap_unchecked());
            }
        }
        return std::hash<bool>{}(false);
    }
};