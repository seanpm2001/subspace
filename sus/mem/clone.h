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

#pragma once

#include <concepts>

#include "sus/macros/inline.h"
#include "sus/mem/copy.h"
#include "sus/mem/move.h"

namespace sus::mem {

namespace __private {

// clang-format off
template <class T>
concept HasCloneMethod = requires(const T& source) {
    { source.clone() } -> std::same_as<T>;
};

template <class T>
concept HasCloneFromMethod = requires(T& self, const T& source) {
    { self.clone_from(source) } -> std::same_as<void>;
};
// clang-format on

}  // namespace __private

/// A `Clone` type can make a new copy of itself.
///
/// When a type is small enough to be passed in registers
/// (typically at most the size of two pointers) and copying is the same as
/// moving, prefer to make the type [`Copy`]($sus::mem::Copy), which also
/// implies `Clone`.
///
/// A type `T` is `Clone` if one of the following holds:
/// * Is [`Copy`]($sus::mem::Copy). Meaning it has a copy constructor and
///   assignment.
/// * Has a `clone() const -> T` method, and is [`Move`]($sus::mem::Move).
///
/// This concept tests the object type of `T`, looking through reference types
/// `T&` or `const T&`.
/// To test the reference types themselves (which can always be copied as a
/// pointer) in generic code that can handle reference types and won't copy
/// the objects behind them, use [`CloneOrRef`]($sus::mem::CloneOrRef).
///
/// A `Clone` type may also provide a `clone_from(const T& source)` method
/// to have `sus::mem::clone_into()` perform copy-assignment from `source`, in
/// order to reuse its resources and avoid allocations. In this case the type is
/// also [`CloneFrom`]($sus::mem::CloneFrom).
///
/// It is not valid to be [`Copy`]($sus::mem::Copy) and also have a `clone()`
/// method, as it becomes ambiguous.
/// One outcome of this is a collection type should only implement a
/// `clone()` method if the type `T` within is `(Clone<T> && !Copy<T>)`, and
/// should only implement copy constructors if the type `T` within is `Copy<T>`.
template <class T>
concept Clone =
    (Copy<T> && !__private::HasCloneMethod<
                    std::remove_const_t<std::remove_reference_t<T>>>) ||
    (!Copy<T> && Move<T> &&
     __private::HasCloneMethod<
         std::remove_const_t<std::remove_reference_t<T>>>);

/// A `CloneOrRef` object or reference of type `T` can be cloned to construct a
/// new `T`.
///
/// This concept is used for templates that want to be generic over references,
/// that is templates that want to allow their template parameter to be a
/// reference and work with that reference as if it were an object itself. This
/// is uncommon outside of library implementations, and its usage should
/// typically be encapsulated inside a type that is `Clone`.
template <class T>
concept CloneOrRef = Clone<T> || std::is_reference_v<T>;

/// Determines if `T` has an optimized path `T::clone_from(const T&)` that can
/// be used from [`clone_into`]($sus::mem::clone_into).
///
/// This is largely an implementation detail of
/// [`clone_into`]($sus::mem::clone_into) as it can be called on all types that
/// satisfy [`Clone`]($sus::mem::Clone). But it can be useful for verifying in
/// a static assert that a type is written correctly and the optimization will
/// be used.
///
/// Evaluates to true if the type is [`Copy`]($sus::mem::Copy), as the copy
/// assignment operator acts like `clone_from` method. It return false if the
/// type is `Copy` and also has a `clone_from` method, as it becomes ambiguous
/// and is thus invalid.
template <class T>
concept CloneFrom =
    Clone<T> &&
    ((Copy<T> && !__private::HasCloneFromMethod<
                     std::remove_const_t<std::remove_reference_t<T>>>) ||
     (!Copy<T> && __private::HasCloneFromMethod<
                      std::remove_const_t<std::remove_reference_t<T>>>));

/// Clones the input either by copying or cloning. Returns a new object of type
/// `T`.
///
/// If `T` is a reference type, it will clone the underlying object.
template <Clone T>
[[nodiscard]] sus_always_inline constexpr std::decay_t<T> clone(
    const T& source) noexcept {
  if constexpr (Copy<T>) {
    return source;
  } else {
    return source.clone();
  }
}

/// Clones the input either by copying or cloning, producing an object of type
/// `T`. As with [`forward`]($sus::mem::forward), the template argument `T`
/// must always be specified when calling this function to deduce the correct
/// return type.
///
/// If `T` is a reference type, it copies and returns the reference instead of
/// the underlying object.
template <CloneOrRef T>
[[nodiscard]] sus_always_inline constexpr T clone_or_forward(
    const std::remove_reference_t<T>& source) noexcept {
  if constexpr (std::is_reference_v<T>) {
    return source;
  } else {
    return ::sus::mem::clone(source);
  }
}

/// Performs copy-assignment from `source`.
///
/// Will perform a copy assignment if T is `Copy`. Otherwise will perform the
/// equivalent of `dest = source.clone()`.
///
/// The `Clone` type may provide an implementation of `clone_from(const T&
/// source)` to reuse its resources and avoid unnecessary allocations, which
/// will be preferred over calling `source.clone()`.
//
// TODO: Provide a concept to verify that this method exists.
template <Clone T>
sus_always_inline constexpr void clone_into(T& dest, const T& source) noexcept {
  if constexpr (Copy<T>) {
    dest = source;
  } else if constexpr (CloneFrom<T>) {
    dest.clone_from(source);
  } else {
    dest = source.clone();
  }
}

}  // namespace sus::mem

// Promote `clone()` and `clone_into()` into the `sus` namespace.
namespace sus {
using ::sus::mem::clone;
using ::sus::mem::clone_into;
}  // namespace sus
