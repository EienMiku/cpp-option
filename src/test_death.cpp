// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <utility>

#include <test_death.hpp>

#include "option.hpp"

using namespace opt;

struct S {
    int value;
};

void test_nullopt_operator_arrow() {
    option<S> o;
    (void) o->value;
}

void test_nullopt_operator_arrow_const() {
    const option<S> o;
    (void) o->value;
}

void test_nullopt_operator_star_lvalue() {
    option<S> o;
    (void) *o;
}

void test_nullopt_operator_star_const_lvalue() {
    const option<S> o;
    (void) *o;
}

void test_nullopt_operator_star_rvalue() {
    option<S> o;
    (void) *std::move(o);
}

void test_nullopt_operator_star_const_rvalue() {
    const option<S> o;
    (void) *std::move(o);
}

int main(int argc, char* argv[]) {
    std_testing::death_test_executive exec;

// #if _ITERATOR_DEBUG_LEVEL != 0
    exec.add_death_tests({
        test_nullopt_operator_arrow,
        test_nullopt_operator_arrow_const,
        test_nullopt_operator_star_lvalue,
        test_nullopt_operator_star_const_lvalue,
        test_nullopt_operator_star_rvalue,
        test_nullopt_operator_star_const_rvalue,
    });
// #endif // _ITERATOR_DEBUG_LEVEL != 0

    return exec.run(argc, argv);
}