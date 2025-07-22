#include "option.hpp"
#include <benchmark/benchmark.h>
#include <optional>
#include <string>
#include <vector>

#ifndef BENCHMARK_EX
    #ifdef BENCHMARK_AGGREGATE
        #define BENCHMARK_EX(x) BENCHMARK(x)->Repetitions(10)->ReportAggregatesOnly(true)
    #else
        #define BENCHMARK_EX(x) BENCHMARK(x)
    #endif
#endif

static void BM_opt_option_massive(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            opt::option<int> opt1 = opt::some(static_cast<int>(i % 1000));
            opt::option<int> opt2 = (i % 2 == 0) ? opt::some(static_cast<int>(i % 500)) : opt::none;
            sum += opt1.unwrap_or(0);
            sum += opt2.unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_massive);

static void BM_std_optional_massive(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            std::optional<int> opt1 = static_cast<int>(i % 1000);
            std::optional<int> opt2 = (i % 2 == 0) ? std::optional<int>(static_cast<int>(i % 500)) : std::nullopt;
            sum += opt1.value_or(0);
            sum += opt2.value_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_massive);

static void BM_opt_option_string(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 500000; ++i) {
            opt::option<std::string> opt1 = opt::some(std::string("test_") + std::to_string(i % 100));
            opt::option<std::string> opt2 = (i % 3 == 0) ? opt::some(std::string("value")) : opt::none;
            auto result1 = opt1.map([](const std::string &s) { return static_cast<long>(s.length()); });
            auto result2 = opt2.map([](const std::string &s) { return static_cast<long>(s.length()); });
            sum += result1.unwrap_or(0L);
            sum += result2.unwrap_or(0L);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_string);

static void BM_std_optional_string(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 500000; ++i) {
            std::optional<std::string> opt1 = std::string("test_") + std::to_string(i % 100);
            std::optional<std::string> opt2 = (i % 3 == 0) ? std::optional<std::string>("value") : std::nullopt;
            auto result1                    = opt1.has_value() ? std::optional<long>(opt1->length()) : std::nullopt;
            auto result2                    = opt2.has_value() ? std::optional<long>(opt2->length()) : std::nullopt;
            sum += result1.value_or(0L);
            sum += result2.value_or(0L);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_string);

static void BM_opt_option_chain(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            opt::option<int> opt = opt::some(static_cast<int>(i % 100));
            auto result          = opt.map([](int x) { return x * 2; });
            if (result.is_some() && result.unwrap() > 50) {
                result = opt::some(result.unwrap() + 10);
            } else {
                result = opt::none;
            }
            sum += result.unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_chain);

static void BM_std_optional_chain(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            std::optional<int> opt = static_cast<int>(i % 100);
            auto result            = opt.has_value() ? std::optional<int>(*opt * 2) : std::nullopt;
            if (result.has_value() && *result > 50) {
                result = *result + 10;
            } else {
                result = std::nullopt;
            }
            sum += result.value_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_chain);

static void BM_opt_option_memory(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            std::vector<opt::option<int>> options;
            options.reserve(100);
            for (int j = 0; j < 100; ++j) {
                if (j % 2 == 0) {
                    options.emplace_back(opt::some(j));
                } else {
                    options.emplace_back(opt::none);
                }
            }
            for (auto &opt : options) {
                sum += opt.unwrap_or(0);
            }
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_memory);

static void BM_std_optional_memory(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            std::vector<std::optional<int>> options;
            options.reserve(100);
            for (int j = 0; j < 100; ++j) {
                if (j % 2 == 0) {
                    options.emplace_back(j);
                } else {
                    options.emplace_back(std::nullopt);
                }
            }
            for (auto &opt : options) {
                sum += opt.value_or(0);
            }
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_memory);

static void BM_opt_option_unwrap_none(benchmark::State &state) {
    opt::option<int> none = opt::none;
    int cnt               = 0;
    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            try {
                cnt += none.unwrap();
            } catch (...) {
                cnt += 1;
            }
        }
    }
    benchmark::DoNotOptimize(cnt);
}
BENCHMARK_EX(BM_opt_option_unwrap_none);

static void BM_std_optional_value_nullopt(benchmark::State &state) {
    std::optional<int> none = std::nullopt;
    int cnt                 = 0;
    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            try {
                cnt += none.value();
            } catch (...) {
                cnt += 1;
            }
        }
    }
    benchmark::DoNotOptimize(cnt);
}
BENCHMARK_EX(BM_std_optional_value_nullopt);

static void BM_opt_option_flatten(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            opt::option<opt::option<int>> nested =
                (i % 2 == 0) ? opt::some(opt::some(i)) : opt::none_opt<opt::option<int>>();
            sum += nested.flatten().unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_flatten);

static void BM_std_optional_flatten(benchmark::State &state) {
    long sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            std::optional<std::optional<int>> nested =
                (i % 2 == 0) ? std::optional<std::optional<int>>(std::optional<int>(i)) : std::nullopt;
            // flatten: if nested.has_value() && nested->has_value() return value, else 0
            sum += (nested.has_value() && nested->has_value()) ? **nested : 0;
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_flatten);

static void BM_opt_option_ptr(benchmark::State &state) {
    int v    = 42;
    long sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 10000000; ++i) {
            opt::option<int *> o = (i % 2 == 0) ? opt::some(&v) : opt::none_opt<int *>();
            sum += o.unwrap_or(nullptr) ? 1 : 0;
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_ptr);

static void BM_std_optional_ptr(benchmark::State &state) {
    int v    = 42;
    long sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 10000000; ++i) {
            std::optional<int *> o = (i % 2 == 0) ? std::optional<int *>(&v) : std::nullopt;
            sum += o.value_or(nullptr) ? 1 : 0;
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_ptr);

static void BM_opt_option_vector_move(benchmark::State &state) {
    for (auto _ : state) {
        std::vector<int> v(100, 42);
        opt::option<std::vector<int>> o = opt::some(std::move(v));
        benchmark::DoNotOptimize(o);
    }
}
BENCHMARK_EX(BM_opt_option_vector_move);

static void BM_std_optional_vector_move(benchmark::State &state) {
    for (auto _ : state) {
        std::vector<int> v(100, 42);
        std::optional<std::vector<int>> o = std::move(v);
        benchmark::DoNotOptimize(o);
    }
}
BENCHMARK_EX(BM_std_optional_vector_move);

BENCHMARK_MAIN();
