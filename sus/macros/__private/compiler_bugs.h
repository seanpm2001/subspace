// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// IWYU pragma: private
// IWYU pragma: friend "sus/.*"
#pragma once

#include "sus/macros/builtin.h"
#include "sus/macros/compiler.h"

// TODO: https://github.com/llvm/llvm-project/issues/56394
// Aggregate initialization can't deduce template types.
#if defined(__clang__) && \
    __clang_major__ > 0  // TODO: Update when the bug is fixed.
#define sus_clang_bug_56394(...) __VA_ARGS__
#define sus_clang_bug_56394_else(...)
#else
#define sus_clang_bug_56394(...)
#define sus_clang_bug_56394_else(...) __VA_ARGS__
#endif

// TODO: This is not actually a bug, remove this.
// TODO: https://github.com/llvm/llvm-project/issues/58835
#if defined(__clang__) && \
    __clang_major__ > 0  // TODO: Update when the bug is fixed.
#define sus_clang_bug_58835(...) __VA_ARGS__
#define sus_clang_bug_58835_else(...)
#else
#define sus_clang_bug_58835(...)
#define sus_clang_bug_58835_else(...) __VA_ARGS__
#endif

// TODO: https://github.com/llvm/llvm-project/issues/54040
// Aggregate initialization via () paren syntax.
//
// Support for this landed and was reverted in clang 16, then relanded but is
// currently broken in 16:
// https://github.com/llvm/llvm-project/issues/61145
#if defined(__clang__) && (__clang_major__ <= 17)
#define sus_clang_bug_54040(...) __VA_ARGS__
#define sus_clang_bug_54040_else(...)
#else
#define sus_clang_bug_54040(...)
#define sus_clang_bug_54040_else(...) __VA_ARGS__
#endif

// TODO: https://github.com/llvm/llvm-project/issues/54050
// Aggregate initialization fails on template classes due to lack of CTAD for
// aggregates.
//
// There are still bugs with aggregate init in Clang 17:
// https://github.com/llvm/llvm-project/issues/61145
#if defined(__clang__) && \
    __clang_major__ <= 17  // TODO: Update when the bug is fixed.
#define sus_clang_bug_54050(...) __VA_ARGS__
#define sus_clang_bug_54050_else(...)
#else
#define sus_clang_bug_54050(...)
#define sus_clang_bug_54050_else(...) __VA_ARGS__
#endif

// GCC internal compiler error when StrongOrd fails.
// TODO: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107542
// Fixed in GCC 12.3 and 13.x.
#if defined(__GNUC__) && \
    !(__GNUC__ >= 13 || (__GNUC__ == 12 && __GNUC_MINOR__ >= 3))
#define sus_gcc_bug_107542(...) __VA_ARGS__
#define sus_gcc_bug_107542_else(...)
#else
#define sus_gcc_bug_107542(...)
#define sus_gcc_bug_107542_else(...) __VA_ARGS__
#endif

// TODO: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108169
// TODO: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99631
// GCC considers class-type template parameters as const.
// Fixed in 13.3 and 14.
#if defined(__GNUC__) && \
    !(__GNUC__ >= 14 || (__GNUC__ == 13 && __GNUC_MINOR__ >= 3))
#define sus_gcc_bug_108169(...) __VA_ARGS__
#define sus_gcc_bug_108169_else(...)
#else
#define sus_gcc_bug_108169(...)
#define sus_gcc_bug_108169_else(...) __VA_ARGS__
#endif

// TODO: https://github.com/llvm/llvm-project/issues/49358
// Clang-cl doesn't support either [[no_unique_address]] nor
// [[msvc::no_unique_address]]
#if _MSC_VER && defined(__clang__) && \
    __clang_major__ > 0  // TODO: Update when the bug is fixed.
#define sus_clang_bug_49358(...) __VA_ARGS__
#define sus_clang_bug_49358_else(...)
#else
#define sus_clang_bug_49358(...)
#define sus_clang_bug_49358_else(...) __VA_ARGS__
#endif

// TODO: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110245
// GCC can't constexpr initialize Option without a value.
#if defined(__GNUC__) && __GNUC__ > 0  // TODO: Update when the bug is fixed.
#define sus_gcc_bug_110245(...) __VA_ARGS__
#define sus_gcc_bug_110245_else(...)
#else
#define sus_gcc_bug_110245(...)
#define sus_gcc_bug_110245_else(...) __VA_ARGS__
#endif

// TODO:
// https://developercommunity.visualstudio.com/t/MSVC-produces-incorrect-alignment-with-/10416202
// MSVC will give the struct an incorrect alignment when
// [[msvc::no_unique_address]] is above an array.
#if _MSC_VER && !defined(__clang__) && \
    _MSC_VER <= 1934  // TODO: Get a better version when it's fixed.
#define sus_msvc_bug_10416202(...) __VA_ARGS__
#define sus_msvc_bug_10416202_else(...)
#else
#define sus_msvc_bug_10416202(...)
#define sus_msvc_bug_10416202_else(...) __VA_ARGS__
#endif

// TODO: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110905
// GCC can't constant-evaluate code that sets an Option multiple times.
#if defined(__GNUC__) && __GNUC__ > 0  // TODO: Update when the bug is fixed.
#define sus_gcc_bug_110905(...) __VA_ARGS__
#define sus_gcc_bug_110905_else(...)
#else
#define sus_gcc_bug_110905(...)
#define sus_gcc_bug_110905_else(...) __VA_ARGS__
#endif

// TODO: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110927
// GCC does not parse dependent types in a partial specialization.
// Fixed in GCC 13.3.
#if defined(__GNUC__) && \
    (__GNUC__ < 13 || (__GNUC__ == 13 && __GNUC_MINOR__ < 3))
#define sus_gcc_bug_110927(...) __VA_ARGS__
#define sus_gcc_bug_110927_else(...)
#define sus_gcc_bug_110927_exists
#else
#define sus_gcc_bug_110927(...)
#define sus_gcc_bug_110927_else(...) __VA_ARGS__
#endif
