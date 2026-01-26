export module option:panic;

import std;

export namespace opt {
    class option_panic : public std::exception {
    public:
        constexpr explicit option_panic(const char *message) noexcept : message{ message } {}

        constexpr const char *what() const noexcept override {
            return message;
        }

    private:
        const char *message;
    };
} // namespace opt
