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

#pragma once

#include <unordered_set>

#include "subspace/iter/compat_ranges.h"
#include "subspace/iter/from_iterator.h"
#include "subspace/iter/iterator.h"

template <class Key, class Hash, class KeyEqual, class Allocator>
struct sus::iter::FromIteratorImpl<
    std::unordered_set<Key, Hash, KeyEqual, Allocator>> {
  static constexpr std::unordered_set<Key, Hash, KeyEqual, Allocator> from_iter(
      ::sus::iter::IntoIterator<Key> auto&& into_iter) noexcept {
    auto&& iter = sus::move(into_iter).into_iter();
    auto s = std::unordered_set<Key, Hash, KeyEqual, Allocator>();
    for (Key k: iter) s.insert(k);
    return s;
  }
};
