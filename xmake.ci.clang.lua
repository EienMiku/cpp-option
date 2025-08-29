add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

set_languages("clatest", "cxxlatest")

add_requires("gtest", {
    configs = {
        cxx_compiler = "clang++-20",
        cxxflags = "-stdlib=libc++",
        ldflags = "-stdlib=libc++"
    },
    system = false
})
add_requires("benchmark", {
    configs = {
        cxx_compiler = "clang++-20",
        cxxflags = "-stdlib=libc++",
        ldflags = "-stdlib=libc++"
    },
    system = false
})

-- Clang (libc++) test

target("test_unit_clang")
    set_kind("binary")
    add_files("src/test_unit.cpp")
    add_packages("gtest")
    add_includedirs("src/include")
    set_toolchains("clang-20")
    set_runtimes("c++_static")
    add_cxxflags("-stdlib=libc++")
    set_encodings("source:utf-8", "target:utf-8")

target("test_unit_v2_clang")
    set_kind("binary")
    add_files("src/test_unit_v2.cpp")
    add_includedirs("src/include")
    add_includedirs("external/llvm-project/libcxx/test/support")
    add_cxxflags("-stdlib=libc++")
    set_encodings("source:utf-8", "target:utf-8")

target("bench_clang")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_toolchains("clang-20")
    set_runtimes("c++_static")
    add_cxxflags("-stdlib=libc++")
    set_encodings("source:utf-8", "target:utf-8")

target("bench_debug_opt_clang")
    set_kind("binary")
    add_files("src/bench.cpp")
    add_includedirs("src/include")
    add_packages("benchmark")
    set_optimize("fastest")
    set_symbols("debug")
    set_toolchains("clang-20")
    set_runtimes("c++_static")
    add_cxxflags("-stdlib=libc++")
    set_encodings("source:utf-8", "target:utf-8")