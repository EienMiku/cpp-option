add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

set_languages("clatest", "cxxlatest")

add_requires("gtest", {system = false})
add_requires("benchmark", {system = false})

-- GCC (libstdc++) test

target("test_unit_gcc")
    set_kind("binary")
    add_files("src/test_unit.cpp")
    add_packages("gtest")
    add_includedirs("src/include")
    set_toolchains("gcc")
    set_encodings("source:utf-8", "target:utf-8")

-- Please run this target with optimal environment if you like
-- target("test_unit_v2_clang")
--     set_kind("binary")
--     add_files("src/test_unit_v2.cpp")
--     add_includedirs("src/include")
--     add_includedirs("external/llvm-project/libcxx/test/support")
--     set_toolchains("gcc")
--     set_encodings("source:utf-8", "target:utf-8")

target("bench_gcc")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_toolchains("gcc")
    add_cxxflags("-fno-exceptions", "-fno-rtti", "-march=native", "-mtune=native", "-flto")
    add_ldflags("-flto")
    set_encodings("source:utf-8", "target:utf-8")

target("bench_debug_opt_gcc")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_symbols("debug")
    set_toolchains("gcc")
    set_encodings("source:utf-8", "target:utf-8")