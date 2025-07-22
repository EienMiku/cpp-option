add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

set_languages("c++latest")

add_requires("gtest", {system = false})
add_requires("benchmark", {system = false})

-- Clang (libc++) test

target("test_unit_clang")
    set_kind("binary")
    add_files("src/test_unit.cpp")
    add_packages("gtest")
    add_includedirs("src/include")
    set_toolchains("clang-20")
    set_runtimes("c++_static")
    add_cxxflags("-Wall", "-Wextra", "-Wpedantic")
    add_cxxflags("-stdlib=libc++")
    add_cxxflags("-finput-charset=utf-8", "-fexec-charset=utf-8")

target("bench_clang")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_toolchains("clang-20")
    set_runtimes("c++_static")
    add_cxxflags("-Wall", "-Wextra", "-Wpedantic")
    add_cxxflags("-stdlib=libc++")
    add_cxxflags("-finput-charset=utf-8", "-fexec-charset=utf-8")
