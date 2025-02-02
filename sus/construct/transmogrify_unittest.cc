// Copyright 2023 Google LLC
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

#include "sus/construct/transmogrify.h"

#include "googletest/include/gtest/gtest.h"
#include "sus/prelude.h"

namespace {

TEST(Transmogrify, Example_Concept) {
  auto add = [](u32 a, const sus::construct::Transmogrify<u32> auto& b) -> u32 {
    return a.wrapping_add(sus::mog<u32>(b));
  };
  sus::check(add(3_u32, -1_i32) == u32::MIN + 2u);
}

TEST(Transmogrify, Example_Function) {
  sus::check(u32::MAX == sus::mog<u32>(-1_i64));
}

}  // namespace
