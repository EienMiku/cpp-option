-- Shared build settings for the `option` project.
-- Call `opt_defaults()` once at root scope (it applies to every target in the
-- file); use the `opt_use_*()` helpers inside the targets that need them.

-- Defaults applied to all targets.
function opt_defaults()
    add_rules("mode.debug", "mode.release")
    set_languages("c++latest")
    set_warnings("allextra")
    set_encodings("source:utf-8", "target:utf-8")
    add_includedirs(path.join(os.projectdir(), "include"))
end

-- The option module units (for targets that `import option;`).
function opt_use_module()
    add_files(path.join(os.projectdir(), "src/option.cppm"))
    add_files(path.join(os.projectdir(), "src/option/*.cppm"))
end

-- libc++ test-support headers (test_macros.h, archetypes.h, ...) for test_unit_full*.
function opt_use_libcxx_test_support()
    add_includedirs(path.join(os.projectdir(), "external/llvm-project/libcxx/test/support"))
end

-- Project test-support headers (test_death.hpp).
function opt_use_test_support()
    add_includedirs(path.join(os.projectdir(), "tests/support"))
end
