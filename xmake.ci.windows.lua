includes("xmake.common.lua")
opt_defaults()
set_toolchains("msvc")

add_requires("gtest", {system = false})
add_requires("benchmark", {system = false})

-- MSVC (Windows)
target("test_unit_msvc")
    set_kind("binary")
    add_files("tests/test_unit.cpp")
    add_packages("gtest")

target("bench_msvc")
    set_kind("binary")
    add_files("benchmarks/bench.cpp")
    add_packages("benchmark")
    set_optimize("fastest")
