includes("xmake.common.lua")
opt_defaults()
set_languages("clatest", "cxxlatest")
set_toolchains("clang")
set_runtimes("c++_static")
add_cxxflags("-stdlib=libc++")
add_ldflags("-stdlib=libc++", "-lc++abi", {force = true})

local libcxx = {
    configs = {
        cxx_compiler = "clang++",
        cxxflags = "-stdlib=libc++",
        ldflags = "-stdlib=libc++"
    },
    system = false
}
add_requires("gtest", libcxx)
add_requires("benchmark", libcxx)

-- Clang (libc++)
target("test_unit_clang")
    set_kind("binary")
    add_files("tests/test_unit.cpp")
    add_packages("gtest")

target("bench_clang")
    set_kind("binary")
    add_files("benchmarks/bench.cpp")
    add_packages("benchmark")
    set_optimize("fastest")

target("bench_debug_opt_clang")
    set_kind("binary")
    add_files("benchmarks/bench.cpp")
    add_packages("benchmark")
    set_optimize("fastest")
    set_symbols("debug")
    add_cxxflags("-march=x86-64-v3", "-mtune=generic")
