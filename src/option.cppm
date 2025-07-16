module;

#include <yvals_core.h>

export module option;

import std;

static_assert(__cpp_explicit_this_parameter >= 202110L, "Deducing `this` is required");

static_assert(__cpp_lib_expected >= 202202L, "`std::expected` is required");

export namespace opt {
    template <typename T>
    class option;

    template <template <typename...> typename Template, typename T>
    struct is_specialization_of : std::false_type {};

    template <template <typename...> class Template, typename... Args>
    struct is_specialization_of<Template, Template<Args...>> : std::true_type {};

    template <template <typename...> class Template, typename T>
    concept specialization_of = is_specialization_of<Template, std::remove_cvref_t<T>>::value;

    template <typename T>
    concept expected_type = specialization_of<std::expected, T>;

    template <typename T>
    concept pair_type = specialization_of<std::pair, T>;

    template <typename T>
    concept option_type = specialization_of<option, T>;

    class option_panic : public std::exception {
    public:
        option_panic(const char *message) noexcept : message{ message } {}

        const char *what() const noexcept override {
            return message;
        }

    private:
        const char *message;
    };

    template <typename T>
    struct option_storage {
        union {
            T value;
        };
        bool has_value = false;

        constexpr option_storage() noexcept : has_value{ false } {}
        constexpr option_storage(const T &val) noexcept(std::is_nothrow_copy_constructible_v<T>) :
            value{ val }, has_value{ true } {}
        constexpr option_storage(T &&val) noexcept(std::is_nothrow_move_constructible_v<T>) :
            value{ std::move(val) }, has_value{ true } {}

        constexpr option_storage(const option_storage &other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
            has_value{ other.has_value } {
            if (has_value) {
                new (&value) T(other.value);
            }
        }

        constexpr option_storage(option_storage &&other) noexcept(std::is_nothrow_move_constructible_v<T>) :
            has_value{ other.has_value } {
            if (has_value) {
                new (&value) T(std::move(other.value));
                other.has_value = false;
            }
        }

        constexpr option_storage &operator=(const option_storage &other) noexcept(
            std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
            if (this != &other) {
                if (has_value) {
                    value.~T();
                }
                has_value = other.has_value;
                if (has_value) {
                    new (&value) T(other.value);
                }
            }
            return *this;
        }

        constexpr option_storage &operator=(option_storage &&other) noexcept(std::is_nothrow_move_constructible_v<T>
                                                                             && std::is_nothrow_destructible_v<T>) {
            if (this != &other) {
                if (has_value) {
                    value.~T();
                }
                has_value = other.has_value;
                if (has_value) {
                    new (&value) T(std::move(other.value));
                    other.has_value = false;
                }
            }
            return *this;
        }

        constexpr ~option_storage() noexcept {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                if (has_value) {
                    value.~T();
                }
            }
        }

        constexpr auto &&get(this auto &&self) noexcept {
            return self.value;
        }
    };

    template <typename T>
    struct option_storage<T &> {
        T *ptr         = nullptr;
        bool has_value = false;

        constexpr option_storage() noexcept : ptr{ nullptr }, has_value{ false } {}
        constexpr option_storage(T &val) noexcept : ptr{ &val }, has_value{ true } {}

        constexpr option_storage(const option_storage &other)            = default;
        constexpr option_storage(option_storage &&other)                 = default;
        constexpr option_storage &operator=(const option_storage &other) = default;
        constexpr option_storage &operator=(option_storage &&other)      = default;

        constexpr T &get() const noexcept {
            return *ptr;
        }
    };

    template <>
    struct option_storage<void> {
        bool has_value = false;

        constexpr option_storage() noexcept : has_value{ false } {}
        constexpr option_storage(bool has_val) noexcept : has_value{ has_val } {}

        constexpr void get() const noexcept {}
    };

    template <typename T>
    class option {
    private:
        option_storage<T> storage;

    public:
        using value_type = T;

        constexpr option() noexcept = default;

        constexpr option(bool has_value) noexcept
            requires std::same_as<T, void>
            : storage{ has_value } {}

        template <typename U = T>
        constexpr option(const U &value) noexcept(std::is_nothrow_constructible_v<option_storage<T>, const U &>)
            requires (!std::is_reference_v<T>)
                  && (!std::same_as<std::decay_t<U>, option<T>>)
                  && (!std::same_as<T, void>)
            : storage{ value } {}

        template <typename U = T>
        constexpr option(U &&value) noexcept(std::is_nothrow_constructible_v<option_storage<T>, U &&>)
            requires (!std::is_reference_v<T>)
                  && (!std::same_as<std::decay_t<U>, option<T>>)
                  && (!std::same_as<T, void>)
            : storage{ std::move(value) } {}

        template <typename U = T>
        constexpr option(std::remove_reference_t<U> &value) noexcept
            requires std::is_reference_v<T>
            : storage{ value } {}

        template <option_type U>
        constexpr auto and_(this auto &&self, U &&optb) noexcept(std::is_nothrow_constructible_v<std::decay_t<U>>) {
            if (self.is_some()) {
                return std::forward<U>(optb);
            }
            return std::decay_t<U>{};
        }

        template <typename F>
        constexpr auto and_then(this auto &&self, F &&f)
            requires (!std::same_as<T, void>)
                  && std::invocable<F, decltype(std::forward<decltype(self)>(self).unwrap())>
        {
            using result_type = std::invoke_result_t<F, decltype(std::forward<decltype(self)>(self).unwrap())>;
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward<decltype(self)>(self).unwrap());
            }
            return result_type{};
        }

        template <typename F>
        constexpr auto and_then(this auto &&self, F &&f)
            requires std::same_as<T, void> && std::invocable<F>
        {
            using result_type = std::invoke_result_t<F>;
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f));
            }
            return result_type{};
        }

        auto as_deref() const {
            if constexpr (std::is_pointer_v<T>) {
                using deref_type = const std::remove_pointer_t<T> &;
                if (is_some()) {
                    return option<deref_type>{ *storage.get() };
                }
                return option<deref_type>{};
            }
            return as_ref().map([](const auto &self) -> const auto & {
                return *self;
            });
        }

        auto as_deref_mut() {
            return as_mut().map([](auto &&self) -> decltype(auto) {
                return *self;
            });
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
            if (!storage.has_value) {
                throw option_panic(msg);
            }
            return storage.get();
        }

        constexpr void expect(const char *msg) const
            requires std::same_as<T, void>
        {
            if (!storage.has_value) {
                throw option_panic(msg);
            }
            storage.get();
        }

        template <typename F>
        constexpr auto filter(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            if (self.is_some() && std::invoke(std::forward<F>(predicate), self.storage.get())) {
                return self;
            }
            return std::decay_t<decltype(self)>{};
        }

        constexpr auto flatten(this auto &&self)
            requires option_type<T>
        {
            if (self.is_some()) {
                return self.unwrap();
            }
            return T{};
        }

        template <typename U = T>
        constexpr auto &get_or_insert(this auto &&self, const U &value)
            requires std::constructible_from<T, const U &>
        {
            if (!self.storage.has_value) {
                self.storage = option_storage<T>{ value };
            }
            return self.storage.get();
        }

        constexpr auto &get_or_insert_default()
            requires std::default_initializable<T>
        {
            if (!storage.has_value) {
                storage = option_storage<T>{ T{} };
            }
            return storage.get();
        }

        template <typename F>
        constexpr auto &get_or_insert_with(this auto &&self, F &&f)
            requires std::invocable<F> && std::same_as<std::invoke_result_t<F>, T>
        {
            if (!self.storage.has_value) {
                self.storage = option_storage<T>{ std::invoke(std::forward<F>(f)) };
            }
            return self.storage.get();
        }

        template <typename U = T>
        constexpr auto &insert(this auto &&self, const U &value)
            requires std::constructible_from<T, const U &>
        {
            self.storage = option_storage<T>{ value };
            return self.storage.get();
        }

        template <typename F>
        constexpr auto inspect(this auto &&self, F &&f) -> decltype(self) {
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
            return !storage.has_value;
        }

        template <typename F>
        constexpr auto is_none_or(this auto &&self, F &&predicate)
            requires std::predicate<F, decltype(self.storage.get())>
        {
            return self.is_none() || std::invoke(std::forward<F>(predicate), self.storage.get());
        }

        constexpr bool is_some() const noexcept {
            return storage.has_value;
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
                using result_type = std::invoke_result_t<F, decltype(std::forward<decltype(self)>(self).unwrap())>;
                if (self.is_some()) {
                    if constexpr (std::same_as<result_type, void>) {
                        std::invoke(std::forward<F>(f), std::forward<decltype(self)>(self).unwrap());
                        return option<void>{ true };
                    } else {
                        return option<result_type>{ std::invoke(std::forward<F>(f),
                                                                std::forward<decltype(self)>(self).unwrap()) };
                    }
                }
                return option<result_type>{};
            }
        }

        template <typename F, typename U>
        constexpr auto map_or(this auto &&self, U &&default_value, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward<decltype(self)>(self).unwrap())>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward<decltype(self)>(self).unwrap());
            }
            return std::forward<U>(default_value);
        }

        template <typename D, typename F>
        constexpr auto map_or_else(this auto &&self, D &&default_f, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F, decltype(std::forward<decltype(self)>(self).unwrap())>
                  && std::invocable<D>
        {
            if (self.is_some()) {
                return std::invoke(std::forward<F>(f), std::forward<decltype(self)>(self).unwrap());
            }
            return std::invoke(std::forward<D>(default_f));
        }

        template <typename E>
        constexpr auto ok_or(this auto &&self, E &&err)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::constructible_from<std::decay_t<E>, E>
        {
            using result_type = std::expected<T, std::decay_t<E>>;
            if (self.is_some()) {
                return result_type{ std::forward<decltype(self)>(self).unwrap() };
            }
            return result_type{ std::unexpect, std::forward<E>(err) };
        }

        template <typename F>
        constexpr auto ok_or_else(this auto &&self, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::invocable<F>
                  && std::constructible_from<std::decay_t<std::invoke_result_t<F>>, std::invoke_result_t<F>>
        {
            using result_type = std::expected<T, std::invoke_result_t<F>>;
            if (self.is_some()) {
                return result_type{ std::forward<decltype(self)>(self).unwrap() };
            }
            return result_type{ std::unexpect, std::invoke(std::forward<F>(f)) };
        }

        template <option_type U>
        constexpr auto or_(this auto &&self, U &&optb) {
            if (self.is_some()) {
                using self_type = std::decay_t<decltype(self)>;
                return self_type{ std::forward<decltype(self)>(self).unwrap() };
            }
            return std::forward<U>(optb);
        }

        template <typename F>
        constexpr auto or_else(this auto &&self, F &&f) -> decltype(self)
            requires option_type<std::invoke_result_t<F>>
        {
            if (self.is_some()) {
                return self;
            }
            return std::invoke(std::forward<F>(f));
        }

        template <typename U = T>
        constexpr auto replace(this auto &&self, const U &value)
            requires std::constructible_from<T, const U &>
        {
            option<T> old;
            if (self.storage.has_value) {
                std::ranges::swap(self.storage, old.storage);
            }
            self.storage = option_storage<T>{ value };
            return old;
        }

        constexpr auto take(this auto &&self) -> option<T> {
            option<T> result;
            if (self.storage.has_value) {
                std::ranges::swap(self.storage, result.storage);
            }
            return result;
        }

        template <typename F>
        constexpr auto take_if(this auto &&self, F &&f) -> option<T>
            requires std::predicate<F, decltype(self.storage.get())>
        {
            option<T> result;
            if (self.storage.has_value && std::invoke(std::forward<F>(f), self.storage.get())) {
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
            if (!self.storage.has_value)
                throw option_panic("Attempted to access value of empty option");
            return self.storage.get();
        }

        constexpr void unwrap(this auto &&self)
            requires std::same_as<T, void>
        {
            if (!self.storage.has_value)
                throw option_panic("Attempted to access value of empty option");
            self.storage.get();
        }

        template <typename U>
        constexpr auto unwrap_or(this auto &&self, U &&default_value)
            requires (!std::same_as<T, void>)
                  && (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && std::convertible_to<U, T>
        {
            if (self.is_some()) {
                return self.unwrap();
            }
            return std::forward<U>(default_value);
        }

        constexpr auto unwrap_or_default(this auto &&self)
            requires (!std::same_as<T, void>) && std::default_initializable<T>
        {
            if (self.is_some()) {
                return self.unwrap();
            }
            return T{};
        }

        constexpr auto unwrap_unchecked(this auto &&self) noexcept -> decltype(self.storage.get()) {
            if (self.storage.has_value) {
                return self.storage.get();
            }
            std::unreachable();
        }

        constexpr auto unzip(this auto &&self)
            requires pair_type<typename std::decay_t<T>>
        {
            using first_type  = decltype(self.unwrap().first);
            using second_type = decltype(self.unwrap().second);

            if (self.is_some()) {
                auto &pair = self.unwrap();
                return std::pair{ option<first_type>{ pair.first }, option<second_type>{ pair.second } };
            }
            return std::pair{ option<first_type>{}, option<second_type>{} };
        }

        template <option_type U>
        constexpr auto xor_(this auto &&self, U &&optb)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(optb)>
                          ? std::copy_constructible<typename std::decay_t<U>::value_type>
                          : std::move_constructible<typename std::decay_t<U>::value_type>)
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
                          ? std::copy_constructible<typename std::decay_t<U>::value_type>
                          : std::move_constructible<typename std::decay_t<U>::value_type>)
        {
            if (self.is_some() && optb.is_some()) {
                return option<std::pair<T, typename std::decay_t<U>::value_type>>{
                    { self.unwrap(), optb.unwrap() }
                };
            }
            return option<std::pair<T, typename std::decay_t<U>::value_type>>{};
        }

        template <option_type U, typename F>
        constexpr auto zip_with(this auto &&self, U &&other, F &&f)
            requires (std::is_lvalue_reference_v<decltype(self)> ? std::copy_constructible<T>
                                                                 : std::move_constructible<T>)
                  && (std::is_lvalue_reference_v<decltype(other)>
                          ? std::copy_constructible<typename std::decay_t<U>::value_type>
                          : std::move_constructible<typename std::decay_t<U>::value_type>)
                  && std::invocable<F, decltype(self.unwrap()), decltype(other.unwrap())>
        {
            using result_type = std::invoke_result_t<F, decltype(self.unwrap()), decltype(other.unwrap())>;
            if (self.is_some() && other.is_some()) {
                return option<result_type>{ std::invoke(std::forward<F>(f), self.unwrap(), other.unwrap()) };
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
                return self.unwrap();
            }
            return std::invoke(std::forward<F>(f));
        }

        static constexpr auto default_() noexcept -> option<T> {
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
        static constexpr auto from(option<U> &other) -> option<T>
            requires std::is_reference_v<T> && std::same_as<T, U &>
        {
            if (other.is_some()) {
                return option<T>{ other.unwrap() };
            }
            return option<T>{};
        }

        template <typename U>
        static constexpr auto from(U &&value) -> option<T>
            requires std::constructible_from<T, U>
        {
            if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
                return option<T>{ std::move(value) };
            } else {
                return option<T>{ value };
            }
        }

        constexpr auto cmp(const option<T> &other) const
            noexcept(noexcept(std::declval<option<T>>() <=> std::declval<option<T>>()))
            requires std::three_way_comparable<T>
        {
            return *this <=> other;
        }

        constexpr auto max(this auto &&self, option<T> &&other) noexcept(noexcept(self > other))
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self > other ? self : std::move(other);
            }
            return self.is_some() ? self : std::move(other);
        }

        constexpr auto min(this auto &&self, option<T> &&other) noexcept(noexcept(self < other))
            requires std::three_way_comparable<T>
        {
            if (self.is_some() && other.is_some()) {
                return self < other ? self : std::move(other);
            }
            return self.is_some() ? self : std::move(other);
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
                               const option<T> &rhs) noexcept(noexcept(std::declval<T>() <=> std::declval<T>())) {
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

    template <typename T>
    constexpr auto some(const T &value) noexcept(std::is_nothrow_constructible_v<option<T>, const T &>) {
        return option<T>{ value };
    }

    template <typename T>
    constexpr auto some(T &&value) noexcept(std::is_nothrow_constructible_v<option<T>, T &&>) {
        return option<T>{ std::move(value) };
    }

    constexpr auto some() noexcept {
        return option<void>{ true };
    }

    // template <typename T>
    // constexpr auto none() {
    //     return option<T>{};
    // }

    template <typename T>
    constexpr auto none = option<T>{};

    template <typename T>
    option(T) -> option<T>;
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