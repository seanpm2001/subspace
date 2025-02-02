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

// IWYU pragma: private, include "sus/collections/slice.h"
// IWYU pragma: friend "sus/.*"
#pragma once

#include "sus/iter/iterator_defn.h"
#include "sus/iter/iterator_ref.h"
#include "sus/lib/__private/forward_decl.h"
#include "sus/macros/no_unique_address.h"
#include "sus/num/unsigned_integer.h"

namespace sus::collections {

/// An iterator over a slice in (non-overlapping) chunks (`chunk_size` elements
/// at a time), starting at the beginning of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last slice
/// of the iteration will be the remainder.
///
/// This struct is created by the `chunks()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] Chunks final
    : public ::sus::iter::IteratorBase<Chunks<ItemT>,
                                       ::sus::collections::Slice<ItemT>> {
 public:
  // `Item` is a `Slice<T>`.
  using Item = ::sus::collections::Slice<ItemT>;

 private:
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  // sus::iter::Iterator trait.
  Option<Item> next() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return Option<Item>();
    } else {
      auto chunksz = ::sus::cmp::min(v_.len(), chunk_size_);
      auto [fst, snd] =
          v_.split_at_unchecked(::sus::marker::unsafe_fn, chunksz);
      v_ = snd;
      return Option<Item>(fst);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  Option<Item> next_back() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      const auto len = v_.len();
      const auto remainder = len % chunk_size_;
      const auto chunksz = remainder != 0u ? remainder : chunk_size_;
      // SAFETY: `split_at_unchecked()` requires the argument be less than or
      // equal to the length. This is guaranteed, but subtle: `chunksz`
      // will always either be `v_.len() % chunk_size_`, which will always
      // evaluate to strictly less than `v_.len()` (or panic, in the case that
      // `chunk_size_` is zero), or it can be `chunk_size_`, in the case that
      // the length is exactly divisible by the chunk size.
      //
      // While it seems like using `chunk_size_` in this case could lead to a
      // value greater than `v_.len()`, it cannot: if `chunk_size_` were greater
      // than `v_.len()`, then `v_.len() % chunk_size_` would return nonzero
      // (note that in this branch of the `if`, we already know that `v_` is
      // non-empty).
      auto [fst, snd] =
          v_.split_at_unchecked(::sus::marker::unsafe_fn, len - chunksz);
      v_ = fst;
      return ::sus::Option<Item>(snd);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  ::sus::num::usize exact_size_hint() const noexcept {
    if (v_.is_empty()) {
      return 0u;
    } else {
      ::sus::num::usize n = v_.len() / chunk_size_;
      ::sus::num::usize rem = v_.len() % chunk_size_;
      return rem > 0u ? n + 1u : n;
    }
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by Slice, Vec, Array.
  friend class Slice<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  constexpr Chunks(::sus::iter::IterRef ref, const Slice<ItemT>& values,
                   ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)), v_(values), chunk_size_(chunk_size) {
    ::sus::check(chunk_size > 0u);
  }

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  Slice<ItemT> v_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(chunk_size_));
};

/// An iterator over mutable a slice in (non-overlapping) chunks (`chunk_size`
/// elements at a time), starting at the beginning of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last slice
/// of the iteration will be the remainder.
///
/// This struct is created by the `chunks_mut()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] ChunksMut final
    : public ::sus::iter::IteratorBase<ChunksMut<ItemT>,
                                       ::sus::collections::SliceMut<ItemT>> {
 public:
  // `Item` is a `SliceMut<T>`.
  using Item = ::sus::collections::SliceMut<ItemT>;

 private:
  static_assert(!std::is_reference_v<ItemT>);
  static_assert(!std::is_const_v<ItemT>);
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  // sus::iter::Iterator trait.
  Option<Item> next() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return Option<Item>();
    } else {
      const auto chunksz = ::sus::cmp::min(v_.len(), chunk_size_);
      auto [fst, snd] =
          v_.split_at_mut_unchecked(::sus::marker::unsafe_fn, chunksz);
      v_ = snd;
      return Option<SliceMut<ItemT>>(fst);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  Option<Item> next_back() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      const auto len = v_.len();
      const auto remainder = len % chunk_size_;
      const auto chunksz = remainder != 0u ? remainder : chunk_size_;
      // SAFETY: `split_at_unchecked()` requires the argument be less than or
      // equal to the length. This is guaranteed, but subtle: `chunksz`
      // will always either be `v_.len() % chunk_size_`, which will always
      // evaluate to strictly less than `v_.len()` (or panic, in the case that
      // `chunk_size_` is zero), or it can be `chunk_size_`, in the case that
      // the length is exactly divisible by the chunk size.
      //
      // While it seems like using `chunk_size_` in this case could lead to a
      // value greater than `v_.len()`, it cannot: if `chunk_size_` were greater
      // than `v_.len()`, then `v_.len() % chunk_size_` would return nonzero
      // (note that in this branch of the `if`, we already know that `v_` is
      // non-empty).
      auto [fst, snd] =
          v_.split_at_mut_unchecked(::sus::marker::unsafe_fn, len - chunksz);
      v_ = fst;
      return ::sus::Option<Item>(snd);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  ::sus::num::usize exact_size_hint() const noexcept {
    if (v_.is_empty()) {
      return 0u;
    } else {
      const auto n = v_.len() / chunk_size_;
      const auto rem = v_.len() % chunk_size_;
      return rem > 0u ? n + 1u : n;
    }
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by SliceMut, Vec, Array.
  friend class SliceMut<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  constexpr ChunksMut(::sus::iter::IterRef ref, const SliceMut<ItemT>& values,
                      ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)), v_(values), chunk_size_(chunk_size) {
    ::sus::check(chunk_size > 0u);
  }

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  SliceMut<ItemT> v_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(chunk_size_));
};

/// An iterator over a slice in (non-overlapping) chunks (`chunk_size` elements
/// at a time), starting at the beginning of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last up to
/// `chunk_size-1` elements will be omitted but can be retrieved from the
/// remainder function from the iterator.
///
/// This struct is created by the `chunks_exact()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] ChunksExact final
    : public ::sus::iter::IteratorBase<ChunksExact<ItemT>,
                                       ::sus::collections::Slice<ItemT>> {
 public:
  // `Item` is a `Slice<T>`.
  using Item = ::sus::collections::Slice<ItemT>;

 private:
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  /// Returns the remainder of the original slice that is not going to be
  /// returned by the iterator. The returned slice has at most `chunk_size-1`
  /// elements.
  [[nodiscard]] Item remainder() const& { return rem_; }

  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return Option<Item>();
    } else {
      // SAFETY: `split_at_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by the checking exactly that
      // in the condition above, and we are in the else branch.
      auto [fst, snd] =
          v_.split_at_unchecked(::sus::marker::unsafe_fn, chunk_size_);
      v_ = ::sus::move(snd);
      return Option<Item>(::sus::move(fst));
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      // SAFETY: `split_at_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by subtracting an unsigned (and
      // thus non-negative) value from the length.
      //
      // SAFETY: unchecked_sub() requires that result of the subtraction is <=
      // usize::MAX and >= usize::MIN. the len() is greater than chunk_size_
      // from the above if condition (we're in the else branch) so the result is
      // >= usize::MIN. Both values are positive and len() is <= usize::MAX
      // (from its type) so the result is <= usize::MAX.
      auto [fst, snd] = v_.split_at_unchecked(
          ::sus::marker::unsafe_fn,
          v_.len().unchecked_sub(::sus::marker::unsafe_fn, chunk_size_));
      v_ = ::sus::move(fst);
      return ::sus::Option<Item>(::sus::move(snd));
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  constexpr ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  constexpr ::sus::num::usize exact_size_hint() const noexcept {
    return v_.len() / chunk_size_;
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by Slice, Vec, Array.
  friend class Slice<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  static constexpr auto with_slice(::sus::iter::IterRef ref,
                                   const Slice<ItemT>& values,
                                   ::sus::num::usize chunk_size) noexcept {
    ::sus::check(chunk_size > 0u);
    auto rem = values.len() % chunk_size;
    // SAFETY: rem <= len() by construction above, so len() - rem is a
    // non-negative value <= len() <= usize:MAX.
    auto fst_len = values.len().unchecked_sub(::sus::marker::unsafe_fn, rem);
    // SAFETY: 0 <= fst_len <= values.len() by construction above.
    auto [fst, snd] =
        values.split_at_unchecked(::sus::marker::unsafe_fn, fst_len);
    return ChunksExact(CONSTRUCT, ::sus::move(ref), fst, snd, chunk_size);
  }

  enum Construct { CONSTRUCT };
  constexpr ChunksExact(Construct, ::sus::iter::IterRef ref,
                        const Slice<ItemT>& values,
                        const Slice<ItemT>& remainder,
                        ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)),
        v_(values),
        rem_(remainder),
        chunk_size_(chunk_size) {}

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  Slice<ItemT> v_;
  Slice<ItemT> rem_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(rem_),
                                  decltype(chunk_size_));
};

/// An iterator over a mutable slice in (non-overlapping) chunks (`chunk_size`
/// elements at a time), starting at the beginning of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last up to
/// `chunk_size-1` elements will be omitted but can be retrieved from the
/// remainder function from the iterator.
///
/// This struct is created by the `chunks_exact_mut()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] ChunksExactMut final
    : public ::sus::iter::IteratorBase<ChunksExactMut<ItemT>,
                                       ::sus::collections::SliceMut<ItemT>> {
 public:
  // `Item` is a `SliceMut<T>`.
  using Item = ::sus::collections::SliceMut<ItemT>;

 private:
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  /// Returns the remainder of the original slice that is not going to be
  /// returned by the iterator. The returned slice has at most `chunk_size-1`
  /// elements.
  [[nodiscard]] constexpr Item remainder() const& { return rem_; }

  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return Option<Item>();
    } else {
      // SAFETY: `split_at_mut_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by the checking exactly that
      // in the condition above, and we are in the else branch.
      auto [fst, snd] =
          v_.split_at_mut_unchecked(::sus::marker::unsafe_fn, chunk_size_);
      v_ = snd;
      return Option<Item>(fst);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      // SAFETY: `split_at_mut_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by subtracting an unsigned (and
      // thus non-negative) value from the length.
      //
      // SAFETY: chunks_size <= len() as checked above, so len() - chunk_size_
      // is a non-negative value <= len() <= usize::MAX.
      auto [fst, snd] = v_.split_at_mut_unchecked(
          ::sus::marker::unsafe_fn,
          v_.len().unchecked_sub(::sus::marker::unsafe_fn, chunk_size_));
      v_ = fst;
      return ::sus::Option<Item>(snd);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  constexpr ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  constexpr ::sus::num::usize exact_size_hint() const noexcept {
    return v_.len() / chunk_size_;
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by SliceMut, Vec, Array.
  friend class SliceMut<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  static constexpr auto with_slice(::sus::iter::IterRef ref,
                                   const SliceMut<ItemT>& values,
                                   ::sus::num::usize chunk_size) noexcept {
    ::sus::check(chunk_size > 0u);
    auto rem = values.len() % chunk_size;
    // SAFETY: rem < len() by construction above, so len() - rem is a
    // non-negative value <= len() <= usize::MAX.
    auto fst_len = values.len().unchecked_sub(::sus::marker::unsafe_fn, rem);
    // SAFETY: 0 <= fst_len <= values.len() by construction above.
    auto [fst, snd] =
        values.split_at_mut_unchecked(::sus::marker::unsafe_fn, fst_len);
    return ChunksExactMut(CONSTRUCT, ::sus::move(ref), fst, snd, chunk_size);
  }

  enum Construct { CONSTRUCT };
  constexpr ChunksExactMut(Construct, ::sus::iter::IterRef ref,
                           const SliceMut<ItemT>& values,
                           const SliceMut<ItemT>& remainder,
                           ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)),
        v_(values),
        rem_(remainder),
        chunk_size_(chunk_size) {}

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  SliceMut<ItemT> v_;
  SliceMut<ItemT> rem_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(rem_),
                                  decltype(chunk_size_));
};

/// An iterator over a slice in (non-overlapping) chunks (`chunk_size` elements
/// at a time), starting at the end of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last slice
/// of the iteration will be the remainder.
///
/// This struct is created by the `rchunks()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] RChunks final
    : public ::sus::iter::IteratorBase<RChunks<ItemT>,
                                       ::sus::collections::Slice<ItemT>> {
 public:
  // `Item` is a `Slice<T>`.
  using Item = ::sus::collections::Slice<ItemT>;

 private:
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return Option<Item>();
    } else {
      const auto len = v_.len();
      const auto chunksz = ::sus::cmp::min(len, chunk_size_);
      // SAFETY: chunkz <= len due to min(), so len - chunksz is a non-negative
      // value <= len <= usize::MAX.
      const auto mid = len.unchecked_sub(::sus::marker::unsafe_fn, chunksz);
      // SAFETY: 0 <= mid <= len since we're subtracting from len a value that
      // is <= len.
      auto [fst, snd] = v_.split_at_unchecked(::sus::marker::unsafe_fn, mid);
      v_ = fst;
      return Option<Item>(snd);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      const auto remainder = v_.len() % chunk_size_;
      const auto chunksz = remainder != 0u ? remainder : chunk_size_;
      // SAFETY: `split_at_unchecked()` requires the argument be less than or
      // equal to the length. This is guaranteed, but subtle: `chunksz`
      // will always either be `v_.len() % chunk_size_`, which will always
      // evaluate to strictly less than `v_.len()` (or panic, in the case that
      // `chunk_size_` is zero), or it can be `chunk_size_`, in the case that
      // the length is exactly divisible by the chunk size.
      //
      // While it seems like using `chunk_size_` in this case could lead to a
      // value greater than `v_.len()`, it cannot: if `chunk_size_` were greater
      // than `v_.len()`, then `v_.len() % chunk_size_` would return nonzero
      // (note that in this branch of the `if`, we already know that `v_` is
      // non-empty).
      auto [fst, snd] =
          v_.split_at_unchecked(::sus::marker::unsafe_fn, chunksz);
      v_ = snd;
      return ::sus::Option<Item>(fst);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  constexpr ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  constexpr ::sus::num::usize exact_size_hint() const noexcept {
    if (v_.is_empty()) {
      return 0u;
    } else {
      ::sus::num::usize n = v_.len() / chunk_size_;
      ::sus::num::usize rem = v_.len() % chunk_size_;
      return rem > 0u ? n + 1u : n;
    }
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by Slice, Vec, Array.
  friend class Slice<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  constexpr RChunks(::sus::iter::IterRef ref, const Slice<ItemT>& values,
                    ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)), v_(values), chunk_size_(chunk_size) {
    ::sus::check(chunk_size > 0u);
  }

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  Slice<ItemT> v_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(chunk_size_));
};

/// An iterator over a mutable slice in (non-overlapping) chunks (`chunk_size`
/// elements at a time), starting at the end of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last slice
/// of the iteration will be the remainder.
///
/// This struct is created by the `rchunks_mut()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] RChunksMut final
    : public ::sus::iter::IteratorBase<RChunksMut<ItemT>,
                                       ::sus::collections::SliceMut<ItemT>> {
 public:
  // `Item` is a `SliceMut<T>`.
  using Item = ::sus::collections::SliceMut<ItemT>;

 private:
  static_assert(!std::is_reference_v<ItemT>);
  static_assert(!std::is_const_v<ItemT>);
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return Option<Item>();
    } else {
      const auto len = v_.len();
      auto chunksz = ::sus::cmp::min(len, chunk_size_);
      // SAFETY: chunkz <= len due to min(), so len - chunksz is a non-negative
      // value <= len <= usize::MAX.
      const auto mid = len.unchecked_sub(::sus::marker::unsafe_fn, chunksz);
      // SAFETY: 0 <= mid <= len since we're subtracting from len a value that
      // is <= len.
      auto [fst, snd] =
          v_.split_at_mut_unchecked(::sus::marker::unsafe_fn, mid);
      v_ = fst;
      return Option<SliceMut<ItemT>>(snd);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept {
    if (v_.is_empty()) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      const auto remainder = v_.len() % chunk_size_;
      const auto chunksz = remainder != 0u ? remainder : chunk_size_;
      // SAFETY: Similar to `RChunks::next_back`
      auto [fst, snd] =
          v_.split_at_mut_unchecked(::sus::marker::unsafe_fn, chunksz);
      v_ = snd;
      return ::sus::Option<Item>(fst);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  constexpr ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  constexpr ::sus::num::usize exact_size_hint() const noexcept {
    if (v_.is_empty()) {
      return 0u;
    } else {
      const auto n = v_.len() / chunk_size_;
      const auto rem = v_.len() % chunk_size_;
      return rem > 0u ? n + 1u : n;
    }
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by SliceMut, Vec, Array.
  friend class SliceMut<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  constexpr RChunksMut(::sus::iter::IterRef ref, const SliceMut<ItemT>& values,
                       ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)), v_(values), chunk_size_(chunk_size) {
    ::sus::check(chunk_size > 0u);
  }

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  SliceMut<ItemT> v_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(chunk_size_));
};

/// An iterator over a slice in (non-overlapping) chunks (`chunk_size` elements
/// at a time), starting at the end of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last up to
/// `chunk_size-1` elements will be omitted but can be retrieved from the
/// remainder function from the iterator.
///
/// This struct is created by the `rchunks_exact()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] RChunksExact final
    : public ::sus::iter::IteratorBase<RChunksExact<ItemT>,
                                       ::sus::collections::Slice<ItemT>> {
 public:
  // `Item` is a `Slice<T>`.
  using Item = ::sus::collections::Slice<ItemT>;

 private:
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  /// Returns the remainder of the original slice that is not going to be
  /// returned by the iterator. The returned slice has at most `chunk_size-1`
  /// elements.
  [[nodiscard]] constexpr Item remainder() const& { return rem_; }

  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return Option<Item>();
    } else {
      // SAFETY: chunk_size_ <= len() as checked in the if condition above, so
      // subtracting len() - chunk_size_ produces a non-negative value <= len()
      // <= usize::MAX.
      const auto mid =
          v_.len().unchecked_sub(::sus::marker::unsafe_fn, chunk_size_);
      // SAFETY: `split_at_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by subtracting a non-negative
      // value from the len.
      auto [fst, snd] = v_.split_at_unchecked(::sus::marker::unsafe_fn, mid);
      v_ = fst;
      return Option<Item>(snd);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      // SAFETY: `split_at_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by checking the condition
      // above, and that we are in the else branch.
      auto [fst, snd] =
          v_.split_at_unchecked(::sus::marker::unsafe_fn, chunk_size_);
      v_ = snd;
      return ::sus::Option<Item>(fst);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  constexpr ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  constexpr ::sus::num::usize exact_size_hint() const noexcept {
    return v_.len() / chunk_size_;
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by Slice, Vec, Array.
  friend class Slice<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  static constexpr auto with_slice(::sus::iter::IterRef ref,
                                   const Slice<ItemT>& values,
                                   ::sus::num::usize chunk_size) noexcept {
    ::sus::check(chunk_size > 0u);
    auto rem = values.len() % chunk_size;
    // SAFETY: 0 <= rem <= values.len() by construction above.
    auto [fst, snd] = values.split_at_unchecked(::sus::marker::unsafe_fn, rem);
    return RChunksExact(CONSTRUCT, ::sus::move(ref), snd, fst, chunk_size);
  }

  enum Construct { CONSTRUCT };
  constexpr RChunksExact(Construct, ::sus::iter::IterRef ref,
                         const Slice<ItemT>& values,
                         const Slice<ItemT>& remainder,
                         ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)),
        v_(values),
        rem_(remainder),
        chunk_size_(chunk_size) {}

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  Slice<ItemT> v_;
  Slice<ItemT> rem_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(rem_),
                                  decltype(chunk_size_));
};

/// An iterator over a mutable slice in (non-overlapping) chunks (`chunk_size`
/// elements at a time), starting at the end of the slice.
///
/// When the slice len is not evenly divided by the chunk size, the last up to
/// `chunk_size-1` elements will be omitted but can be retrieved from the
/// remainder function from the iterator.
///
/// This struct is created by the `rchunks_exact_mut()` method on slices.
template <class ItemT>
struct [[nodiscard]] [[sus_trivial_abi]] RChunksExactMut final
    : public ::sus::iter::IteratorBase<RChunksExactMut<ItemT>,
                                       ::sus::collections::SliceMut<ItemT>> {
 public:
  // `Item` is a `SliceMut<T>`.
  using Item = ::sus::collections::SliceMut<ItemT>;

 private:
  // `SliceItem` is a `T`.
  using SliceItem = ItemT;

 public:
  /// Returns the remainder of the original slice that is not going to be
  /// returned by the iterator. The returned slice has at most `chunk_size-1`
  /// elements.
  [[nodiscard]] constexpr Item remainder() const& { return rem_; }

  // sus::iter::Iterator trait.
  constexpr Option<Item> next() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return Option<Item>();
    } else {
      // SAFETY: `split_at_mut_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by subtracting a non-negative
      // value from the len.
      auto [fst, snd] = v_.split_at_mut_unchecked(::sus::marker::unsafe_fn,
                                                  v_.len() - chunk_size_);
      v_ = fst;
      return Option<Item>(snd);
    }
  }

  // sus::iter::DoubleEndedIterator trait.
  constexpr Option<Item> next_back() noexcept {
    if (v_.len() < chunk_size_) [[unlikely]] {
      return ::sus::Option<Item>();
    } else {
      // SAFETY: `split_at_unchecked` requires the argument be less than or
      // equal to the length. This is guaranteed by checking the condition
      // above, and that we are in the else branch.
      auto [fst, snd] =
          v_.split_at_mut_unchecked(::sus::marker::unsafe_fn, chunk_size_);
      v_ = snd;
      return ::sus::Option<Item>(fst);
    }
  }

  // Replace the default impl in sus::iter::IteratorBase.
  constexpr ::sus::iter::SizeHint size_hint() const noexcept {
    const auto remaining = exact_size_hint();
    return {remaining, ::sus::Option<::sus::num::usize>(remaining)};
  }

  /// sus::iter::ExactSizeIterator trait.
  constexpr ::sus::num::usize exact_size_hint() const noexcept {
    return v_.len() / chunk_size_;
  }

  // TODO: Impl count(), nth(), last(), nth_back().

 private:
  // Constructed by SliceMut, Vec, Array.
  friend class SliceMut<ItemT>;
  friend class Vec<ItemT>;
  template <class ArrayItemT, size_t N>
  friend class Array;

  static constexpr auto with_slice(::sus::iter::IterRef ref,
                             const SliceMut<ItemT>& values,
                             ::sus::num::usize chunk_size) noexcept {
    ::sus::check(chunk_size > 0u);
    auto rem = values.len() % chunk_size;
    // SAFETY: 0 <= rem <= values.len() by construction above.
    auto [fst, snd] =
        values.split_at_mut_unchecked(::sus::marker::unsafe_fn, rem);
    return RChunksExactMut(::sus::move(ref), snd, fst, chunk_size);
  }

  constexpr RChunksExactMut(::sus::iter::IterRef ref,
                            const SliceMut<ItemT>& values,
                            const SliceMut<ItemT>& remainder,
                            ::sus::num::usize chunk_size) noexcept
      : ref_(::sus::move(ref)),
        v_(values),
        rem_(remainder),
        chunk_size_(chunk_size) {}

  [[sus_no_unique_address]] ::sus::iter::IterRef ref_;
  SliceMut<ItemT> v_;
  SliceMut<ItemT> rem_;
  ::sus::num::usize chunk_size_;

  sus_class_trivially_relocatable(::sus::marker::unsafe_fn, decltype(ref_),
                                  decltype(v_), decltype(rem_),
                                  decltype(chunk_size_));
};

}  // namespace sus::collections
