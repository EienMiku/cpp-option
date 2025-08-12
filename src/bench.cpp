// NOLINTBEGIN
#include "option.hpp"
#include <benchmark/benchmark.h>
#include <cstddef>
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
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            auto opt1 = opt::some(i % 1000);
            auto opt2 = (i % 2 == 0) ? opt::some(i % 500) : opt::none;
            sum += opt1.unwrap_or(0);
            sum += opt2.unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_massive);

static void BM_std_optional_massive(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            auto opt1 = std::make_optional(i % 1000);
            auto opt2 = (i % 2 == 0) ? std::make_optional(i % 500) : std::nullopt;
            sum += opt1.value_or(0);
            sum += opt2.value_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_massive);

static void BM_opt_option_string(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 500000; ++i) {
            auto opt1    = opt::some(std::string("test_") + std::to_string(i % 100));
            auto opt2    = (i % 3 == 0) ? opt::some(std::string("value")) : opt::none;
            auto result1 = opt1.map([](const auto &s) { return s.length(); });
            auto result2 = opt2.map([](const auto &s) { return s.length(); });
            sum += result1.unwrap_or(0L);
            sum += result2.unwrap_or(0L);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_string);

static void BM_std_optional_string(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 500000; ++i) {
            auto opt1    = std::make_optional(std::string("test_") + std::to_string(i % 100));
            auto opt2    = (i % 3 == 0) ? std::make_optional(std::string("value")) : std::nullopt;
            auto result1 = opt1.transform([](const auto &s) { return s.length(); });
            auto result2 = opt2.transform([](const auto &s) { return s.length(); });
            sum += result1.value_or(0L);
            sum += result2.value_or(0L);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_string);

static void BM_opt_option_chain(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            auto opt    = opt::some(i % 100);
            auto result = opt.map([](auto x) { return x * 2; });
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
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 1000000; ++i) {
            auto opt    = std::make_optional(i % 100);
            auto result = opt.transform([](auto x) { return x * 2; });
            if (result.has_value() && *result > 50) {
                result = std::make_optional(*result + 10);
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
    size_t sum = 0;
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
    size_t sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 10000; ++i) {
            std::vector<std::optional<int>> options;
            options.reserve(100);
            for (int j = 0; j < 100; ++j) {
                if (j % 2 == 0) {
                    options.emplace_back(std::make_optional(j));
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
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto nested = (i % 2 == 0) ? opt::some(opt::some(i)) : opt::none;
            sum += nested.flatten().unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_flatten);

static void BM_std_optional_flatten(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto nested = (i % 2 == 0) ? std::make_optional(std::make_optional(i)) : std::nullopt;
            // flatten: if nested.has_value() && nested->has_value() return value, else 0
            sum += (nested.has_value() && nested->has_value()) ? **nested : 0;
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_flatten);

static void BM_opt_option_ptr(benchmark::State &state) {
    int v      = 42;
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 10000000; ++i) {
            auto o = (i % 2 == 0) ? opt::some(&v) : opt::none;
            sum += o.unwrap_or(nullptr) ? 1 : 0;
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_ptr);

static void BM_std_optional_ptr(benchmark::State &state) {
    int v    = 42;
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 10000000; ++i) {
            auto o = (i % 2 == 0) ? std::make_optional(&v) : std::nullopt;
            sum += o.value_or(nullptr) ? 1 : 0;
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_ptr);

static void BM_opt_option_vector_move(benchmark::State &state) {
    for (auto _ : state) {
        std::vector v(100, 42);
        auto o = opt::some(std::move(v));
        benchmark::DoNotOptimize(o);
    }
}
BENCHMARK_EX(BM_opt_option_vector_move);

static void BM_std_optional_vector_move(benchmark::State &state) {
    for (auto _ : state) {
        std::vector v(100, 42);
        auto o = std::make_optional(std::move(v));
        benchmark::DoNotOptimize(o);
    }
}
BENCHMARK_EX(BM_std_optional_vector_move);

static void BM_opt_option_and(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto o1 = (i % 2 == 0) ? opt::some(i) : opt::none;
            auto o2 = (i % 3 == 0) ? opt::some(i * 2) : opt::none;
            sum += o1.and_(o2).unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_and);

static void BM_std_optional_and(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto o1     = (i % 2 == 0) ? std::make_optional(i) : std::nullopt;
            auto o2     = (i % 3 == 0) ? std::make_optional(i * 2) : std::nullopt;
            auto result = (o1.has_value() && o2.has_value()) ? o2 : std::nullopt;
            sum += result.value_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_and);

static void BM_opt_option_or(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto o1 = (i % 2 == 0) ? opt::some(i) : opt::none;
            auto o2 = (i % 3 == 0) ? opt::some(i * 2) : opt::none;
            sum += o1.or_(o2).unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_or);

static void BM_std_optional_or(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto o1     = (i % 2 == 0) ? std::make_optional(i) : std::nullopt;
            auto o2     = (i % 3 == 0) ? std::make_optional(i * 2) : std::nullopt;
            auto result = o1.has_value() ? o1 : o2;
            sum += result.value_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_or);

static void BM_opt_option_xor(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto o1 = (i % 2 == 0) ? opt::some(i) : opt::none;
            auto o2 = (i % 3 == 0) ? opt::some(i * 2) : opt::none;
            sum += o1.xor_(o2).unwrap_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_opt_option_xor);

static void BM_std_optional_xor(benchmark::State &state) {
    size_t sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000000; ++i) {
            auto o1     = (i % 2 == 0) ? std::make_optional(i) : std::nullopt;
            auto o2     = (i % 3 == 0) ? std::make_optional(i * 2) : std::nullopt;
            auto result = (o1.has_value() != o2.has_value()) ? (o1.has_value() ? o1 : o2) : std::nullopt;
            sum += result.value_or(0);
        }
    }
    benchmark::DoNotOptimize(sum);
}
BENCHMARK_EX(BM_std_optional_xor);

BENCHMARK_MAIN();
// NOLINTEND