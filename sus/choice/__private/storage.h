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

#include <compare>
#include <memory>
#include <type_traits>

#include "sus/choice/__private/nothing.h"
#include "sus/choice/__private/pack_index.h"
#include "sus/macros/no_unique_address.h"
#include "sus/mem/move.h"
#include "sus/tuple/tuple.h"

namespace sus::choice_type::__private {

template <class... Ts>
struct MakeStorageType {
  static_assert(sizeof...(Ts) > 0u,
                "Every Choice tag must come with at least one value type. "
                "The type can be `void` to specify no value. "
                "For example: "
                "`using C = Choice<sus_choice_types((0, u32), (1, void))>;`");
  using type = ::sus::Tuple<Ts...>;
};

template <>
struct MakeStorageType<void> {
  using type = Nothing;
};

enum MissingStorageType {};

template <class... Ts>
struct StorageTypeOfTagHelper {
  using type = MissingStorageType;
};

template <>
struct StorageTypeOfTagHelper<Nothing> {
  using type = Nothing;
};

template <class T>
struct StorageTypeOfTagHelper<::sus::Tuple<T>> {
  using type = T;
};

template <class... Ts>
  requires(sizeof...(Ts) > 1)
struct StorageTypeOfTagHelper<::sus::Tuple<Ts...>> {
  using type = ::sus::Tuple<Ts...>;
};

/// The Storage class stores Tuples internally, but for Tuples of a single
/// object, they are stored and accessed as the interior object.
///
/// For example:
/// * Storage of `Tuple<i32, u32>` will be `Tuple<i32, u32>`.
/// * Storage of `Tuple<i32>` will be just `i32`.
template <size_t I, class... Ts>
using StorageTypeOfTag = StorageTypeOfTagHelper<PackIth<I, Ts...>>::type;

template <class T>
struct StorageCountHelper {
  static constexpr size_t value = StorageIsVoid<T> ? 0u : 1u;
};

template <class... Ts>
struct StorageCountHelper<::sus::tuple_type::Tuple<Ts...>> {
  static constexpr size_t value = sizeof...(Ts);
};

// TODO: We could receive size_t as an input here and figure out the count
// without going through the StorageType and StorageIsVoid.
template <class StorageType>
static constexpr size_t StorageCount = StorageCountHelper<StorageType>::value;

struct SafelyConstructibleFromReferenceSuccess {};

template <size_t I>
struct SafelyConstructibleFromReferenceFailedForArgument {};

template <size_t I, class StorageType, class... Params>
struct SafelyConstructibleHelper;

template <size_t I>
struct SafelyConstructibleHelper<I, Nothing> {
  static_assert(I == 0u);
  using type = SafelyConstructibleFromReferenceSuccess;
};

template <size_t I, class... Ts, class P, class... Params>
  requires(sizeof...(Params) > 0u)
struct SafelyConstructibleHelper<I, ::sus::tuple_type::Tuple<Ts...>, P,
                                 Params...> {
  static_assert(I + 1u + sizeof...(Params) == sizeof...(Ts));

  static constexpr bool safe =
      ::sus::construct::SafelyConstructibleFromReference<PackIth<I, Ts...>, P>;

  using type = std::conditional_t<
      safe,
      typename SafelyConstructibleHelper<
          I + 1u, ::sus::tuple_type::Tuple<Ts...>, Params...>::type,
      SafelyConstructibleFromReferenceFailedForArgument<I>>;
};

template <size_t I, class... Ts, class P>
struct SafelyConstructibleHelper<I, ::sus::tuple_type::Tuple<Ts...>, P> {
  static_assert(I + 1u == sizeof...(Ts));

  static constexpr bool safe =
      ::sus::construct::SafelyConstructibleFromReference<PackIth<I, Ts...>, P>;

  using type = std::conditional_t<  //
      safe,                         //
      SafelyConstructibleFromReferenceSuccess,
      SafelyConstructibleFromReferenceFailedForArgument<I>>;
};

/// Checks [`SafelyConstructibleFromReference`](
/// $sus::construct::SafelyConstructibleFromReference) against every parameter
/// type in `From...` and how it will be stored inside the [`Choice`](
/// $sus::choice_type::Choice), typically as one of the members of a [`Tuple`](
/// $sus::tuple_type::Tuple).
template <class StorageType, class... From>
concept StorageIsSafelyConstructibleFromReference = std::same_as<
    typename SafelyConstructibleHelper<0u, StorageType, From...>::type,
    SafelyConstructibleFromReferenceSuccess>;

template <size_t I, class... Elements>
union Storage;

template <size_t I, class... Ts, class... Elements>
  requires(sizeof...(Ts) > 1 && sizeof...(Elements) > 0)
union Storage<I, ::sus::Tuple<Ts...>, Elements...> {
  constexpr Storage() {}
  ~Storage()
    requires(std::is_trivially_destructible_v<::sus::Tuple<Ts...>> && ... &&
             std::is_trivially_destructible_v<Elements>)
  = default;
  constexpr ~Storage()
    requires(!(std::is_trivially_destructible_v<::sus::Tuple<Ts...>> && ... &&
               std::is_trivially_destructible_v<Elements>))
  {}

  Storage(const Storage&)
    requires(std::is_trivially_copy_constructible_v<::sus::Tuple<Ts...>> &&
             ... && std::is_trivially_copy_constructible_v<Elements>)
  = default;
  Storage& operator=(const Storage&)
    requires(std::is_trivially_copy_assignable_v<::sus::Tuple<Ts...>> && ... &&
             std::is_trivially_copy_assignable_v<Elements>)
  = default;
  Storage(Storage&&)
    requires(std::is_trivially_move_constructible_v<::sus::Tuple<Ts...>> &&
             ... && std::is_trivially_move_constructible_v<Elements>)
  = default;
  Storage& operator=(Storage&&)
    requires(std::is_trivially_move_assignable_v<::sus::Tuple<Ts...>> && ... &&
             std::is_trivially_move_assignable_v<Elements>)
  = default;

  using Type = ::sus::Tuple<Ts...>;

  inline constexpr void activate_for_construct(size_t index) {
    if (index != I) {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.activate_for_construct(index);
    }
  }
  inline constexpr void construct(Type&& tuple) {
    std::construct_at(&tuple_, ::sus::move(tuple));
  }
  inline constexpr void assign(Type&& tuple) { tuple_ = ::sus::move(tuple); }
  inline void move_construct(size_t index, Storage&& from) {
    if (index == I) {
      std::construct_at(&tuple_, ::sus::move(from.tuple_));
    } else {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.move_construct(index, ::sus::move(from.more_));
    }
  }
  inline constexpr void move_assign(size_t index, Storage&& from) {
    if (index == I) {
      tuple_ = ::sus::move(from.tuple_);
    } else {
      more_.move_assign(index, ::sus::move(from.more_));
    }
  }
  inline constexpr void copy_construct(size_t index, const Storage& from) {
    if (index == I) {
      std::construct_at(&tuple_, from.tuple_);
    } else {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.copy_construct(index, from.more_);
    }
  }
  inline constexpr void copy_assign(size_t index, const Storage& from) {
    if (index == I) {
      tuple_ = from.tuple_;
    } else {
      more_.copy_assign(index, from.more_);
    }
  }
  inline constexpr void clone_construct(size_t index, const Storage& from) {
    if (index == I) {
      std::construct_at(&tuple_, ::sus::clone(from.tuple_));
    } else {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.clone_construct(index, from.more_);
    }
  }
  inline constexpr void destroy(size_t index) {
    if (index == I) {
      tuple_.~Type();
    } else {
      more_.destroy(index);
      more_.~Storage<I + 1, Elements...>();
    }
  }
  inline constexpr bool eq(size_t index, const Storage& other) const& {
    if (index == I) {
      return tuple_ == other.tuple_;
    } else {
      return more_.eq(index, other.more_);
    }
  }
  inline constexpr std::strong_ordering strong_ord(
      size_t index, const Storage& other) const& {
    if (index == I) {
      return std::strong_order(tuple_, other.tuple_);
    } else {
      return more_.ord(index, other.more_);
    }
  }
  inline constexpr std::weak_ordering weak_ord(size_t index,
                                               const Storage& other) const& {
    if (index == I) {
      return std::weak_order(tuple_, other.tuple_);
    } else {
      return more_.weak_ord(index, other.more_);
    }
  }
  inline constexpr std::partial_ordering partial_ord(
      size_t index, const Storage& other) const& {
    if (index == I) {
      return std::partial_order(tuple_, other.tuple_);
    } else {
      return more_.partial_ord(index, other.more_);
    }
  }

  constexpr auto as() const& {
    return [this]<size_t... Is>(std::index_sequence<Is...>) {
      return ::sus::Tuple<const std::remove_reference_t<Ts>&...>(
          tuple_.template at<Is>()...);
    }(std::make_index_sequence<sizeof...(Ts)>());
  }
  constexpr auto as_mut() & {
    return [this]<size_t... Is>(std::index_sequence<Is...>) {
      return ::sus::Tuple<Ts&...>(tuple_.template at_mut<Is>()...);
    }(std::make_index_sequence<sizeof...(Ts)>());
  }
  inline constexpr auto into_inner() && { return ::sus::move(tuple_); }

  [[sus_no_unique_address]] Type tuple_;
  [[sus_no_unique_address]] Storage<I + 1, Elements...> more_;
};

template <size_t I, class... Elements>
  requires(sizeof...(Elements) > 0)
union Storage<I, Nothing, Elements...> {
  constexpr Storage() {}
  ~Storage()
    requires(... && std::is_trivially_destructible_v<Elements>)
  = default;
  constexpr ~Storage()
    requires(!(... && std::is_trivially_destructible_v<Elements>))
  {}

  Storage(const Storage&)
    requires(... && std::is_trivially_copy_constructible_v<Elements>)
  = default;
  Storage& operator=(const Storage&)
    requires(... && std::is_trivially_copy_assignable_v<Elements>)
  = default;
  Storage(Storage&&)
    requires(... && std::is_trivially_move_constructible_v<Elements>)
  = default;
  Storage& operator=(Storage&&)
    requires(... && std::is_trivially_move_assignable_v<Elements>)
  = default;

  inline constexpr void activate_for_construct(size_t index) {
    if (index != I) {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.activate_for_construct(index);
    }
  }
  inline constexpr void move_construct(size_t index, Storage&& from) {
    if (index != I) {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.move_construct(index, ::sus::move(from.more_));
    }
  }
  inline constexpr void move_assign(size_t index, Storage&& from) {
    if (index != I) {
      more_.move_assign(index, ::sus::move(from.more_));
    }
  }
  inline constexpr void copy_construct(size_t index, const Storage& from) {
    if (index != I) {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.copy_construct(index, from.more_);
    }
  }
  inline constexpr void copy_assign(size_t index, const Storage& from) {
    if (index != I) {
      more_.copy_assign(index, from.more_);
    }
  }
  inline constexpr void clone_construct(size_t index, const Storage& from) {
    if (index != I) {
      more_.clone_construct(index, from.more_);
    }
  }
  inline constexpr void destroy(size_t index) {
    if (index != I) {
      more_.destroy(index);
      more_.~Storage<I + 1, Elements...>();
    }
  }
  inline constexpr bool eq(size_t index, const Storage& other) const& {
    if (index == I) {
      return true;
    } else {
      return more_.eq(index, other.more_);
    }
  }
  inline constexpr auto strong_ord(size_t index, const Storage& other) const& {
    if (index == I) {
      return std::strong_ordering::equivalent;
    } else {
      return more_.strong_ord(index, other.more_);
    }
  }
  inline constexpr auto weak_ord(size_t index, const Storage& other) const& {
    if (index == I) {
      return std::weak_ordering::equivalent;
    } else {
      return more_.weak_ord(index, other.more_);
    }
  }
  inline constexpr auto partial_ord(size_t index, const Storage& other) const& {
    if (index == I) {
      return std::partial_ordering::equivalent;
    } else {
      return more_.partial_ord(index, other.more_);
    }
  }

  [[sus_no_unique_address]] Storage<I + 1, Elements...> more_;
};

template <size_t I, class T, class... Elements>
  requires(sizeof...(Elements) > 0)
union Storage<I, ::sus::Tuple<T>, Elements...> {
  constexpr Storage() {}
  ~Storage()
    requires(std::is_trivially_destructible_v<::sus::Tuple<T>> && ... &&
             std::is_trivially_destructible_v<Elements>)
  = default;
  constexpr ~Storage()
    requires(!(std::is_trivially_destructible_v<::sus::Tuple<T>> && ... &&
               std::is_trivially_destructible_v<Elements>))
  {}

  Storage(const Storage&)
    requires(std::is_trivially_copy_constructible_v<::sus::Tuple<T>> && ... &&
             std::is_trivially_copy_constructible_v<Elements>)
  = default;
  Storage& operator=(const Storage&)
    requires(std::is_trivially_copy_assignable_v<::sus::Tuple<T>> && ... &&
             std::is_trivially_copy_assignable_v<Elements>)
  = default;
  Storage(Storage&&)
    requires(std::is_trivially_move_constructible_v<::sus::Tuple<T>> && ... &&
             std::is_trivially_move_constructible_v<Elements>)
  = default;
  Storage& operator=(Storage&&)
    requires(std::is_trivially_move_assignable_v<::sus::Tuple<T>> && ... &&
             std::is_trivially_move_assignable_v<Elements>)
  = default;

  using Type = ::sus::Tuple<T>;

  inline constexpr void activate_for_construct(size_t index) {
    if (index != I) {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.activate_for_construct(index);
    }
  }
  template <class U>
  inline constexpr void construct(U&& value) {
    std::construct_at(&tuple_, ::sus::Tuple<T>(::sus::forward<U>(value)));
  }
  inline constexpr void assign(T&& value) {
    tuple_ = Type(::sus::forward<T>(value));
  }
  inline constexpr void move_construct(size_t index, Storage&& from) {
    if (index == I) {
      std::construct_at(&tuple_, ::sus::move(from.tuple_));
    } else {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.move_construct(index, ::sus::move(from.more_));
    }
  }
  inline constexpr void move_assign(size_t index, Storage&& from) {
    if (index == I) {
      tuple_ = ::sus::move(from.tuple_);
    } else {
      more_.move_assign(index, ::sus::move(from.more_));
    }
  }
  inline constexpr void copy_construct(size_t index, const Storage& from) {
    if (index == I) {
      std::construct_at(&tuple_, from.tuple_);
    } else {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.copy_construct(index, from.more_);
    }
  }
  inline constexpr void copy_assign(size_t index, const Storage& from) {
    if (index == I) {
      tuple_ = from.tuple_;
    } else {
      more_.copy_assign(index, from.more_);
    }
  }
  inline constexpr void clone_construct(size_t index, const Storage& from) {
    if (index == I) {
      auto x = ::sus::clone(from.tuple_);
      std::construct_at(&tuple_, sus::move(x));
    } else {
      std::construct_at(&more_);  // Make the more_ member active.
      more_.clone_construct(index, from.more_);
    }
  }
  inline constexpr void destroy(size_t index) {
    if (index == I) {
      tuple_.~Type();
    } else {
      more_.destroy(index);
      more_.~Storage<I + 1, Elements...>();
    }
  }
  inline constexpr bool eq(size_t index, const Storage& other) const& {
    if (index == I) {
      return tuple_ == other.tuple_;
    } else {
      return more_.eq(index, other.more_);
    }
  }
  inline constexpr auto strong_ord(size_t index, const Storage& other) const& {
    if (index == I) {
      return std::strong_order(tuple_, other.tuple_);
    } else {
      return more_.strong_ord(index, other.more_);
    }
  }
  inline constexpr auto weak_ord(size_t index, const Storage& other) const& {
    if (index == I) {
      return std::weak_order(tuple_, other.tuple_);
    } else {
      return more_.weak_ord(index, other.more_);
    }
  }
  inline constexpr auto partial_ord(size_t index, const Storage& other) const& {
    if (index == I) {
      return std::partial_order(tuple_, other.tuple_);
    } else {
      return more_.partial_ord(index, other.more_);
    }
  }

  inline constexpr decltype(auto) as() const& {
    return tuple_.template at<0>();
  }
  inline constexpr decltype(auto) as_mut() & {
    return tuple_.template at_mut<0>();
  }
  inline constexpr decltype(auto) into_inner() && {
    return ::sus::move(tuple_).template into_inner<0>();
  }

  [[sus_no_unique_address]] Type tuple_;
  [[sus_no_unique_address]] Storage<I + 1, Elements...> more_;
};

template <size_t I, class... Ts>
  requires(sizeof...(Ts) > 1)
union Storage<I, ::sus::Tuple<Ts...>> {
  constexpr Storage() {}
  ~Storage()
    requires(std::is_trivially_destructible_v<::sus::Tuple<Ts...>>)
  = default;
  constexpr ~Storage()
    requires(!(std::is_trivially_destructible_v<::sus::Tuple<Ts...>>))
  {}

  Storage(const Storage&)
    requires(std::is_trivially_copy_constructible_v<::sus::Tuple<Ts...>>)
  = default;
  Storage& operator=(const Storage&)
    requires(std::is_trivially_copy_assignable_v<::sus::Tuple<Ts...>>)
  = default;
  Storage(Storage&&)
    requires(std::is_trivially_move_constructible_v<::sus::Tuple<Ts...>>)
  = default;
  Storage& operator=(Storage&&)
    requires(std::is_trivially_move_assignable_v<::sus::Tuple<Ts...>>)
  = default;

  using Type = ::sus::Tuple<Ts...>;

  inline constexpr void activate_for_construct(size_t index) {
    ::sus::check(index == I);
  }
  inline constexpr void construct(Type&& tuple) {
    std::construct_at(&tuple_, ::sus::move(tuple));
  }
  inline constexpr void assign(Type&& tuple) { tuple_ = ::sus::move(tuple); }
  inline constexpr void move_construct(size_t index, Storage&& from) {
    ::sus::check(index == I);
    std::construct_at(&tuple_, ::sus::move(from.tuple_));
  }
  inline constexpr void move_assign(size_t index, Storage&& from) {
    ::sus::check(index == I);
    tuple_ = ::sus::move(from.tuple_);
  }
  inline constexpr void copy_construct(size_t index, const Storage& from) {
    ::sus::check(index == I);
    std::construct_at(&tuple_, from.tuple_);
  }
  inline constexpr void copy_assign(size_t index, const Storage& from) {
    ::sus::check(index == I);
    tuple_ = from.tuple_;
  }
  inline constexpr void clone_construct(size_t index, const Storage& from) {
    ::sus::check(index == I);
    std::construct_at(&tuple_, ::sus::clone(from.tuple_));
  }
  inline constexpr void destroy(size_t index) {
    ::sus::check(index == I);
    tuple_.~Type();
  }
  inline constexpr bool eq(size_t index, const Storage& other) const& noexcept {
    ::sus::check(index == I);
    return tuple_ == other.tuple_;
  }
  inline constexpr auto strong_ord(size_t index,
                                   const Storage& other) const& noexcept {
    ::sus::check(index == I);
    return std::strong_order(tuple_, other.tuple_);
  }
  inline constexpr auto weak_ord(size_t index,
                                 const Storage& other) const& noexcept {
    ::sus::check(index == I);
    return std::weak_order(tuple_, other.tuple_);
  }
  inline constexpr auto partial_ord(size_t index,
                                    const Storage& other) const& noexcept {
    ::sus::check(index == I);
    return std::partial_order(tuple_, other.tuple_);
  }

  constexpr auto as() const& {
    return [this]<size_t... Is>(std::index_sequence<Is...>) {
      return ::sus::Tuple<const std::remove_reference_t<Ts>&...>(
          tuple_.template at<Is>()...);
    }(std::make_index_sequence<sizeof...(Ts)>());
  }
  constexpr auto as_mut() & {
    return [this]<size_t... Is>(std::index_sequence<Is...>) {
      return ::sus::Tuple<Ts&...>(tuple_.template at_mut<Is>()...);
    }(std::make_index_sequence<sizeof...(Ts)>());
  }
  inline constexpr auto into_inner() && { return ::sus::move(tuple_); }

  [[sus_no_unique_address]] Type tuple_;
};

template <size_t I>
union Storage<I, Nothing> {
  constexpr Storage() {}

  inline constexpr void activate_for_construct(size_t index) {
    ::sus::check(index == I);
  }
  inline constexpr void move_construct(size_t index, Storage&&) {
    ::sus::check(index == I);
  }
  inline constexpr void move_assign(size_t index, Storage&&) {
    ::sus::check(index == I);
  }
  inline constexpr void copy_construct(size_t index, const Storage&) {
    ::sus::check(index == I);
  }
  inline constexpr void copy_assign(size_t index, const Storage&) {
    ::sus::check(index == I);
  }
  inline constexpr void clone_construct(size_t index, const Storage&) {
    ::sus::check(index == I);
  }
  inline constexpr void destroy(size_t index) { ::sus::check(index == I); }
  inline constexpr bool eq(size_t index, const Storage&) const& {
    ::sus::check(index == I);
    return true;
  }
  inline constexpr auto strong_ord(size_t index, const Storage&) const& {
    ::sus::check(index == I);
    return std::strong_ordering::equivalent;
  }
  inline constexpr auto weak_ord(size_t index, const Storage&) const& {
    ::sus::check(index == I);
    return std::weak_ordering::equivalent;
  }
  inline constexpr auto partial_ord(size_t index, const Storage&) const& {
    ::sus::check(index == I);
    return std::partial_ordering::equivalent;
  }
};

template <size_t I, class T>
union Storage<I, ::sus::Tuple<T>> {
  constexpr Storage() {}
  ~Storage()
    requires(std::is_trivially_destructible_v<::sus::Tuple<T>>)
  = default;
  constexpr ~Storage()
    requires(!(std::is_trivially_destructible_v<::sus::Tuple<T>>))
  {}

  Storage(const Storage&)
    requires(std::is_trivially_copy_constructible_v<::sus::Tuple<T>>)
  = default;
  Storage& operator=(const Storage&)
    requires(std::is_trivially_copy_assignable_v<::sus::Tuple<T>>)
  = default;
  Storage(Storage&&)
    requires(std::is_trivially_move_constructible_v<::sus::Tuple<T>>)
  = default;
  Storage& operator=(Storage&&)
    requires(std::is_trivially_move_assignable_v<::sus::Tuple<T>>)
  = default;

  using Type = ::sus::Tuple<T>;

  inline constexpr void activate_for_construct(size_t index) {
    ::sus::check(index == I);
  }
  template <class U>
  inline constexpr void construct(U&& value) {
    std::construct_at(&tuple_, ::sus::Tuple<T>(::sus::forward<U>(value)));
  }
  inline constexpr void assign(T&& value) {
    tuple_ = Type(::sus::forward<T>(value));
  }
  inline constexpr void move_construct(size_t index, Storage&& from) {
    ::sus::check(index == I);
    std::construct_at(&tuple_, ::sus::move(from.tuple_));
  }
  inline constexpr void move_assign(size_t index, Storage&& from) {
    ::sus::check(index == I);
    tuple_ = ::sus::move(from.tuple_);
  }
  inline constexpr void copy_construct(size_t index, const Storage& from) {
    ::sus::check(index == I);
    std::construct_at(&tuple_, from.tuple_);
  }
  inline constexpr void copy_assign(size_t index, const Storage& from) {
    ::sus::check(index == I);
    tuple_ = from.tuple_;
  }
  inline constexpr void clone_construct(size_t index, const Storage& from) {
    ::sus::check(index == I);
    std::construct_at(&tuple_, ::sus::clone(from.tuple_));
  }
  inline constexpr void destroy(size_t index) {
    ::sus::check(index == I);
    tuple_.~Type();
  }
  inline constexpr bool eq(size_t index, const Storage& other) const& {
    ::sus::check(index == I);
    return tuple_ == other.tuple_;
  }
  inline constexpr auto strong_ord(size_t index, const Storage& other) const& {
    ::sus::check(index == I);
    return std::strong_order(tuple_, other.tuple_);
  }
  inline constexpr auto weak_ord(size_t index, const Storage& other) const& {
    ::sus::check(index == I);
    return std::weak_order(tuple_, other.tuple_);
  }
  inline constexpr auto partial_ord(size_t index, const Storage& other) const& {
    ::sus::check(index == I);
    return std::partial_order(tuple_, other.tuple_);
  }

  inline constexpr decltype(auto) as() const& {
    return tuple_.template at<0>();
  }
  inline constexpr decltype(auto) as_mut() & {
    return tuple_.template at_mut<0>();
  }
  inline constexpr decltype(auto) into_inner() && {
    return ::sus::move(tuple_).template into_inner<0>();
  }

  [[sus_no_unique_address]] ::sus::Tuple<T> tuple_;
};

template <size_t I, class S>
static constexpr auto& construct_choice_storage(S& storage) {
  return construct_choice_storage(storage, std::integral_constant<size_t, I>());
}

template <size_t I, class S>
static constexpr auto& construct_choice_storage(
    S& storage, std::integral_constant<size_t, I>) {
  std::construct_at(&storage.more_);
  return construct_choice_storage(storage.more_,
                                  std::integral_constant<size_t, I - 1u>());
}

template <class S>
static constexpr auto& construct_choice_storage(
    S& storage, std::integral_constant<size_t, 0>) {
  return storage;
}

template <size_t I, class S>
static constexpr const auto& find_choice_storage(const S& storage) {
  return find_choice_storage(storage, std::integral_constant<size_t, I>());
}

template <size_t I, class S>
static constexpr const auto& find_choice_storage(
    const S& storage, std::integral_constant<size_t, I>) {
  return find_choice_storage(storage.more_,
                             std::integral_constant<size_t, I - 1u>());
}

template <class S>
static constexpr const auto& find_choice_storage(
    const S& storage, std::integral_constant<size_t, 0>) {
  return storage;
}

template <size_t I, class S>
static constexpr auto& find_choice_storage_mut(S& storage) {
  return find_choice_storage_mut(storage, std::integral_constant<size_t, I>());
}

template <size_t I, class S>
static constexpr auto& find_choice_storage_mut(
    S& storage, std::integral_constant<size_t, I>) {
  return find_choice_storage_mut(storage.more_,
                                 std::integral_constant<size_t, I - 1u>());
}

template <class S>
static constexpr auto& find_choice_storage_mut(
    S& storage, std::integral_constant<size_t, 0>) {
  return storage;
}

}  // namespace sus::choice_type::__private
