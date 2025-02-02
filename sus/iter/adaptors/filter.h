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

// IWYU pragma: private, include "sus/iter/iterator.h"
// IWYU pragma: friend "sus/.*"
#pragma once

#include "sus/fn/fn_concepts.h"
#include "sus/iter/iterator_defn.h"
#include "sus/mem/move.h"
#include "sus/mem/relocate.h"

namespace sus::iter {

using ::sus::mem::relocate_by_memcpy;

/// An iterator that filters based on a predicate function.
///
/// This type is returned from `Iterator::filter()`.
template <class InnerSizedIter, class Pred>
class [[nodiscard]] Filter final
    : public IteratorBase<Filter<InnerSizedIter, Pred>,
                          typename InnerSizedIter::Item> {
 public:
  using Item = InnerSizedIter::Item;

  static_assert(
      ::sus::fn::FnMut<Pred, bool(const std::remove_reference_t<Item>&)>);

  // Type is Move and (can be) Clone.
  Filter(Filter&&) = default;
  Filter& operator=(Filter&&) = default;

  // sus::mem::Clone trait.
  constexpr Filter clone() const noexcept
    requires(::sus::mem::Clone<Pred> &&  //
             ::sus::mem::Clone<InnerSizedIter>)
  {
    return Filter(sus::clone(pred_), sus::clone(next_iter_));
  }

  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    while (true) {
      Option<Item> item = next_iter_.next();
      if (item.is_none() || ::sus::fn::call_mut(pred_, item.as_value()))
        return item;
    }
  }
  /// sus::iter::Iterator trait.
  constexpr SizeHint size_hint() const noexcept {
    // Can't know a lower bound, due to the predicate.
    return SizeHint(0u, next_iter_.size_hint().upper);
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept
    requires(DoubleEndedIterator<InnerSizedIter, Item>)
  {
    // TODO: Just call find(pred) on itself?
    while (true) {
      Option<Item> item = next_iter_.next_back();
      if (item.is_none() || ::sus::fn::call_mut(pred_, item.as_value()))
        return item;
    }
  }

 private:
  template <class U, class V>
  friend class IteratorBase;

  explicit constexpr Filter(Pred&& pred, InnerSizedIter&& next_iter) noexcept
      : pred_(::sus::move(pred)), next_iter_(::sus::move(next_iter)) {}

  Pred pred_;
  InnerSizedIter next_iter_;

  sus_class_trivially_relocatable_if_types(::sus::marker::unsafe_fn,
                                           decltype(pred_),
                                           decltype(next_iter_));
};

}  // namespace sus::iter
