includes("xmake.common.lua")
opt_defaults()

add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
set_toolchains("clang")

add_requires("benchmark", {optional = true})

-- Local scratchpad: git-ignored, and the target only exists if the file is present.
if os.exists("src/scratch.cppm") then
target("scratch")
    set_kind("binary")
    add_files("src/scratch.cppm")
end

target("test_unit_full")
    set_kind("binary")
    add_files("tests/test_unit_full.cpp")
    opt_use_libcxx_test_support()

target("test_unit_ext")
    set_kind("binary")
    add_files("tests/test_unit_ext.cpp")

target("test_unit_ext_module")
    set_kind("binary")
    add_files("tests/test_unit_ext_module.cppm")
    opt_use_module()

target("test_unit_full_module")
    set_kind("binary")
    add_files("tests/test_unit_full_module.cppm")
    opt_use_module()
    opt_use_libcxx_test_support()

target("test_death")
    set_kind("binary")
    add_files("tests/test_death.cpp")
    opt_use_test_support()

target("bench")
    set_kind("binary")
    set_runtimes("MD")
    add_files("benchmarks/bench.cpp")
    add_packages("benchmark")
    set_optimize("fastest")
