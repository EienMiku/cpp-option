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
    set_warnings("allextra")

target("bench_gcc")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_toolchains("gcc")
    set_encodings("source:utf-8", "target:utf-8")
    set_warnings("allextra")

target("bench_debug_opt_gcc")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_symbols("debug")
    set_toolchains("gcc")
    set_encodings("source:utf-8", "target:utf-8")
    set_warnings("allextra")