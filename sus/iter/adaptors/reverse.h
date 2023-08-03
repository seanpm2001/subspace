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

// IWYU pragma: private, include "sus/iter/iterator.h"
// IWYU pragma: friend "sus/.*"
#pragma once

#include "sus/fn/fn_box_defn.h"
#include "sus/iter/iterator_concept.h"
#include "sus/iter/iterator_defn.h"
#include "sus/mem/move.h"
#include "sus/mem/relocate.h"
#include "sus/mem/size_of.h"

namespace sus::iter {

using ::sus::mem::relocate_by_memcpy;

/// An iterator that iterates over another iterator but in reverse.
///
/// The iterator wrapped by Reverse must be a DoubleEndedIterator.
///
/// This type is returned from `Iterator::rev()`.
template <class InnerSizedIter>
class [[nodiscard]] Reverse final
    : public IteratorBase<Reverse<InnerSizedIter>,
                          typename InnerSizedIter::Item> {
  static_assert(
      DoubleEndedIterator<InnerSizedIter, typename InnerSizedIter::Item>);

 public:
  using Item = InnerSizedIter::Item;

  /// sus::iter::Iterator trait.
  Option<Item> next() noexcept { return next_iter_.next_back(); }
  /// sus::iter::Iterator trait.
  SizeHint size_hint() const noexcept { return next_iter_.size_hint(); }
  /// sus::iter::DoubleEndedIterator trait.
  Option<Item> next_back() noexcept { return next_iter_.next(); }
  /// sus::iter::ExactSizeIterator trait.
  usize exact_size_hint() const noexcept
    requires(ExactSizeIterator<InnerSizedIter, Item>)
  {
    return next_iter_.exact_size_hint();
  }

 private:
  template <class U, class V>
  friend class IteratorBase;

  static Reverse with(InnerSizedIter&& next_iter) noexcept {
    return Reverse(::sus::move(next_iter));
  }

  Reverse(InnerSizedIter&& next_iter) : next_iter_(::sus::move(next_iter)) {}

  InnerSizedIter next_iter_;

  sus_class_trivially_relocatable_if_types(::sus::marker::unsafe_fn,
                                           decltype(next_iter_));
};

}  // namespace sus::iter
