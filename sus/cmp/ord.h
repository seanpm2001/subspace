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

// TODO: Move all this to sus::cmp?

#pragma once

#include <compare>
#include <concepts>

#include "sus/assertions/check.h"
#include "sus/fn/fn_concepts.h"
#include "sus/macros/lifetimebound.h"
#include "sus/mem/forward.h"

namespace sus::cmp {

/// Concept that combines all ordering types together.
template <class T>
concept Ordering = (std::same_as<T, std::strong_ordering> ||
                    std::same_as<T, std::partial_ordering> ||
                    std::same_as<T, std::weak_ordering>);

/// Concept for types that form a unique total order (aka
/// [`std::strong_ordering`](https://en.cppreference.com/w/cpp/utility/compare/strong_ordering)).
/// Objects that sort the same for ordering must also compare as equal.
///
/// # StrongOrd and Eq interations
/// While [`StrongOrd`]($sus::cmp::StrongOrd) can report equality, it does not
/// imply that the type satisfies [`Eq`]($sus::cmp::Eq), and a separate
/// `operator==` is required for that concept. For correctness, types that
/// satisfy
/// [`StrongOrd`]($sus::cmp::StrongOrd) and [`Eq`]($sus::cmp::Eq) must have
/// object which compare as equivalent for ording also compare as equal with
/// `operator==`.
///
/// Generic code that requires a type to be
/// [`StrongOrd`]($sus::cmp::StrongOrd) should take care to use `operator<=>`
/// and not `operator==` unless also requiring [`Eq`]($sus::cmp::Eq).
template <class Lhs, class Rhs = Lhs>
concept StrongOrd = requires(const std::remove_reference_t<Lhs>& lhs,
                             const std::remove_reference_t<Rhs>& rhs) {
  { lhs <=> rhs } -> std::same_as<std::strong_ordering>;
};

/// Concept for types that form a total ordering (aka
/// [`std::weak_ordering`](https://en.cppreference.com/w/cpp/utility/compare/weak_ordering)).
///
/// Types that satisfy [`Ord`]($sus::cmp::Ord) can be sorted or compared and
/// always return a consistent result as all possible values are comparable.
/// Objects that compare as equivalent for ordering may still be different
/// internally and compare as different through `operator==`. If unique identity
/// is required, use [`StrongOrd`]($sus::cmp::StrongOrd). Otherwise, typically
/// use [`Ord`]($sus::cmp::Ord) for constraining types that will be ordered.
///
/// # How can I implement Ord?
/// [`Ord`]($sus::cmp::Ord) requires that the type has `operator<=>` which
/// returns
/// [`std::weak_ordering`](https://en.cppreference.com/w/cpp/utility/compare/weak_ordering)
/// (or
/// [`std::strong_ordering`](https://en.cppreference.com/w/cpp/utility/compare/strong_ordering)).
/// It will implicitly also be [`Ord`]($sus::cmp::Ord) and
/// [`PartialOrd`]($sus::cmp::PartialOrd) as a result.
///
/// # Lexicographical comparison
/// Lexicographical comparison is an operation with the following properties:
/// * Two sequences are compared element by element.
/// * The first mismatching element defines which sequence is lexicographically
///   less or greater than the other.
/// * If one sequence is a prefix of another, the shorter sequence is
///   lexicographically less than the other.
/// * If two sequence have equivalent elements and are of the same length, then
///   the sequences are lexicographically equal.
/// * An empty sequence is lexicographically less than any non-empty sequence.
/// * Two empty sequences are lexicographically equal.
///
/// # Ord and Eq interations
/// While [`Ord`]($sus::cmp::Ord) can report equality, it does not imply that
/// the type satisfies [`Eq`]($sus::cmp::Eq), and a separate `operator==` is
/// required for that concept. Unlike
/// [`StrongOrd`]($sus::cmp::StrongOrd), it is not required that objects which
/// are ordered as equivalent also compare as equal with `operator==`.
///
/// Generic code that requires a type to be [`Ord`]($sus::cmp::Ord) should
/// take care to use `operator<=>` and not `operator==` unless also requiring
/// [`Eq`]($sus::cmp::Eq), in which case consider requiring
/// [`StrongOrd`]($sus::cmp::StrongOrd) in place of both
/// [`Ord`]($sus::cmp::Ord) and [`Eq`]($sus::cmp::Eq).
///
/// # Determining Ord strictly
/// [`Ord`]($sus::cmp::Ord) will be also satisfied if the types satisfy
/// [`StrongOrd`]($sus::cmp::StrongOrd). To determine if a
/// [`Ord`]($sus::cmp::Ord) is the strongest type of ordering between the
/// types, use [`ExclusiveOrd`]($sus::cmp::ExclusiveOrd).
template <class Lhs, class Rhs = Lhs>
concept Ord =
    StrongOrd<Lhs, Rhs> || requires(const std::remove_reference_t<Lhs>& lhs,
                                    const std::remove_reference_t<Rhs>& rhs) {
      { lhs <=> rhs } -> std::same_as<std::weak_ordering>;
    };

/// Concept for types that form a partial ordering (aka
/// [`std::partial_ordering`](https://en.cppreference.com/w/cpp/utility/compare/partial_ordering)).
///
/// # Determining PartialOrd strictly
/// [`PartialOrd`]($sus::cmp::PartialOrd) will be satisfied if the types
/// satisfy [`StrongOrd`]($sus::cmp::StrongOrd) or [`Ord`]($sus::cmp::Ord).
/// To determine if a partial ordering is the strongest type of ordering between
/// the types, use [`ExclusivePartialOrd`]($sus::cmp::ExclusivePartialOrd).
template <class Lhs, class Rhs = Lhs>
concept PartialOrd =
    Ord<Lhs, Rhs> || StrongOrd<Lhs, Rhs> ||
    requires(const std::remove_reference_t<Lhs>& lhs, const Rhs& rhs) {
      { lhs <=> rhs } -> std::same_as<std::partial_ordering>;
    };

/// Concept for types that have a unique total ordering (aka
/// [`std::strong_ordering`](https://en.cppreference.com/w/cpp/utility/compare/strong_ordering)).
///
/// This is an alias for StrongOrd, but exists as a set with
/// [`ExclusiveOrd`]($sus::cmp::ExclusiveOrd) and
/// [`ExclusivePartialOrd`]($sus::cmp::ExclusivePartialOrd).
template <class Lhs, class Rhs = Lhs>
concept ExclusiveStrongOrd = StrongOrd<Lhs, Rhs>;

/// Determines if the types `Lhs` and `Rhs` have total ordering (aka
/// [`std::weak_ordering`](https://en.cppreference.com/w/cpp/utility/compare/weak_ordering)),
/// and that this is the strongest ordering that exists between the types.
template <class Lhs, class Rhs = Lhs>
concept ExclusiveOrd = (!StrongOrd<Lhs, Rhs> && Ord<Lhs, Rhs>);

/// Determines if the types `Lhs` and `Rhs` have a partial ordering (aka
/// [`std::partial_ordering`](https://en.cppreference.com/w/cpp/utility/compare/partial_ordering)),
/// and that this is the strongest ordering that exists between the types.
template <class Lhs, class Rhs = Lhs>
concept ExclusivePartialOrd = (!Ord<Lhs, Rhs> && PartialOrd<Lhs, Rhs>);

/// Compares and returns the minimum of two values.
///
/// Returns the first argument if the comparison determines them to be equal.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::min<i32&>(a, b)`.
///
/// Note that if either input is a temporary object this can return a reference
/// to an object past its lifetime.
template <class T>
  requires(::sus::cmp::Ord<T>)
inline constexpr T min(T a sus_lifetimebound, T b sus_lifetimebound) noexcept {
  return a > b ? ::sus::forward<T>(b) : ::sus::forward<T>(a);
}

/// Compares and returns the minimum of two values with respect to the specified
/// comparison function.
///
/// Returns the first argument if the comparison determines them to be equal.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::min_by<i32&>(a, b, c)`.
///
/// Note that if either input is a temporary object this can return a reference
/// to an object past its lifetime.
template <class T>
constexpr T min_by(
    T a sus_lifetimebound, T b sus_lifetimebound,
    ::sus::fn::FnOnce<std::weak_ordering(
        const std::remove_reference_t<T>&,
        const std::remove_reference_t<T>&)> auto compare) noexcept {
  return ::sus::fn::call_once(::sus::move(compare), a, b) ==
                 std::weak_ordering::greater
             ? ::sus::forward<T>(b)
             : ::sus::forward<T>(a);
}

/// Returns the element that gives the minimum value from the specified
/// function.
///
/// Returns the first argument if the comparison determines them to be equal.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::min_by_key<i32&>(a, b, k)`.
///
/// Note that if either input is a temporary object this can return a reference
/// to an object past its lifetime.
template <
    class T,
    ::sus::fn::FnMut<::sus::fn::NonVoid(const std::remove_reference_t<T>&)>
        KeyFn,
    int&...,
    class Key = std::invoke_result_t<KeyFn&, const std::remove_reference_t<T>&>>
  requires(::sus::cmp::Ord<Key>)
constexpr T min_by_key(T a sus_lifetimebound, T b sus_lifetimebound,
                       KeyFn f) noexcept {
  return ::sus::fn::call_mut(f, a) > ::sus::fn::call_mut(f, b)
             ? ::sus::forward<T>(b)
             : ::sus::forward<T>(a);
}

/// Compares and returns the maximum of two values.
///
/// Returns the second argument if the comparison determines them to be equal.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::max<i32&>(a, b)`. Note that if either input is a temporary object
/// this can return a reference to an object past its lifetime.
template <class T>
  requires(::sus::cmp::Ord<T>)
constexpr T max(T a sus_lifetimebound, T b sus_lifetimebound) noexcept {
  return a > b ? ::sus::forward<T>(a) : ::sus::forward<T>(b);
}

/// Compares and returns the maximum of two values with respect to the specified
/// comparison function.
///
/// Returns the second argument if the comparison determines them to be equal.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::max_by<i32&>(a, b, c)`. Note that if either input is a
/// temporary object this can return a reference to an object past its lifetime.
template <class T>
constexpr T max_by(
    T a sus_lifetimebound, T b sus_lifetimebound,
    ::sus::fn::FnOnce<std::weak_ordering(
        const std::remove_reference_t<T>&,
        const std::remove_reference_t<T>&)> auto compare) noexcept {
  return ::sus::fn::call_once(::sus::move(compare), a, b) ==
                 std::weak_ordering::greater
             ? ::sus::forward<T>(a)
             : ::sus::forward<T>(b);
}

/// Returns the element that gives the maximum value from the specified
/// function.
///
/// Returns the second argument if the comparison determines them to be equal.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::max_by_key<i32&>(a, b, k)`. Note that if either input is a
/// temporary object this can return a reference to an object past its lifetime.
template <
    class T,
    ::sus::fn::FnMut<::sus::fn::NonVoid(const std::remove_reference_t<T>&)>
        KeyFn,
    int&...,
    class Key = std::invoke_result_t<KeyFn&, const std::remove_reference_t<T>&>>
  requires(::sus::cmp::Ord<Key>)
constexpr T max_by_key(T a sus_lifetimebound, T b sus_lifetimebound,
                       KeyFn f) noexcept {
  return f(a) > f(b) ? ::sus::forward<T>(a) : ::sus::forward<T>(b);
}

/// Restrict a value to a certain interval.
///
/// Returns `max` if `v` is greater than `max`, and `min` if `v` is less than
/// `min`. Otherwise this returns `v`.
///
/// By default this receives and returns objects by value. To receive and return
/// references, specify the type parameter, such as:
/// `sus::cmp::clamp<i32&>(a, min, max)`. Note that if any input is a temporary
/// object this can return a reference to an object past its lifetime.
///
/// # Panics
/// Panics if `min > max`.
template <class T>
  requires(::sus::cmp::Ord<T>)
constexpr T clamp(T v sus_lifetimebound, T min sus_lifetimebound,
                  T max sus_lifetimebound) noexcept {
  ::sus::check(min <= max);
  return v < min ? ::sus::forward<T>(min)
                 : (v > max ? ::sus::forward<T>(max) : ::sus::forward<T>(v));
}

}  // namespace sus::cmp
