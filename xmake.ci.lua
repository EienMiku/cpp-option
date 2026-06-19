-- Unified CI build config; the toolchain selects which target set is defined:
--   xmake f --toolchain=gcc   --file=xmake.ci.lua   (libstdc++)
--   xmake f --toolchain=clang --file=xmake.ci.lua   (libc++)
--   xmake f --toolchain=msvc  --file=xmake.ci.lua   (MSVC STL)

includes("xmake.common.lua")
opt_defaults()

if is_config("toolchain", "clang") then
    -- Clang (libc++)
    set_languages("clatest", "cxxlatest")
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

elseif is_config("toolchain", "gcc") then
    -- GCC (libstdc++)
    set_languages("clatest", "cxxlatest")
    add_requires("gtest", {system = false})
    add_requires("benchmark", {system = false})

    target("test_unit_gcc")
        set_kind("binary")
        add_files("tests/test_unit.cpp")
        add_packages("gtest")

    target("bench_gcc")
        set_kind("binary")
        add_files("benchmarks/bench.cpp")
        add_packages("benchmark")
        set_optimize("fastest")
        add_cxxflags("-fno-rtti", "-march=native", "-mtune=native", "-flto")
        add_ldflags("-flto")

    target("bench_debug_opt_gcc")
        set_kind("binary")
        add_files("benchmarks/bench.cpp")
        add_packages("benchmark")
        set_optimize("fastest")
        set_symbols("debug")
        add_cxxflags("-march=x86-64-v3", "-mtune=generic")

elseif is_config("toolchain", "msvc") then
    -- MSVC (Windows)
    add_requires("gtest", {system = false})
    add_requires("benchmark", {system = false})

    target("test_unit_msvc")
        set_kind("binary")
        add_files("tests/test_unit.cpp")
        add_packages("gtest")

    target("bench_msvc")
        set_kind("binary")
        add_files("benchmarks/bench.cpp")
        add_packages("benchmark")
        set_optimize("fastest")
end
