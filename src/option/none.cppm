export module option:none;

import std;
import :fwd;

export namespace opt {
    struct none_t {
        constexpr explicit none_t() = default;

        template <detail::option_type T>
        constexpr auto and_(T &&) const noexcept -> T {
            return T{};
        }

        template <detail::option_type T>
        constexpr auto or_(T &&optb) const noexcept -> T {
            if (optb.is_some()) {
                return std::forward<decltype(optb)>(optb);
            }
            return T{};
        }

        template <detail::option_type T>
        constexpr auto xor_(T &&optb) const noexcept -> T {
            if (optb.is_some()) {
                return std::forward<decltype(optb)>(optb);
            }
            return T{};
        }

        constexpr auto eq(const none_t &) const noexcept -> bool {
            return true;
        }

        template <typename T>
        constexpr auto eq(const option<T> &other) const noexcept -> bool {
            return *this == other;
        }

        constexpr auto ne(const none_t &) const noexcept -> bool {
            return false;
        }

        template <typename T>
        constexpr auto ne(const option<T> &other) const noexcept -> bool {
            return *this != other;
        }

        constexpr auto lt(const none_t &) const noexcept -> bool {
            return false;
        }

        template <typename T>
        constexpr auto lt(const option<T> &other) const noexcept -> bool {
            return *this < other;
        }

        constexpr auto le(const none_t &) const noexcept -> bool {
            return true;
        }

        template <typename T>
        constexpr auto le(const option<T> &other) const noexcept -> bool {
            return *this <= other;
        }

        constexpr auto gt(const none_t &) const noexcept -> bool {
            return false;
        }

        template <typename T>
        constexpr auto gt(const option<T> &other) const noexcept -> bool {
            return *this > other;
        }

        constexpr auto ge(const none_t &) const noexcept -> bool {
            return true;
        }

        template <typename T>
        constexpr auto ge(const option<T> &other) const noexcept -> bool {
            return *this >= other;
        }
    };
} // namespace opt
