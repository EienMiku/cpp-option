export module option:fwd;

import std;

export namespace opt {
    struct none_t;

    namespace detail {
        template <typename T>
        concept option_prohibited_type = !(std::is_lvalue_reference_v<T>
                                           || (std::is_object_v<T> && std::is_destructible_v<T> && !std::is_array_v<T>))
                                      || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, none_t>
                                      || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::nullopt_t>
                                      || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::in_place_t>;
    } // namespace detail

    template <typename T>
    class option;

    namespace detail {
        template <typename T, template <typename...> typename Template>
        constexpr bool is_specialization_of_v = false;

        template <template <typename...> class Template, typename... Ts>
        constexpr bool is_specialization_of_v<Template<Ts...>, Template> = true;

        template <typename T, template <typename...> class Template>
        concept specialization_of = is_specialization_of_v<T, Template>;

        template <typename T>
        concept expected_type = specialization_of<std::remove_cv_t<T>, std::expected>;

        template <typename T>
        concept pair_type = specialization_of<std::remove_cvref_t<T>, std::pair>;

        template <class T>
        constexpr bool tuple_like_no_subrange_impl = false;

        template <class... T>
        constexpr bool tuple_like_no_subrange_impl<std::tuple<T...>> = true;

        template <class T1, class T2>
        constexpr bool tuple_like_no_subrange_impl<std::pair<T1, T2>> = true;

        template <class T, size_t Size>
        constexpr bool tuple_like_no_subrange_impl<std::array<T, Size>> = true;

        template <class T>
        constexpr bool tuple_like_no_subrange_impl<std::complex<T>> = true;

        template <class T>
        concept tuple_like_no_subrange = tuple_like_no_subrange_impl<std::remove_cvref_t<T>>;

        template <class T>
        concept pair_like_no_subrange = tuple_like_no_subrange<T> && std::tuple_size_v<std::remove_cvref_t<T>> == 2;

        template <class T>
        constexpr bool is_ranges_subrange_v = false;

        template <class Iter, class Sent, std::ranges::subrange_kind Kind>
        constexpr bool is_ranges_subrange_v<std::ranges::subrange<Iter, Sent, Kind>> = true;

        template <class T>
        concept tuple_like = tuple_like_no_subrange<T> || is_ranges_subrange_v<std::remove_cvref_t<T>>;

        template <class T>
        concept pair_like = tuple_like<T> && std::tuple_size_v<std::remove_cvref_t<T>> == 2;

        template <typename T>
        concept option_type = specialization_of<std::remove_cvref_t<T>, option>;

        template <typename T>
        concept option_like = option_type<T>
                           || std::same_as<std::remove_cvref_t<T>, none_t>
                           || std::same_as<std::remove_cvref_t<T>, std::nullopt_t>;

        template <typename T>
        concept has_member_clone = requires(const T &t) {
            { t.clone() } -> std::convertible_to<T>;
        };

        template <typename T>
        concept has_adl_clone = requires(const T &t) {
            { clone(t) } -> std::convertible_to<T>;
        };

        template <typename T>
        concept has_clone = has_member_clone<T> || has_adl_clone<T>;

        template <typename T>
        concept cloneable = has_clone<T> || std::is_trivially_copyable_v<T>;

        template <typename T>
        concept noexcept_cloneable = requires(const T &v) {
            { v.clone() } noexcept;
        } || requires(const T &v) {
            { clone(v) } noexcept;
        } || (std::is_trivially_copyable_v<T> && std::is_nothrow_copy_constructible_v<T>);

        template <cloneable T>
        constexpr T clone_value(const T &value) noexcept(noexcept_cloneable<T>) {
            if constexpr (has_member_clone<T>) {
                return value.clone();
            } else if constexpr (has_adl_clone<T>) {
                return clone(value);
            } else {
                static_assert(
                    std::is_trivially_copyable_v<T>,
                    "Type must either have a .clone() member function, an ADL-findable clone() free function, "
                    "or be trivially copyable, because it is "
                    "impossible to determine whether a user-defined copy constructor performs a deep copy.");
                return value;
            }
        }

        struct within_invoke_t {
            constexpr explicit within_invoke_t() = default;
        };

        inline constexpr within_invoke_t within_invoke{};

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

        template <typename T>
        concept not_void = !std::is_void_v<T>;

        inline namespace swap {
            using std::swap;

            template <typename T>
            concept cpp17_swappable = requires(T &a, T &b) { requires requires() { swap(a, b); }; };
        } // namespace swap

#if defined(__cpp_lib_reference_from_temporary) && __cpp_lib_reference_from_temporary >= 202202L
        template <class T, class U>
        constexpr bool cpp23_reference_constructs_from_temporary_v = std::reference_constructs_from_temporary_v<T, U>;
#elif defined(__clang__) && defined(__has_builtin) && __has_builtin(__reference_constructs_from_temporary)
        template <class T, class U>
        constexpr bool cpp23_reference_constructs_from_temporary_v = __reference_constructs_from_temporary(T, U);
#else
        // Fallback
        template <class T, class U>
        constexpr bool cpp23_reference_constructs_from_temporary_v = false;
#endif

#pragma push_macro("has_cpp_lib_optional_ref")
#if (defined(__cpp_lib_optional) && __cpp_lib_optional >= 202506L)                                                     \
    || (defined(__cpp_lib_freestanding_optional) && __cpp_lib_freestanding_optional >= 202506L)
    #define has_cpp_lib_optional_ref 1
#else
    #define has_cpp_lib_optional_ref 0
#endif

        template <typename U, typename T>
        concept non_std_optional_of =
#if has_cpp_lib_optional_ref
            !std::same_as<std::remove_cvref_t<U>, std::optional<T>>;
#else
            true;
#endif
    } // namespace detail
} // namespace opt

#pragma pop_macro("has_cpp_lib_optional_ref")
