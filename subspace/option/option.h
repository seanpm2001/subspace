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

#include <optional>  // TODO: Make this.. optional?
#include <type_traits>

#include "fmt/format.h"
#include "subspace/assertions/check.h"
#include "subspace/assertions/unreachable.h"
#include "subspace/construct/default.h"
#include "subspace/construct/into.h"
#include "subspace/fn/fn_concepts.h"
#include "subspace/iter/from_iterator.h"
#include "subspace/iter/into_iterator.h"
#include "subspace/iter/iterator_concept.h"
#include "subspace/iter/product.h"
#include "subspace/iter/sum.h"
#include "subspace/lib/__private/forward_decl.h"
#include "subspace/macros/inline.h"
#include "subspace/macros/lifetimebound.h"
#include "subspace/macros/nonnull.h"
#include "subspace/macros/pure.h"
#include "subspace/marker/unsafe.h"
#include "subspace/mem/__private/ref_concepts.h"
#include "subspace/mem/clone.h"
#include "subspace/mem/copy.h"
#include "subspace/mem/forward.h"
#include "subspace/mem/move.h"
#include "subspace/mem/relocate.h"
#include "subspace/mem/replace.h"
#include "subspace/mem/take.h"
#include "subspace/ops/eq.h"
#include "subspace/ops/ord.h"
#include "subspace/ops/try.h"
#include "subspace/option/__private/is_option_type.h"
#include "subspace/option/__private/is_tuple_type.h"
#include "subspace/option/__private/marker.h"
#include "subspace/option/__private/storage.h"
#include "subspace/option/state.h"
#include "subspace/result/__private/is_result_type.h"
#include "subspace/string/__private/any_formatter.h"
#include "subspace/string/__private/format_to_stream.h"

// Have to forward declare a bunch of iterator stuff here because Iterator
// includes Option, so it can't include iterator back.
namespace sus::iter {
template <class Item>
class Once;
template <class Item>
Once<Item> once(::sus::option::Option<Item> o) noexcept;
namespace __private {
template <class T>
  requires requires(const T& t) {
    { t.iter() };
  }
constexpr auto begin(const T& t) noexcept;
template <class T>
  requires requires(const T& t) {
    { t.iter() };
  }
constexpr auto end(const T& t) noexcept;
}  // namespace __private
}  // namespace sus::iter

namespace sus::option {

using State::None;
using State::Some;
using ::sus::iter::Once;
using ::sus::mem::__private::IsTrivialCopyAssignOrRef;
using ::sus::mem::__private::IsTrivialCopyCtorOrRef;
using ::sus::mem::__private::IsTrivialDtorOrRef;
using ::sus::mem::__private::IsTrivialMoveAssignOrRef;
using ::sus::mem::__private::IsTrivialMoveCtorOrRef;
using ::sus::option::__private::Storage;
using ::sus::option::__private::StoragePointer;

namespace __private {
template <class T, ::sus::iter::Iterator<Option<T>> Iter>
  requires ::sus::iter::Product<T>
constexpr Option<T> from_product_impl(Iter&& it) noexcept;
template <class T, ::sus::iter::Iterator<Option<T>> Iter>
  requires ::sus::iter::Sum<T>
constexpr Option<T> from_sum_impl(Iter&& it) noexcept;
}  // namespace __private

/// A type which either holds `Some` value of type `T`, or `None`.
///
/// To immediately pull the inner value out of an Option, use `unwrap()`. If the
/// `Option` is an lvalue, use `operator*` and `operator->` to access the inner
/// value. However if doing this many times, consider doing `unwrap()` a single
/// time up front.
///
/// `Option<const T>` for non-reference-type `T` is disallowed, as the Option
/// owns the `T` in that case and it ensures the `Option` and the `T` are both
/// accessed with the same const-ness.
///
/// If a type `T` is a reference or satisties `sus::mem::NeverValueField`, then
/// `Option<T>` will have the same size as T and will be internally represented
/// as just a `T` (or `T*` in the case of a reference `T&`).
///
/// # Copying
///
/// Most methods of Option act on an rvalue and consume the Option to transform
/// it into a new Option with a new value. This ensures that the value inside an
/// Option is moved while transforming it.
///
/// However, if `Option` is `Copy`, then the majority of methods offer an
/// overload to be called as an lvalue, in which case the `Option` will copy
/// itself, and its contained value, and perform the intended method on the copy
/// instead. This can have performance implications!
///
/// The unwrapping methods are excluded from this, and are only available on an
/// rvalue Option to avoid copying just to access the inner value. To do that,
/// access the inner value as a reference through `as_value()` and
/// `as_value_mut()` or the `*` operator.
///
/// # Returning references
///
/// Methods that return references are only callable on an rvalue Option if the
/// Option is holding a reference. If the Option is holding a non-reference
/// type, returning a reference from an rvalue Option would be giving a
/// reference to a short-lived object which is a bugprone pattern in C++ leading
/// to memory safety bugs.
template <class T>
class Option final {
  // Note that `const T&` is allowed (so we don't `std::remove_reference_t<T>`)
  // as it would require dropping the const qualification to copy that into the
  // Option as a `T&`.
  static_assert(!std::is_const_v<T>,
                "`Option<const T>` should be written `const Option<T>`, as "
                "const applies transitively.");
  static_assert(
      !std::is_rvalue_reference_v<T>,
      "`Option<T&&> is not supported, use Option<T&> or Option<const T&.");

 public:
  /// Default-construct an Option that is holding no value.
  inline constexpr Option() noexcept = default;

  /// Construct an Option that is holding the given value.
  ///
  /// # Const References
  ///
  /// For `Option<const T&>` it is possible to bind to a temporary which would
  /// create a memory safety bug. The `[[clang::lifetimebound]]` attribute is
  /// used to prevent this via Clang. But additionally, the incoming type is
  /// required to match with `sus::construct::SafelyConstructibleFromReference`
  /// to prevent conversions that would construct a temporary.
  ///
  /// To force accepting a const reference anyway in cases where a type can
  /// convert to a reference without constructing a temporary, use an unsafe
  /// `static_cast<const T&>()` at the callsite (and document it =)).
  static inline constexpr Option with(const T& t) noexcept
    requires(!std::is_reference_v<T> && ::sus::mem::Copy<T>)
  {
    return Option(t);
  }

  static inline constexpr Option with(T&& t) noexcept
    requires(!std::is_reference_v<T>)
  {
    if constexpr (::sus::mem::Move<T>) {
      return Option(move_to_storage(t));
    } else {
      static_assert(::sus::mem::Copy<T>, "All types should be Copy or Move.");
      return Option(t);
    }
  }

  template <std::convertible_to<T> U>
  sus_pure static inline constexpr Option with(U&& t sus_lifetimebound) noexcept
    requires(std::is_reference_v<T> &&
             sus::construct::SafelyConstructibleFromReference<T, U &&>)
  {
    return Option(move_to_storage(t));
  }

  /// Moves `val` into a new `Option<T>` holding `Some(val)`.
  ///
  /// Implements `sus::construct::From<Option<T>, T>`.
  ///
  /// #[doc.overloads=from.t]
  static constexpr Option from(const T& val) noexcept
    requires(!std::is_reference_v<T> && ::sus::mem::Copy<T>)
  {
    return with(val);
  }
  static constexpr Option from(T&& val) noexcept
    requires(!std::is_reference_v<T>)
  {
    return with(::sus::move(val));
  }
  template <std::convertible_to<T> U>
  sus_pure static constexpr Option from(U&& val sus_lifetimebound) noexcept
    requires(std::is_reference_v<T> &&
             sus::construct::SafelyConstructibleFromReference<T, U &&>)
  {
    return with(::sus::forward<U>(val));
  }

  /// Computes the product of an iterator over `Option<T>` as long as there is
  /// no `None` found. If a `None` is found, the function returns `None`.
  ///
  /// Implements sus::iter::Product<Option<T>>.
  ///
  /// The product is computed using the implementation of the inner type `T`
  /// which also satisfies `sus::iter::Product<T>`.
  template <::sus::iter::Iterator<Option<T>> Iter>
    requires ::sus::iter::Product<T>
  static constexpr Option from_product(Iter&& it) noexcept {
    return __private::from_product_impl<T>(::sus::move(it));
  }

  /// Computes the sum of an iterator over `Option<T>` as long as there is
  /// no `None` found. If a `None` is found, the function returns `None`.
  ///
  /// Implements sus::iter::Sum<Option<T>>.
  ///
  /// The sum is computed using the implementation of the inner type `T`
  /// which also satisfies `sus::iter::Sum<T>`.
  template <::sus::iter::Iterator<Option<T>> Iter>
    requires ::sus::iter::Sum<T>
  static constexpr Option from_sum(Iter&& it) noexcept {
    return __private::from_sum_impl<T>(::sus::move(it));
  }

  /// Destructor for the Option.
  ///
  /// Destroys the value contained within the option, if there is one.
  ///
  /// If T can be trivially destroyed, we don't need to explicitly destroy it,
  /// so we can use the default destructor, which allows Option<T> to also be
  /// trivially destroyed.
  constexpr ~Option() noexcept
    requires(IsTrivialDtorOrRef<T>)
  = default;

  constexpr inline ~Option() noexcept
    requires(!IsTrivialDtorOrRef<T>)
  {
    if (t_.state() == Some) t_.destroy();
  }

  // If T can be trivially copy-constructed, Option<T> can also be trivially
  // copy-constructed.
  constexpr Option(const Option& o)
    requires(::sus::mem::CopyOrRef<T> && IsTrivialCopyCtorOrRef<T>)
  = default;

  constexpr Option(const Option& o) noexcept
    requires(::sus::mem::CopyOrRef<T> && !IsTrivialCopyCtorOrRef<T>)
  {
    if (o.t_.state() == Some)
      t_.construct_from_none(copy_to_storage(o.t_.val()));
  }

  constexpr Option(const Option& o)
    requires(!::sus::mem::CopyOrRef<T>)
  = delete;

  // If T can be trivially move-constructed, Option<T> can also be trivially
  // move-constructed.
  constexpr Option(Option&& o)
    requires(::sus::mem::MoveOrRef<T> && IsTrivialMoveCtorOrRef<T>)
  = default;

  constexpr Option(Option&& o) noexcept
    requires(::sus::mem::MoveOrRef<T> && !IsTrivialMoveCtorOrRef<T>)
  {
    if (o.t_.state() == Some) t_.construct_from_none(o.t_.take_and_set_none());
  }

  constexpr Option(Option&& o)
    requires(!::sus::mem::MoveOrRef<T>)
  = delete;

  // If T can be trivially copy-assigned, Option<T> can also be trivially
  // copy-assigned.
  constexpr Option& operator=(const Option& o)
    requires(::sus::mem::CopyOrRef<T> && IsTrivialCopyAssignOrRef<T>)
  = default;

  constexpr Option& operator=(const Option& o) noexcept
    requires(::sus::mem::CopyOrRef<T> && !IsTrivialCopyAssignOrRef<T>)
  {
    if (o.t_.state() == Some)
      t_.set_some(copy_to_storage(o.t_.val()));
    else if (t_.state() == Some)
      t_.set_none();
    return *this;
  }

  constexpr Option& operator=(const Option& o)
    requires(!::sus::mem::CopyOrRef<T>)
  = delete;

  // If T can be trivially move-assigned, we don't need to explicitly construct
  // it, so we can use the default destructor, which allows Option<T> to also
  // be trivially move-assigned.
  constexpr Option& operator=(Option&& o)
    requires(::sus::mem::MoveOrRef<T> && IsTrivialMoveAssignOrRef<T>)
  = default;

  constexpr Option& operator=(Option&& o) noexcept
    requires(::sus::mem::MoveOrRef<T> && !IsTrivialMoveAssignOrRef<T>)
  {
    if (o.t_.state() == Some)
      t_.set_some(o.t_.take_and_set_none());
    else if (t_.state() == Some)
      t_.set_none();
    return *this;
  }

  constexpr Option& operator=(Option&& o)
    requires(!::sus::mem::MoveOrRef<T>)
  = delete;

  /// sus::mem::Clone trait.
  constexpr Option clone() const& noexcept
    requires(::sus::mem::Clone<T> && !::sus::mem::CopyOrRef<T>)
  {
    if (t_.state() == Some)
      return Option(::sus::clone(t_.val()));
    else
      return Option();
  }

  /// sus::mem::CloneInto trait.
  constexpr void clone_from(const Option& source) & noexcept
    requires(::sus::mem::Clone<T> && !::sus::mem::CopyOrRef<T>)
  {
    if (&source == this) [[unlikely]]
      return;
    if (t_.state() == Some && source.t_.state() == Some) {
      ::sus::clone_into(t_.val_mut(), source.t_.val());
    } else {
      *this = source.clone();
    }
  }

  /// Returns whether the Option currently contains a value.
  ///
  /// If there is a value present, it can be extracted with `unwrap()` or
  /// `expect()`, or can be accessed through `operator->` and `operator*`.
  sus_pure constexpr bool is_some() const noexcept {
    return t_.state() == Some;
  }
  /// Returns whether the Option is currently empty, containing no value.
  sus_pure constexpr bool is_none() const noexcept {
    return t_.state() == None;
  }

  /// An operator which returns the state of the Option, either `Some` or
  /// `None`.
  ///
  /// This supports the use of an Option in a switch, allowing it to act as
  /// a tagged union between "some value" and "no value".
  ///
  /// # Example
  ///
  /// ```cpp
  /// auto x = Option<int>::with(2);
  /// switch (x) {
  ///  case Some:
  ///   return sus::move(x).unwrap_unchecked(unsafe_fn);
  ///  case None:
  ///   return -1;
  /// }
  /// ```
  sus_pure constexpr operator State() const& { return t_.state(); }

  /// Returns the contained value inside the Option.
  ///
  /// The function will panic with the given message if the Option's state is
  /// currently `None`.
  constexpr sus_nonnull_fn T expect(
      /* TODO: string view type */ sus_nonnull_arg const char* sus_nonnull_var
          msg) && noexcept {
    ::sus::check_with_message(t_.state() == Some, *msg);
    return ::sus::move(*this).unwrap_unchecked(::sus::marker::unsafe_fn);
  }

  /// Returns the contained value inside the Option.
  ///
  /// The function will panic without a message if the Option's state is
  /// currently `None`.
  constexpr T unwrap() && noexcept {
    ::sus::check(t_.state() == Some);
    return ::sus::move(*this).unwrap_unchecked(::sus::marker::unsafe_fn);
  }

  /// Returns the contained value inside the Option, if there is one. Otherwise,
  /// returns `default_result`.
  ///
  /// Note that if it is non-trivial to construct a `default_result`, that
  /// <unwrap_or_else>() should be used instead, as it will only construct the
  /// default value if required.
  constexpr T unwrap_or(T default_result) && noexcept {
    if (t_.state() == Some) {
      return t_.take_and_set_none();
    } else {
      return default_result;
    }
  }

  /// Returns the contained value inside the Option, if there is one.
  /// Otherwise, returns the result of the given function.
  constexpr T unwrap_or_else(::sus::fn::FnOnce<T()> auto&& f) && noexcept {
    if (t_.state() == Some) {
      return t_.take_and_set_none();
    } else {
      return ::sus::fn::call_once(::sus::move(f));
    }
  }

  /// Returns the contained value inside the Option, if there is one.
  /// Otherwise, constructs a default value for the type and returns that.
  ///
  /// The Option's contained type `T` must be #Default, and will be
  /// constructed through that trait.
  constexpr T unwrap_or_default() && noexcept
    requires(!std::is_reference_v<T> && ::sus::construct::Default<T>)
  {
    if (t_.state() == Some) {
      return t_.take_and_set_none();
    } else {
      return T();
    }
  }

  /// Returns the contained value inside the Option.
  ///
  /// # Safety
  ///
  /// It is Undefined Behaviour to call this function when the Option's state is
  /// `None`. The caller is responsible for ensuring the Option contains a value
  /// beforehand, and the safer `unwrap()` or `expect()` should almost always be
  /// preferred.
  constexpr inline T unwrap_unchecked(
      ::sus::marker::UnsafeFnMarker) && noexcept {
    return t_.take_and_set_none();
  }

  /// Returns a reference to the contained value inside the Option.
  ///
  /// To access a (non-reference) value inside an rvalue Option, use unwrap() to
  /// extract the value.
  ///
  /// # Panic
  ///
  /// The function will panic without a message if the Option's state is
  /// currently `None`.
  ///
  /// # Implementation Notes
  ///
  /// Implementation note: We only allow calling this on an rvalue Option if the
  /// contained value is a reference, otherwise we are returning a reference to
  /// a short-lived object which leads to common C++ memory bugs.
  sus_pure constexpr const std::remove_reference_t<T>& as_value()
      const& noexcept {
    ::sus::check(t_.state() == Some);
    return t_.val();
  }
  sus_pure constexpr const std::remove_reference_t<T>& as_value() && noexcept
    requires(std::is_reference_v<T>)
  {
    ::sus::check(t_.state() == Some);
    return t_.val();
  }
  sus_pure constexpr std::remove_reference_t<T>& as_value_mut() & noexcept {
    ::sus::check(t_.state() == Some);
    return t_.val_mut();
  }
  sus_pure constexpr std::remove_reference_t<T>& as_value_mut() && noexcept
    requires(std::is_reference_v<T>)
  {
    ::sus::check(t_.state() == Some);
    return t_.val_mut();
  }
  sus_pure constexpr std::remove_reference_t<T>& as_value_mut() const& noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::clone(*this).as_value_mut();
  }

  sus_pure constexpr const std::remove_reference_t<T>& as_value_unchecked(
      ::sus::marker::UnsafeFnMarker) const& noexcept {
    return t_.val();
  }
  sus_pure constexpr const std::remove_reference_t<T>& as_value_unchecked(
      ::sus::marker::UnsafeFnMarker) && noexcept
    requires(std::is_reference_v<T>)
  {
    return t_.val();
  }
  sus_pure constexpr std::remove_reference_t<T>& as_value_unchecked_mut(
      ::sus::marker::UnsafeFnMarker) & noexcept {
    return t_.val_mut();
  }
  sus_pure constexpr std::remove_reference_t<T>& as_value_unchecked_mut(
      ::sus::marker::UnsafeFnMarker) && noexcept
    requires(std::is_reference_v<T>)
  {
    return t_.val_mut();
  }
  sus_pure constexpr std::remove_reference_t<T>& as_value_unchecked_mut(
      ::sus::marker::UnsafeFnMarker) const& noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::clone(*this).as_value_unchecked_mut(::sus::marker::unsafe_fn);
  }

  /// Returns a reference to the contained value inside the Option.
  ///
  /// The reference is const if the Option is const, and is mutable otherwise.
  /// This method allows calling methods directly on the type inside the Option
  /// without unwrapping.
  ///
  /// To access a (non-reference) value inside an rvalue Option, use unwrap() to
  /// extract the value.
  ///
  /// # Panic
  ///
  /// The function will panic without a message if the Option's state is
  /// currently `None`.
  ///
  /// # Implementation Notes
  ///
  /// Implementation note: We only allow calling this on an rvalue Option if the
  /// contained value is a reference, otherwise we are returning a reference to
  /// a short-lived object which leads to common C++ memory bugs.
  ///
  /// Implementation note: This method is added in addition to the Rust Option
  /// API because:
  /// * C++ moving is verbose, making unwrap() on lvalues loud.
  /// * Unwrapping requires a new lvalue name since C++ doesn't allow name
  ///   reuse, making variable names bad.
  /// * It's expected due to std::optional and general container-of-one things
  ///   to provide access through operator* and operator->.
  sus_pure constexpr const std::remove_reference_t<T>& operator*()
      const& noexcept {
    ::sus::check(t_.state() == Some);
    return t_.val();
  }
  sus_pure constexpr const std::remove_reference_t<T>& operator*() && noexcept
    requires(std::is_reference_v<T>)
  {
    ::sus::check(t_.state() == Some);
    return t_.val();
  }
  sus_pure constexpr std::remove_reference_t<T>& operator*() & noexcept {
    ::sus::check(t_.state() == Some);
    return t_.val_mut();
  }

  /// Returns a pointer to the contained value inside the Option.
  ///
  /// The pointer is const if the Option is const, and is mutable otherwise.
  /// This method allows calling methods directly on the type inside the Option
  /// without unwrapping.
  ///
  /// To access a (non-reference) value inside an rvalue Option, use unwrap() to
  /// extract the value.
  ///
  /// # Panic
  ///
  /// The function will panic without a message if the Option's state is
  /// currently `None`.
  ///
  /// # Implementation Notes
  ///
  /// Implementation note: We only allow calling this on an rvalue Option if the
  /// contained value is a reference, otherwise we are returning a reference to
  /// a short-lived object which leads to common C++ memory bugs.
  ///
  /// Implementation note: This method is added in addition to the Rust Option
  /// API because:
  /// * C++ moving is verbose, making unwrap() on lvalues loud.
  /// * Unwrapping requires a new lvalue name since C++ doesn't allow name
  ///   reuse, making variable names bad.
  /// * It's expected due to std::optional and general container-of-one things
  ///   to provide access through operator* and operator->.
  sus_pure constexpr const std::remove_reference_t<T>* operator->()
      const& noexcept {
    ::sus::check(t_.state() == Some);
    return ::sus::mem::addressof(
        static_cast<const std::remove_reference_t<T>&>(t_.val()));
  }
  sus_pure constexpr const std::remove_reference_t<T>* operator->() && noexcept
    requires(std::is_reference_v<T>)
  {
    ::sus::check(t_.state() == Some);
    return ::sus::mem::addressof(
        static_cast<const std::remove_reference_t<T>&>(t_.val()));
  }
  sus_pure constexpr std::remove_reference_t<T>* operator->() & noexcept {
    ::sus::check(t_.state() == Some);
    return ::sus::mem::addressof(
        static_cast<std::remove_reference_t<T>&>(t_.val_mut()));
  }

  /// Stores the value `t` inside this Option, replacing any previous value, and
  /// returns a mutable reference to the new value.
  constexpr T& insert(T t) & noexcept
    requires(sus::mem::MoveOrRef<T>)
  {
    t_.set_some(move_to_storage(t));
    return t_.val_mut();
  }

  /// If the Option holds a value, returns a mutable reference to it. Otherwise,
  /// stores `t` inside the Option and returns a mutable reference to the new
  /// value.
  ///
  /// If it is non-trivial to construct `T`, the <get_or_insert_with>() method
  /// would be preferable, as it only constructs a `T` if needed.
  constexpr T& get_or_insert(T t) & noexcept sus_lifetimebound
    requires(sus::mem::MoveOrRef<T>)
  {
    if (t_.state() == None) {
      t_.construct_from_none(move_to_storage(t));
    }
    return t_.val_mut();
  }

  /// If the Option holds a value, returns a mutable reference to it. Otherwise,
  /// constructs a default value `T`, stores it inside the Option and returns a
  /// mutable reference to the new value.
  ///
  /// This method differs from <unwrap_or_default>() in that it does not consume
  /// the Option, and instead it can not be called on rvalues.
  ///
  /// This is a shorthand for
  /// `Option<T>::get_or_insert_default(Default<T>::make_default)`.
  ///
  /// The Option's contained type `T` must be #Default, and will be
  /// constructed through that trait.
  constexpr T& get_or_insert_default() & noexcept sus_lifetimebound
    requires(::sus::construct::Default<T>)
  {
    if (t_.state() == None) t_.construct_from_none(T());
    return t_.val_mut();
  }

  /// If the Option holds a value, returns a mutable reference to it. Otherwise,
  /// constructs a `T` by calling `f`, stores it inside the Option and returns a
  /// mutable reference to the new value.
  ///
  /// This method differs from <unwrap_or_else>() in that it does not consume
  /// the Option, and instead it can not be called on rvalues.
  constexpr T& get_or_insert_with(::sus::fn::FnOnce<T()> auto&& f) & noexcept {
    if (t_.state() == None) {
      t_.construct_from_none(
          move_to_storage(::sus::fn::call_once(::sus::move(f))));
    }
    return t_.val_mut();
  }

  /// Returns a new Option containing whatever was inside the current Option.
  ///
  /// If this Option contains `None` then it is left unchanged and returns an
  /// Option containing `None`. If this Option contains `Some` with a value, the
  /// value is moved into the returned Option and this Option will contain
  /// `None` afterward.
  constexpr Option take() & noexcept {
    if (t_.state() == Some)
      return Option(t_.take_and_set_none());
    else
      return Option();
  }

  /// Maps the Option's value through a function.
  ///
  /// When called on an rvalue, it consumes the Option, passing the value
  /// through the map function, and returning an `Option<R>` where `R` is the
  /// return type of the map function.
  ///
  /// Returns an `Option<R>` in state `None` if the current Option is in state
  /// `None`.
  template <::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> MapFn, int&...,
            class R = std::invoke_result_t<MapFn&&, T&&>>
  constexpr Option<R> map(MapFn&& m) && noexcept {
    if (t_.state() == Some) {
      return Option<R>(
          ::sus::fn::call_once(::sus::move(m), t_.take_and_set_none()));
    } else {
      return Option<R>();
    }
  }
  template <::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> MapFn, int&...,
            class R = std::invoke_result_t<MapFn&&, T&&>>
  constexpr Option<R> map(MapFn&& m) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).map(::sus::move(m));
  }

  /// Returns the provided default result (if none), or applies a function to
  /// the contained value (if any).
  ///
  /// Arguments passed to map_or are eagerly evaluated; if you are passing the
  /// result of a function call, it is recommended to use map_or_else, which is
  /// lazily evaluated.
  template <::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> MapFn, int&...,
            class R = std::invoke_result_t<MapFn&&, T&&>>
  constexpr R map_or(R default_result, MapFn&& m) && noexcept {
    if (t_.state() == Some) {
      return ::sus::fn::call_once(::sus::move(m), t_.take_and_set_none());
    } else {
      return default_result;
    }
  }
  template <::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> MapFn, int&...,
            class R = std::invoke_result_t<MapFn&&, T&&>>
  constexpr R map_or(R default_result, MapFn&& m) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).map_or(::sus::move(default_result),
                                      ::sus::move(m));
  }

  /// Computes a default function result (if none), or applies a different
  /// function to the contained value (if any).
  template <::sus::fn::FnOnce<::sus::fn::NonVoid()> DefaultFn,
            ::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> MapFn, int&...,
            class D = std::invoke_result_t<DefaultFn>,
            class R = std::invoke_result_t<MapFn, T&&>>
    requires(std::is_same_v<D, R>)
  constexpr R map_or_else(DefaultFn&& default_fn, MapFn&& m) && noexcept {
    if (t_.state() == Some) {
      return ::sus::fn::call_once(::sus::move(m), t_.take_and_set_none());
    } else {
      return ::sus::move(default_fn)();
    }
  }
  template <::sus::fn::FnOnce<::sus::fn::NonVoid()> DefaultFn,
            ::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> MapFn, int&...,
            class D = std::invoke_result_t<DefaultFn>,
            class R = std::invoke_result_t<MapFn, T&&>>
    requires(std::is_same_v<D, R>)
  constexpr R map_or_else(DefaultFn&& default_fn, MapFn&& m) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).map_or_else(::sus::move(default_fn),
                                           ::sus::move(m));
  }

  /// Consumes the Option and applies a predicate function to the value
  /// contained in the Option. Returns a new Option with the same value if the
  /// predicate returns true, otherwise returns an Option with its state set to
  /// `None`.
  ///
  /// The predicate function must take `const T&` and return `bool`.
  constexpr Option<T> filter(
      ::sus::fn::FnOnce<bool(const T&)> auto&& p) && noexcept {
    if (t_.state() == Some) {
      if (::sus::fn::call_once(::sus::move(p), t_.val())) {
        return Option(t_.take_and_set_none());
      } else {
        // The state has to become None, and we must destroy the inner T.
        t_.set_none();
        return Option();
      }
    } else {
      return Option();
    }
  }
  constexpr Option<T> filter(
      ::sus::fn::FnOnce<bool(const T&)> auto&& p) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).filter(::sus::move(p));
  }

  /// Consumes this Option and returns an Option with `None` if this Option
  /// holds `None`, otherwise returns the given `opt`.
  template <class U>
  constexpr Option<U> and_that(Option<U> opt) && noexcept {
    if (t_.state() == Some) {
      t_.set_none();
    } else {
      opt = Option<U>();
    }
    return opt;
  }
  template <class U>
  constexpr Option<U> and_that(Option<U> opt) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).and_that(::sus::move(opt));
  }

  /// Consumes this Option and returns an Option with `None` if this Option
  /// holds `None`, otherwise calls `f` with the contained value and returns the
  /// result.
  ///
  /// The function `f` receives the Option's inner `T` and can return any
  /// `Option<U>`.
  ///
  /// Some languages call this operation flatmap.
  template <::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> AndFn, int&...,
            class R = std::invoke_result_t<AndFn, T&&>,
            class U = ::sus::option::__private::IsOptionType<R>::inner_type>
    requires(::sus::option::__private::IsOptionType<R>::value)
  constexpr Option<U> and_then(AndFn&& f) && noexcept {
    if (t_.state() == Some)
      return ::sus::fn::call_once(::sus::move(f), t_.take_and_set_none());
    else
      return Option<U>();
  }
  template <::sus::fn::FnOnce<::sus::fn::NonVoid(T&&)> AndFn, int&...,
            class R = std::invoke_result_t<AndFn, T&&>,
            class U = ::sus::option::__private::IsOptionType<R>::inner_type>
    requires(::sus::option::__private::IsOptionType<R>::value)
  constexpr Option<U> and_then(AndFn&& f) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).and_then(::sus::move(f));
  }

  /// Consumes and returns an Option with the same value if this Option contains
  /// a value, otherwise returns the given `opt`.
  constexpr Option<T> or_that(Option<T> opt) && noexcept {
    if (t_.state() == Some)
      return Option(t_.take_and_set_none());
    else
      return opt;
  }
  constexpr Option<T> or_that(Option<T> opt) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).or_that(::sus::move(opt));
  }

  /// Consumes and returns an Option with the same value if this Option contains
  /// a value, otherwise returns the Option returned by `f`.
  constexpr Option<T> or_else(
      ::sus::fn::FnOnce<Option<T>()> auto&& f) && noexcept {
    if (t_.state() == Some)
      return Option(t_.take_and_set_none());
    else
      return ::sus::fn::call_once(::sus::move(f));
  }
  constexpr Option<T> or_else(
      ::sus::fn::FnOnce<Option<T>()> auto&& f) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).or_else(::sus::move(f));
  }

  /// Consumes this Option and returns an Option, holding the value from either
  /// this Option `opt`, if exactly one of them holds a value, otherwise returns
  /// an Option that holds `None`.
  constexpr Option<T> xor_that(Option<T> opt) && noexcept {
    if (t_.state() == Some) {
      // If `this` holds Some, we change `this` to hold None. If `opt` is None,
      // we return what this was holding, otherwise we return None.
      if (opt.t_.state() == None) {
        return Option(t_.take_and_set_none());
      } else {
        t_.set_none();
        return Option();
      }
    } else {
      // If `this` holds None, we need to do nothing to `this`. If `opt` is Some
      // we would return its value, and if `opt` is None we should return None.
      return opt;
    }
  }
  constexpr Option<T> xor_that(Option<T> opt) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).xor_that(::sus::move(opt));
  }

  /// Transforms the `Option<T>` into a `Result<T, E>`, mapping `Some(v)` to
  /// `Ok(v)` and `None` to `Err(e)`.
  ///
  /// Arguments passed to `ok_or` are eagerly evaluated; if you are passing the
  /// result of a function call, it is recommended to use `ok_or_else`, which is
  /// lazily evaluated.
  //
  // TODO: No refs in Result: https://github.com/chromium/subspace/issues/133
  template <class E, int&..., class Result = ::sus::result::Result<T, E>>
  constexpr Result ok_or(E e) && noexcept
    requires(!std::is_reference_v<T> && !std::is_reference_v<E>)
  {
    if (t_.state() == Some)
      return Result::with(t_.take_and_set_none());
    else
      return Result::with_err(::sus::move(e));
  }
  template <class E, int&..., class Result = ::sus::result::Result<T, E>>
  constexpr Result ok_or(E e) const& noexcept
    requires(!std::is_reference_v<T> && !std::is_reference_v<E> &&
             ::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).ok_or(::sus::move(e));
  }

  /// Transforms the `Option<T>` into a `Result<T, E>`, mapping `Some(v)` to
  /// `Ok(v)` and `None` to `Err(f())`.
  //
  // TODO: No refs in Result: https://github.com/chromium/subspace/issues/133
  template <::sus::fn::FnOnce<::sus::fn::NonVoid()> ElseFn, int&...,
            class E = std::invoke_result_t<ElseFn>,
            class Result = ::sus::result::Result<T, E>>
  constexpr Result ok_or_else(ElseFn&& f) && noexcept
    requires(!std::is_reference_v<T> && !std::is_reference_v<E>)
  {
    if (t_.state() == Some)
      return Result::with(t_.take_and_set_none());
    else
      return Result::with_err(::sus::fn::call_once(::sus::move(f)));
  }
  template <::sus::fn::FnOnce<::sus::fn::NonVoid()> ElseFn, int&...,
            class E = std::invoke_result_t<ElseFn>,
            class Result = ::sus::result::Result<T, E>>
  constexpr Result ok_or_else(ElseFn&& f) const& noexcept
    requires(!std::is_reference_v<T> && !std::is_reference_v<E> &&
             ::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).ok_or_else(::sus::move(f));
  }

  /// Transposes an #Option of a #Result into a #Result of an #Option.
  ///
  /// `None` will be mapped to `Ok(None)`. `Some(Ok(_))` and `Some(Err(_))` will
  /// be mapped to `Ok(Some(_))` and `Err(_)`.
  template <int&...,
            class OkType =
                typename ::sus::result::__private::IsResultType<T>::ok_type,
            class ErrType =
                typename ::sus::result::__private::IsResultType<T>::err_type,
            class Result = ::sus::result::Result<Option<OkType>, ErrType>>
    requires(::sus::result::__private::IsResultType<T>::value)
  constexpr Result transpose() && noexcept {
    if (t_.state() == None) {
      return Result::with(Option<OkType>());
    } else {
      if (t_.val().is_ok()) {
        return Result::with(Option<OkType>::with(
            t_.take_and_set_none().unwrap_unchecked(::sus::marker::unsafe_fn)));
      } else {
        return Result::with_err(t_.take_and_set_none().unwrap_err_unchecked(
            ::sus::marker::unsafe_fn));
      }
    }
  }
  template <int&...,
            class OkType =
                typename ::sus::result::__private::IsResultType<T>::ok_type,
            class ErrType =
                typename ::sus::result::__private::IsResultType<T>::err_type,
            class Result = ::sus::result::Result<Option<OkType>, ErrType>>
    requires(::sus::result::__private::IsResultType<T>::value)
  constexpr Result transpose() const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).transpose();
  }

  /// Zips self with another Option.
  ///
  /// If self is `Some(s)` and other is `Some(o)`, this method returns
  /// `Some(Tuple(s, o))`. Otherwise, `None` is returned.
  template <class U, int&..., class Tuple = ::sus::tuple_type::Tuple<T, U>>
  constexpr Option<Tuple> zip(Option<U> o) && noexcept {
    if (o.t_.state() == None) {
      if (t_.state() == Some) t_.set_none();
      return Option<Tuple>();
    } else if (t_.state() == None) {
      return Option<Tuple>();
    } else {
      // SAFETY: We have verified `*this` and `o` contain Some above.
      return Option<Tuple>::with(Tuple::with(
          ::sus::move(*this).unwrap_unchecked(::sus::marker::unsafe_fn),
          ::sus::move(o).unwrap_unchecked(::sus::marker::unsafe_fn)));
    }
  }
  template <class U, int&..., class Tuple = ::sus::tuple_type::Tuple<T, U>>
  constexpr Option<Tuple> zip(Option<U> o) const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).zip(::sus::move(o));
  }

  /// Unzips an Option holding a Tuple of two values into a Tuple of two
  /// Options.
  ///
  /// `Option<Tuple<i32, u32>>` is unzipped to `Tuple<Option<i32>,
  /// Option<u32>>`.
  ///
  /// If self is Some, the result is a Tuple with both Options holding the
  /// values from self. Otherwise, the result is a Tuple of two Options set to
  /// None.
  constexpr auto unzip() && noexcept
    requires(!std::is_reference_v<T> && __private::IsTupleOfSizeTwo<T>::value)
  {
    using U = __private::IsTupleOfSizeTwo<T>::first_type;
    using V = __private::IsTupleOfSizeTwo<T>::second_type;
    if (is_some()) {
      auto&& [u, v] = t_.take_and_set_none();
      return ::sus::tuple_type::Tuple<Option<U>, Option<V>>::with(
          Option<U>::with(::sus::forward<U>(u)),
          Option<V>::with(::sus::forward<V>(v)));
    } else {
      return ::sus::tuple_type::Tuple<Option<U>, Option<V>>::with(Option<U>(),
                                                                  Option<V>());
    }
  }
  constexpr auto unzip() const& noexcept
    requires(!std::is_reference_v<T> && __private::IsTupleOfSizeTwo<T>::value &&
             ::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).unzip();
  }

  /// Replaces whatever the Option is currently holding with `Some` value `t`
  /// and returns an Option holding what was there previously.
  constexpr Option replace(T t) & noexcept
    requires(sus::mem::MoveOrRef<T>)
  {
    if (t_.state() == None) {
      t_.construct_from_none(move_to_storage(t));
      return Option();
    } else {
      return Option(t_.replace_some(move_to_storage(t)));
    }
  }

  /// Maps an `Option<T&>` to an `Option<T>` by copying the referenced `T`.
  constexpr Option<std::remove_const_t<std::remove_reference_t<T>>> copied()
      const& noexcept
    requires(std::is_reference_v<T> && ::sus::mem::Copy<T>)
  {
    if (t_.state() == None) {
      return Option<std::remove_const_t<std::remove_reference_t<T>>>();
    } else {
      return Option<std::remove_const_t<std::remove_reference_t<T>>>::with(
          t_.val());
    }
  }

  /// Maps an `Option<T&>` to an `Option<T>` by cloning the referenced `T`.
  constexpr Option<std::remove_const_t<std::remove_reference_t<T>>> cloned()
      const& noexcept
    requires(std::is_reference_v<T> && ::sus::mem::Clone<T>)
  {
    if (t_.state() == None) {
      return Option<std::remove_const_t<std::remove_reference_t<T>>>();
    } else {
      // Specify the type `T` for clone() as `t_.val()` may be a
      // `StoragePointer<T>` when the Option is holding a reference, and we want
      // to clone the `T` object, not the `StoragePointer<T>`. The latter
      // converts to a `const T&`.
      return Option<std::remove_const_t<std::remove_reference_t<T>>>::with(
          ::sus::clone<std::remove_reference_t<T>>(t_.val()));
    }
  }

  /// Maps an `Option<Option<T>>` to an `Option<T>`.
  constexpr T flatten() && noexcept
    requires(::sus::option::__private::IsOptionType<T>::value)
  {
    if (t_.state() == Some)
      return ::sus::move(*this).unwrap_unchecked(::sus::marker::unsafe_fn);
    else
      return T();
  }
  constexpr T flatten() const& noexcept
    requires(::sus::option::__private::IsOptionType<T>::value &&
             ::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).flatten();
  }

  /// Returns an Option<const T&> from this Option<T>, that either holds `None`
  /// or a reference to the value in this Option.
  ///
  /// # Implementation Notes
  ///
  /// Implementation note: We only allow calling this on an rvalue Option if the
  /// contained value is a reference, otherwise we are returning a reference to
  /// a short-lived object which leads to common C++ memory bugs.
  sus_pure constexpr Option<const std::remove_reference_t<T>&> as_ref()
      const& noexcept {
    if (t_.state() == None)
      return Option<const std::remove_reference_t<T>&>();
    else
      return Option<const std::remove_reference_t<T>&>(t_.val());
  }
  sus_pure constexpr Option<const std::remove_reference_t<T>&>
  as_ref() && noexcept
    requires(std::is_reference_v<T>)
  {
    if (t_.state() == None)
      return Option<const std::remove_reference_t<T>&>();
    else
      return Option<const std::remove_reference_t<T>&>(t_.take_and_set_none());
  }

  /// Returns an Option<T&> from this Option<T>, that either holds `None` or a
  /// reference to the value in this Option.
  sus_pure constexpr Option<T&> as_mut() & noexcept {
    if (t_.state() == None)
      return Option<T&>();
    else
      return Option<T&>(t_.val_mut());
  }
  // Calling as_mut() on an rvalue is not returning a reference to the inner
  // value if the inner value is already a reference, so we allow calling it on
  // an rvalue Option in that case.
  sus_pure constexpr Option<T&> as_mut() && noexcept
    requires(std::is_reference_v<T>)
  {
    if (t_.state() == None)
      return Option<T&>();
    else
      return Option<T&>(t_.take_and_set_none());
  }
  sus_pure constexpr Option<T&> as_mut() const& noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::clone(*this).as_mut();
  }

  sus_pure constexpr Once<const std::remove_reference_t<T>&> iter()
      const& noexcept {
    return ::sus::iter::once<const std::remove_reference_t<T>&>(as_ref());
  }
  constexpr Once<const std::remove_reference_t<T>&> iter() && noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::iter::once<const std::remove_reference_t<T>&>(
        ::sus::move(*this).as_ref());
  }
  constexpr Once<const std::remove_reference_t<T>&> iter() const& noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::clone(*this).iter();
  }

  sus_pure constexpr Once<T&> iter_mut() & noexcept {
    return ::sus::iter::once<T&>(as_mut());
  }
  constexpr Once<T&> iter_mut() && noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::iter::once<T&>(::sus::move(*this).as_mut());
  }
  constexpr Once<T&> iter_mut() const& noexcept
    requires(std::is_reference_v<T>)
  {
    return ::sus::clone(*this).iter_mut();
  }

  constexpr Once<T> into_iter() && noexcept {
    return ::sus::iter::once<T>(take());
  }
  constexpr Once<T> into_iter() const& noexcept
    requires(::sus::mem::CopyOrRef<T>)
  {
    return ::sus::clone(*this).into_iter();
  }

  /// sus::ops::Eq<Option<U>> trait.
  ///
  /// The non-template overload allows some/none marker types to convert to
  /// Option for comparison.
  friend constexpr inline bool operator==(const Option& l,
                                          const Option& r) noexcept
    requires(::sus::ops::Eq<T>)
  {
    switch (l) {
      case Some:
        return r.is_some() && (l.as_value_unchecked(::sus::marker::unsafe_fn) ==
                               r.as_value_unchecked(::sus::marker::unsafe_fn));
      case None: return r.is_none();
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }
  template <class U>
    requires(::sus::ops::Eq<T, U>)
  friend constexpr inline bool operator==(const Option<T>& l,
                                          const Option<U>& r) noexcept {
    switch (l) {
      case Some:
        return r.is_some() && (l.as_value_unchecked(::sus::marker::unsafe_fn) ==
                               r.as_value_unchecked(::sus::marker::unsafe_fn));
      case None: return r.is_none();
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }

  template <class U>
    requires(!::sus::ops::Eq<T, U>)
  friend constexpr inline bool operator==(const Option<T>& l,
                                          const Option<U>& r) = delete;

  /// Compares two Options.
  ///
  /// Satisfies sus::ops::Ord<Option<U>> if sus::ops::Ord<U>.
  ///
  /// Satisfies sus::ops::WeakOrd<Option<U>> if sus::ops::WeakOrd<U>.
  ///
  /// Satisfies sus::ops::PartialOrd<Option<U>> if sus::ops::PartialOrd<U>.
  ///
  /// The non-template overloads allow some/none marker types to convert to
  /// Option for comparison.
  //
  // sus::ops::Ord<Option<U>> trait.
  friend constexpr inline auto operator<=>(const Option& l,
                                           const Option& r) noexcept
    requires(::sus::ops::ExclusiveOrd<T>)
  {
    switch (l) {
      case Some:
        if (r.is_some()) {
          return l.as_value_unchecked(::sus::marker::unsafe_fn) <=>
                 r.as_value_unchecked(::sus::marker::unsafe_fn);
        } else {
          return std::strong_ordering::greater;
        }
      case None:
        if (r.is_some())
          return std::strong_ordering::less;
        else
          return std::strong_ordering::equivalent;
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }
  template <class U>
    requires(::sus::ops::ExclusiveOrd<T, U>)
  friend constexpr inline auto operator<=>(const Option<T>& l,
                                           const Option<U>& r) noexcept {
    switch (l) {
      case Some:
        if (r.is_some()) {
          return l.as_value_unchecked(::sus::marker::unsafe_fn) <=>
                 r.as_value_unchecked(::sus::marker::unsafe_fn);
        } else {
          return std::strong_ordering::greater;
        }
      case None:
        if (r.is_some())
          return std::strong_ordering::less;
        else
          return std::strong_ordering::equivalent;
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }

  // sus::ops::WeakOrd<Option<U>> trait.
  friend constexpr inline auto operator<=>(const Option& l,
                                           const Option& r) noexcept
    requires(::sus::ops::ExclusiveWeakOrd<T>)
  {
    switch (l) {
      case Some:
        if (r.is_some()) {
          return l.as_value_unchecked(::sus::marker::unsafe_fn) <=>
                 r.as_value_unchecked(::sus::marker::unsafe_fn);
        } else {
          return std::weak_ordering::greater;
        }
      case None:
        if (r.is_some())
          return std::weak_ordering::less;
        else
          return std::weak_ordering::equivalent;
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }
  template <class U>
    requires(::sus::ops::ExclusiveWeakOrd<T, U>)
  friend constexpr inline auto operator<=>(const Option<T>& l,
                                           const Option<U>& r) noexcept {
    switch (l) {
      case Some:
        if (r.is_some()) {
          return l.as_value_unchecked(::sus::marker::unsafe_fn) <=>
                 r.as_value_unchecked(::sus::marker::unsafe_fn);
        } else {
          return std::weak_ordering::greater;
        }
      case None:
        if (r.is_some())
          return std::weak_ordering::less;
        else
          return std::weak_ordering::equivalent;
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }

  // sus::ops::PartialOrd<Option<U>> trait.
  template <class U>
    requires(::sus::ops::ExclusivePartialOrd<T, U>)
  friend constexpr inline auto operator<=>(const Option<T>& l,
                                           const Option<U>& r) noexcept {
    switch (l) {
      case Some:
        if (r.is_some()) {
          return l.as_value_unchecked(::sus::marker::unsafe_fn) <=>
                 r.as_value_unchecked(::sus::marker::unsafe_fn);
        } else {
          return std::partial_ordering::greater;
        }
      case None:
        if (r.is_some())
          return std::partial_ordering::less;
        else
          return std::partial_ordering::equivalent;
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }
  friend constexpr inline auto operator<=>(const Option& l,
                                           const Option& r) noexcept
    requires(::sus::ops::ExclusivePartialOrd<T>)
  {
    switch (l) {
      case Some:
        if (r.is_some()) {
          return l.as_value_unchecked(::sus::marker::unsafe_fn) <=>
                 r.as_value_unchecked(::sus::marker::unsafe_fn);
        } else {
          return std::partial_ordering::greater;
        }
      case None:
        if (r.is_some())
          return std::partial_ordering::less;
        else
          return std::partial_ordering::equivalent;
    }
    ::sus::unreachable_unchecked(::sus::marker::unsafe_fn);
  }

  template <class U>
    requires(!::sus::ops::PartialOrd<T, U>)
  friend constexpr inline auto operator<=>(
      const Option<T>& l, const Option<U>& r) noexcept = delete;

  /// Implements sus::mem::From<std::optional>.
  ///
  /// This also allows `sus::into()` to convert with type deduction from
  /// `std::optional` to `sus::Option`.
  ///
  /// Include subspace/option/option_compat.h to use these methods, as std
  /// `<optional>` header is not included by default.
  ///
  /// #[doc.overloads=from.optional]
  constexpr static Option from(
      const std::optional<std::remove_reference_t<T>>& s) noexcept
    requires(::sus::mem::Copy<T> && !std::is_reference_v<T>);
  /// #[doc.overloads=from.optional]
  constexpr static Option from(
      std::optional<std::remove_reference_t<T>>&& s) noexcept
    requires(::sus::mem::Move<T> && !std::is_reference_v<T>);
  /// Implements sus::construct::From<std::optional<U>> when `U` can be
  /// converted to `T`, i.e. `sus::construct::Into<U, T>`
  ///
  /// #[doc.overloads=from.optional.u]
  template <::sus::construct::Into<T> U>
  static inline constexpr Option from(const std::optional<U>& s) noexcept
    requires(!std::is_reference_v<T>);
  /// #[doc.overloads=from.optional.u]
  template <::sus::construct::Into<T> U>
  static inline constexpr Option from(std::optional<U>&& s) noexcept
    requires(!std::is_reference_v<T>);
  /// Implicit conversion from std::optional.
  ///
  /// Include subspace/option/option_compat.h to use these methods, as std
  /// `<optional>` header is not included by default.
  ///
  /// #[doc.overloads=ctor.optional]
  constexpr Option(const std::optional<std::remove_reference_t<T>>& s) noexcept
    requires(::sus::mem::Copy<T> && !std::is_reference_v<T>);
  /// #[doc.overloads=ctor.optional]
  constexpr Option(std::optional<std::remove_reference_t<T>>&& s) noexcept
    requires(::sus::mem::Move<T> && !std::is_reference_v<T>);
  /// Implicit conversion to std::optional.
  ///
  /// Include subspace/option/option_compat.h to use these methods, as std
  /// `<optional>` header is not included by default.
  ///
  /// #[doc.overloads=convert.optional]
  constexpr operator std::optional<std::remove_reference_t<T>>() const& noexcept
    requires(::sus::mem::Copy<T> && !std::is_reference_v<T>);
  /// #[doc.overloads=convert.optional]
  constexpr operator std::optional<std::remove_reference_t<T>>() && noexcept
    requires(::sus::mem::Move<T> && !std::is_reference_v<T>);
  /// #[doc.overloads=convert.optional]
  constexpr operator std::optional<const std::remove_reference_t<T>*>()
      const& noexcept
    requires(std::is_reference_v<T>);
  // Skip implementing `& noexcept` `&& noexcept` and `const& noexcept` and just
  // implement `const& noexcept` which covers them all.
  /// #[doc.overloads=convert.optional]
  constexpr operator std::optional<std::remove_reference_t<T>*>()
      const& noexcept
    requires(!std::is_const_v<std::remove_reference_t<T>> &&
             std::is_reference_v<T>);

 private:
  template <class U>
  friend class Option;

  // Since `T` may be a reference or a value type, this constructs the correct
  // storage from a `T` object or a `T&&` (which is received as `T&`).
  template <class U>
  static constexpr inline decltype(auto) copy_to_storage(const U& t) {
    if constexpr (std::is_reference_v<T>)
      return StoragePointer<T>(t);
    else
      return t;
  }
  // Since `T` may be a reference or a value type, this constructs the correct
  // storage from a `T` object or a `T&&` (which is received as `T&`).
  template <class U>
  static constexpr inline decltype(auto) move_to_storage(U&& t) {
    if constexpr (std::is_reference_v<T>)
      return StoragePointer<T>(t);
    else
      return ::sus::move(t);
  }

  /// Constructors for `Some`.
  constexpr explicit Option(T t)
    requires(std::is_reference_v<T>)
      : t_(StoragePointer<T>(t)) {}
  constexpr explicit Option(const T& t)
    requires(!std::is_reference_v<T>)
      : t_(t) {}
  constexpr explicit Option(T&& t)
    requires(!std::is_reference_v<T> && ::sus::mem::MoveOrRef<T>)
      : t_(::sus::move(t)) {}

  template <class U>
  using StorageType =
      std::conditional_t<std::is_reference_v<U>, Storage<StoragePointer<U>>,
                         Storage<U>>;
  StorageType<T> t_;

  sus_class_trivially_relocatable_if_types(::sus::marker::unsafe_fn,
                                           StorageType<T>);
};

// Implicit for-ranged loop iteration via `Option::iter()`.
using ::sus::iter::__private::begin;
using ::sus::iter::__private::end;

/// Used to construct an Option<T> with a Some(t) value.
///
/// Calling some() produces a hint to make an Option<T> but does not actually
/// construct Option<T>. This is to allow constructing an Option<T> or
/// Option<T&> correctly.
template <class T>
[[nodiscard]] inline constexpr __private::SomeMarker<T&&> some(
    T&& t sus_lifetimebound) noexcept {
  return __private::SomeMarker<T&&>(::sus::forward<T>(t));
}

/// Used to construct an Option<T> with a None value.
///
/// Calling none() produces a hint to make an Option<T> but does not actually
/// construct Option<T>. This is because the type `T` is not known until the
/// construction is explicitly requested.
sus_pure_const inline constexpr auto none() noexcept {
  return __private::NoneMarker();
}

}  // namespace sus::option

// Implements sus::ops::Try for Option.
template <class T>
struct sus::ops::TryImpl<::sus::option::Option<T>> {
  using Output = T;
  constexpr static bool is_success(const ::sus::option::Option<T>& t) {
    return t.is_some();
  }
  constexpr static Output to_output(::sus::option::Option<T> t) {
    return ::sus::move(t).unwrap();
  }
  constexpr static ::sus::option::Option<T> from_output(T t) {
    return ::sus::option::Option<T>::with(::sus::move(t));
  }
};

// std hash support.
template <class T>
struct std::hash<::sus::option::Option<T>> {
  auto operator()(const ::sus::option::Option<T>& u) const noexcept {
    if (u.is_some())
      return std::hash<T>()(*u);
    else
      return 0;
  }
};
template <class T>
  requires(::sus::ops::Eq<T>)
struct std::equal_to<::sus::option::Option<T>> {
  constexpr auto operator()(const ::sus::option::Option<T>& l,
                            const ::sus::option::Option<T>& r) const noexcept {
    return l == r;
  }
};

// fmt support.
template <class T, class Char>
struct fmt::formatter<::sus::option::Option<T>, Char> {
  template <class ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return underlying_.parse(ctx);
  }

  template <class FormatContext>
  constexpr auto format(const ::sus::option::Option<T>& t,
                        FormatContext& ctx) const {
    if (t.is_none()) {
      return fmt::format_to(ctx.out(), "None");
    } else {
      auto out = ctx.out();
      out = fmt::format_to(out, "Some(");
      ctx.advance_to(out);
      out = underlying_.format(t.as_value(), ctx);
      return fmt::format_to(out, ")");
    }
  }

 private:
  ::sus::string::__private::AnyFormatter<T, Char> underlying_;
};

// Stream support.
sus__format_to_stream(sus::option, Option, T);

// Promote Option and its enum values into the `sus` namespace.
namespace sus {
using ::sus::option::none;
using ::sus::option::None;
using ::sus::option::Option;
using ::sus::option::some;
using ::sus::option::Some;
}  // namespace sus

//////

// This header contains a type that needs to use Option, and is used by an
// Option method implementation below. So we have to include it after defining
// Option but before implementing the following methods.
#include "subspace/iter/size_hint.h"

namespace sus::option {

namespace __private {

// This is a separate function instead of an out-of-line definition to work
// around bug https://github.com/llvm/llvm-project/issues/63769 in Clang 16.
template <class T, ::sus::iter::Iterator<Option<T>> Iter>
  requires ::sus::iter::Product<T>
constexpr Option<T> from_product_impl(Iter&& it) noexcept {
  class IterUntilNone final
      : public ::sus::iter::IteratorBase<IterUntilNone, T> {
   public:
    constexpr IterUntilNone(Iter& iter, bool& found_none)
        : iter_(iter), found_none_(found_none) {}

    constexpr Option<T> next() noexcept {
      Option<Option<T>> next = iter_.next();
      Option<T> out;
      if (next.is_some()) {
        out = ::sus::move(next).unwrap_unchecked(::sus::marker::unsafe_fn);
        if (out.is_none()) found_none_ = true;
      }
      return out;
    }
    constexpr ::sus::iter::SizeHint size_hint() const noexcept {
      return ::sus::iter::SizeHint(0u, iter_.size_hint().upper);
    }

   private:
    Iter& iter_;
    bool& found_none_;
  };
  static_assert(::sus::iter::Iterator<IterUntilNone, T>);

  bool found_none = false;
  auto out = Option<T>::with(IterUntilNone(it, found_none).product());
  if (found_none) out = Option<T>();
  return out;
}

// This is a separate function instead of an out-of-line definition to work
// around bug https://github.com/llvm/llvm-project/issues/63769 in Clang 16.
template <class T, ::sus::iter::Iterator<Option<T>> Iter>
  requires ::sus::iter::Sum<T>
constexpr Option<T> from_sum_impl(Iter&& it) noexcept {
  class IterUntilNone final
      : public ::sus::iter::IteratorBase<IterUntilNone, T> {
   public:
    constexpr IterUntilNone(Iter& iter, bool& found_none)
        : iter_(iter), found_none_(found_none) {}

    constexpr Option<T> next() noexcept {
      Option<Option<T>> next = iter_.next();
      Option<T> out;
      if (next.is_some()) {
        out = ::sus::move(next).unwrap_unchecked(::sus::marker::unsafe_fn);
        if (out.is_none()) found_none_ = true;
      }
      return out;
    }
    constexpr ::sus::iter::SizeHint size_hint() const noexcept {
      return ::sus::iter::SizeHint(0u, iter_.size_hint().upper);
    }

   private:
    Iter& iter_;
    bool& found_none_;
  };
  static_assert(::sus::iter::Iterator<IterUntilNone, T>);

  bool found_none = false;
  auto out = Option<T>::with(IterUntilNone(it, found_none).sum());
  if (found_none) out = Option<T>();
  return out;
}

}  // namespace __private

}  // namespace sus::option

// sus::iter::FromIterator trait for Option.
template <class T>
struct sus::iter::FromIteratorImpl<::sus::option::Option<T>> {
  // TODO: Subdoc doesn't split apart template instantiations so comments
  // collide. This should be able to appear in the docs.
  //
  // Takes each item in the Iterator: if it is None, no further elements are
  // taken, and the None is returned. Should no None occur, a container of type
  // T containing the values of type U from each Option<U> is returned.
  template <class IntoIter, int&...,
            class Iter =
                std::decay_t<decltype(std::declval<IntoIter&&>().into_iter())>,
            class O = typename Iter::Item,
            class U = ::sus::option::__private::IsOptionType<O>::inner_type>
    requires(::sus::option::__private::IsOptionType<O>::value &&
             ::sus::iter::IntoIterator<IntoIter, ::sus::option::Option<U>>)
  static constexpr ::sus::option::Option<T> from_iter(
      IntoIter&& option_iter) noexcept
    requires(!std::is_reference_v<T> && ::sus::iter::FromIterator<T, U>)
  {
    // An iterator over `option_iter`'s iterator that returns each element in it
    // until it reaches a `None` or the end.
    struct UntilNoneIter final
        : public ::sus::iter::IteratorBase<UntilNoneIter, U> {
      UntilNoneIter(Iter&& iter, bool& found_none)
          : iter(iter), found_none(found_none) {}

      ::sus::option::Option<U> next() noexcept {
        ::sus::option::Option<::sus::option::Option<U>> item = iter.next();
        if (found_none || item.is_none()) return ::sus::option::Option<U>();
        found_none = item->is_none();
        return ::sus::move(item).flatten();
      }
      ::sus::iter::SizeHint size_hint() const noexcept {
        return ::sus::iter::SizeHint(0u, iter.size_hint().upper);
      }

      Iter& iter;
      bool& found_none;
    };

    bool found_none = false;
    auto iter = UntilNoneIter(::sus::move(option_iter).into_iter(),
                              found_none);
    auto collected = sus::iter::from_iter<T>(::sus::move(iter));
    if (found_none)
      return ::sus::option::Option<T>();
    else
      return ::sus::option::Option<T>::with(::sus::move(collected));
  }
};
