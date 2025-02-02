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

#include <type_traits>

#include "googletest/include/gtest/gtest.h"
#include "sus/collections/array.h"
#include "sus/construct/into.h"
#include "sus/iter/__private/step.h"
#include "sus/num/num_concepts.h"
#include "sus/num/signed_integer.h"
#include "sus/num/unsigned_integer.h"
#include "sus/cmp/eq.h"
#include "sus/cmp/ord.h"
#include "sus/prelude.h"
#include "sus/tuple/tuple.h"

namespace {

using sus::None;
using sus::Option;
using sus::Tuple;

static_assert(std::is_signed_v<decltype(i8::primitive_value)>);
static_assert(sizeof(decltype(i8::primitive_value)) == 1);
static_assert(sizeof(i8) == sizeof(decltype(i8::primitive_value)));

static_assert(sus::mem::Copy<i8>);
static_assert(sus::mem::TrivialCopy<i8>);
static_assert(sus::mem::Clone<i8>);
static_assert(sus::mem::relocate_by_memcpy<i8>);
static_assert(sus::mem::Move<i8>);

namespace behaviour {
using T = i8;
using From = decltype(i8::primitive_value);
static_assert(!std::is_trivial_v<T>, "");
static_assert(!std::is_aggregate_v<T>, "");
static_assert(std::is_standard_layout_v<T>, "");
static_assert(!std::is_trivially_default_constructible_v<T>, "");
static_assert(std::is_trivially_copy_constructible_v<T>, "");
static_assert(std::is_trivially_copy_assignable_v<T>, "");
static_assert(std::is_trivially_move_constructible_v<T>, "");
static_assert(std::is_trivially_move_assignable_v<T>, "");
static_assert(std::is_trivially_destructible_v<T>, "");
static_assert(std::is_copy_constructible_v<T>, "");
static_assert(std::is_copy_assignable_v<T>, "");
static_assert(std::is_move_constructible_v<T>, "");
static_assert(std::is_move_assignable_v<T>, "");
static_assert(std::is_nothrow_swappable_v<T>, "");
static_assert(std::is_constructible_v<T, From&&>, "");
static_assert(std::is_assignable_v<T, From&&>, "");
static_assert(std::is_constructible_v<T, const From&>, "");
static_assert(std::is_assignable_v<T, const From&>, "");
static_assert(std::is_constructible_v<T, From>, "");
static_assert(!std::is_trivially_constructible_v<T, From>, "");
static_assert(std::is_assignable_v<T, From>, "");
static_assert(std::is_nothrow_destructible_v<T>, "");
static_assert(sus::construct::Default<T>, "");
static_assert(sus::mem::relocate_by_memcpy<T>, "");
}  // namespace behaviour

// i8::MAX
static_assert(i8::MAX.primitive_value == 0x7f);
template <class T>
concept MaxInRange = requires {
  { 0x7f_i8 } -> std::same_as<T>;
  { i8(int8_t{0x7f}) } -> std::same_as<T>;
};
static_assert(MaxInRange<i8>);

// std hashing
static_assert(std::same_as<decltype(std::hash<i8>()(0_i8)), size_t>);
static_assert(std::same_as<decltype(std::equal_to<i8>()(0_i8, 1_i8)), bool>);

TEST(i8, Traits) {
  static_assert(sus::iter::__private::Step<i8>);

  // ** Signed only **
  static_assert(sus::num::Neg<i8>);

  static_assert(sus::num::Add<i8, i8>);
  static_assert(sus::num::AddAssign<i8, i8>);
  static_assert(sus::num::Sub<i8, i8>);
  static_assert(sus::num::SubAssign<i8, i8>);
  static_assert(sus::num::Mul<i8, i8>);
  static_assert(sus::num::MulAssign<i8, i8>);
  static_assert(sus::num::Div<i8, i8>);
  static_assert(sus::num::DivAssign<i8, i8>);
  static_assert(sus::num::Rem<i8, i8>);
  static_assert(sus::num::RemAssign<i8, i8>);
  static_assert(sus::num::BitAnd<i8, i8>);
  static_assert(sus::num::BitAndAssign<i8, i8>);
  static_assert(sus::num::BitOr<i8, i8>);
  static_assert(sus::num::BitOrAssign<i8, i8>);
  static_assert(sus::num::BitXor<i8, i8>);
  static_assert(sus::num::BitXorAssign<i8, i8>);
  static_assert(sus::num::BitNot<i8>);
  static_assert(sus::num::Shl<i8>);
  static_assert(sus::num::ShlAssign<i8>);
  static_assert(sus::num::Shr<i8>);
  static_assert(sus::num::ShrAssign<i8>);

  static_assert(sus::cmp::StrongOrd<i8, int8_t>);
  static_assert(sus::cmp::StrongOrd<i8, int16_t>);
  static_assert(sus::cmp::StrongOrd<i8, int32_t>);
  static_assert(sus::cmp::StrongOrd<i8, int64_t>);
  static_assert(sus::cmp::StrongOrd<i8, i8>);
  static_assert(sus::cmp::StrongOrd<i8, i16>);
  static_assert(sus::cmp::StrongOrd<i8, i32>);
  static_assert(sus::cmp::StrongOrd<i8, i64>);
  static_assert(sus::cmp::StrongOrd<i8, isize>);
  static_assert(1_i8 >= 1_i8);
  static_assert(2_i8 > 1_i8);
  static_assert(1_i8 <= 1_i8);
  static_assert(1_i8 < 2_i8);
  static_assert(sus::cmp::Eq<i8, int8_t>);
  static_assert(sus::cmp::Eq<i8, int16_t>);
  static_assert(sus::cmp::Eq<i8, int32_t>);
  static_assert(sus::cmp::Eq<i8, int64_t>);
  static_assert(sus::cmp::Eq<i8, i8>);
  static_assert(sus::cmp::Eq<i8, i16>);
  static_assert(sus::cmp::Eq<i8, i32>);
  static_assert(sus::cmp::Eq<i8, i64>);
  static_assert(sus::cmp::Eq<i8, isize>);
  static_assert(1_i8 == 1_i8);
  static_assert(!(1_i8 == 2_i8));
  static_assert(1_i8 != 2_i8);
  static_assert(!(1_i8 != 1_i8));

  // Verify constexpr.
  [[maybe_unused]] constexpr i8 e = []() {
    i8 a = 1_i8 + 2_i8 - 3_i8 * 4_i8 / 5_i8 % 6_i8 & 7_i8 | 8_i8 ^ 9_i8;
    [[maybe_unused]] bool b = 2_i8 == 3_i8;
    [[maybe_unused]] std::strong_ordering c = 2_i8 <=> 3_i8;
    [[maybe_unused]] i8 d = a << 1_u32;
    [[maybe_unused]] i8 e = a >> 1_u32;
    a += 1_i8;
    a -= 1_i8;
    a *= 1_i8;
    a /= 1_i8;
    a %= 1_i8;
    a &= 1_i8;
    a |= 1_i8;
    a ^= 1_i8;
    a <<= 1_u32;
    a >>= 1_u32;
    return a;
  }();
}

TEST(i8, Literals) {
  // Hex.
  static_assert((0x1C_i8).primitive_value == 0x1C);
  static_assert((0X1C_i8).primitive_value == 0X1C);
  static_assert((0X001C_i8).primitive_value == 0X1C);
  EXPECT_EQ((0x1C_i8).primitive_value, 0x1C);
  EXPECT_EQ((0X1C_i8).primitive_value, 0X1C);
  EXPECT_EQ((0X001C_i8).primitive_value, 0X1C);
  // Binary.
  static_assert((0b101_i8).primitive_value == 0b101);
  static_assert((0B101_i8).primitive_value == 0B101);
  static_assert((0b00101_i8).primitive_value == 0b101);
  EXPECT_EQ((0b101_i8).primitive_value, 0b101);
  EXPECT_EQ((0B101_i8).primitive_value, 0B101);
  EXPECT_EQ((0b00101_i8).primitive_value, 0b101);
  // Octal.
  static_assert((0123_i8).primitive_value == 0123);
  static_assert((000123_i8).primitive_value == 0123);
  EXPECT_EQ((0123_i8).primitive_value, 0123);
  EXPECT_EQ((000123_i8).primitive_value, 0123);
  // Decimal.
  static_assert((0_i8).primitive_value == 0);
  static_assert((1_i8).primitive_value == 1);
  static_assert((12_i8).primitive_value == 12);
  static_assert((123_i8).primitive_value == 123);
}

TEST(i8, Constants) {
  constexpr auto max = i8::MAX;
  static_assert(std::same_as<decltype(max), const i8>);
  EXPECT_EQ(max.primitive_value, 0x7f);
  constexpr auto min = i8::MIN;
  static_assert(std::same_as<decltype(min), const i8>);
  EXPECT_EQ(min.primitive_value, -0x7f - 1);
  constexpr auto bits = i8::BITS;
  static_assert(std::same_as<decltype(bits), const u32>);
  EXPECT_EQ(bits, 8u);
}

template <class From, class To>
concept IsImplicitlyConvertible =
    std::is_convertible_v<From, To> && std::is_nothrow_assignable_v<To&, From>;
template <class From, class To>
concept IsExplicitlyConvertible =
    std::constructible_from<To, From> && !std::is_convertible_v<From, To> &&
    !std::is_assignable_v<To&, From>;
template <class From, class To>
concept NotConvertible =
    !std::constructible_from<To, From> && !std::is_convertible_v<From, To> &&
    !std::is_assignable_v<To&, From>;

template <class T>
auto make_enum() {
  enum E : T {
    X,
    Y,
    Z,
    MIN = std::numeric_limits<T>::min(),
    MAX = std::numeric_limits<T>::max()
  };
  return E::Z;
}

template <class T>
auto make_enumclass() {
  enum class E : T {
    X,
    Y,
    Z,
    MIN = std::numeric_limits<T>::min(),
    MAX = std::numeric_limits<T>::max()
  };
  return E::Z;
}

#define ENUM(kind, T) decltype([]() { return make_enum##kind<T>(); }())

template <auto From, class To, int = [](To) constexpr { return 0; }(From)>
constexpr bool is_constexpr_convertible(int) {
  return true;
}
template <auto From, class To>
constexpr bool is_constexpr_convertible(...) {
  return false;
}

template <auto From, class To, int = [](To) constexpr { return 0; }(To(From))>
constexpr bool is_constexpr_constructible(int) {
  return true;
}
template <auto From, class To>
constexpr bool is_constexpr_constructible(...) {
  return false;
}

template <auto From, class To>
concept IsConstexprAssignable = requires(To to) {
  { to = From };
};

TEST(i8, CompileTimeConversion) {
  using Self = i8;

  static_assert(is_constexpr_convertible<0_i8, Self>(0));
  static_assert(!is_constexpr_convertible<0_i16, Self>(0));
  static_assert(!is_constexpr_convertible<0_i32, Self>(0));
  static_assert(!is_constexpr_convertible<0_i64, Self>(0));
  static_assert(!is_constexpr_convertible<0_isize, Self>(0));
  static_assert(is_constexpr_convertible<int8_t{0}, Self>(0));
  static_assert(!is_constexpr_convertible<int16_t{0}, Self>(0));
  static_assert(!is_constexpr_convertible<int32_t{0}, Self>(0));
  static_assert(!is_constexpr_convertible<int64_t{0}, Self>(0));
  static_assert(!is_constexpr_convertible<ptrdiff_t{0}, Self>(0));

  static_assert(IsConstexprAssignable<0_i8, Self>);
  static_assert(!IsConstexprAssignable<0_i16, Self>);
  static_assert(!IsConstexprAssignable<0_i32, Self>);
  static_assert(!IsConstexprAssignable<0_i64, Self>);
  static_assert(!IsConstexprAssignable<0_isize, Self>);
  static_assert(IsConstexprAssignable<int8_t{0}, Self>);
  static_assert(!IsConstexprAssignable<int16_t{0}, Self>);
  static_assert(!IsConstexprAssignable<int32_t{0}, Self>);
  static_assert(!IsConstexprAssignable<int64_t{0}, Self>);
  static_assert(!IsConstexprAssignable<ptrdiff_t{0}, Self>);

  static_assert(!IsExplicitlyConvertible<uint8_t, Self>);
  static_assert(!IsExplicitlyConvertible<uint16_t, Self>);
  static_assert(!IsExplicitlyConvertible<uint32_t, Self>);
  static_assert(!IsExplicitlyConvertible<uint64_t, Self>);
  static_assert(!IsExplicitlyConvertible<u8, Self>);
  static_assert(!IsExplicitlyConvertible<u16, Self>);
  static_assert(!IsExplicitlyConvertible<u32, Self>);
  static_assert(!IsExplicitlyConvertible<u64, Self>);
  static_assert(!IsExplicitlyConvertible<usize, Self>);
}

TEST(i8, CompileTimeConversionEnum) {
  using Self = i8;

  static_assert(is_constexpr_convertible<ENUM(, int8_t)::X, Self>(0));
  static_assert(!is_constexpr_convertible<ENUM(, int16_t)::X, Self>(0));
  static_assert(!is_constexpr_convertible<ENUM(, int32_t)::X, Self>(0));
  static_assert(!is_constexpr_convertible<ENUM(, int64_t)::X, Self>(0));

  // Conversion from enum class is explicit (constructible) instead of implicit
  // (convertible).
  static_assert(!is_constexpr_convertible<ENUM(class, int8_t)::X, Self>(0));
  static_assert(!is_constexpr_convertible<ENUM(class, int16_t)::X, Self>(0));
  static_assert(!is_constexpr_convertible<ENUM(class, int32_t)::X, Self>(0));
  static_assert(!is_constexpr_convertible<ENUM(class, int64_t)::X, Self>(0));
  static_assert(is_constexpr_constructible<ENUM(class, int8_t)::X, Self>(0));
  static_assert(!is_constexpr_constructible<ENUM(class, int16_t)::X, Self>(0));
  static_assert(!is_constexpr_constructible<ENUM(class, int32_t)::X, Self>(0));
  static_assert(!is_constexpr_constructible<ENUM(class, int64_t)::X, Self>(0));
}

TEST(i8, ToPrimitive) {
  static_assert(IsImplicitlyConvertible<i8, int8_t>);
  static_assert(IsImplicitlyConvertible<i8, int16_t>);
  static_assert(IsImplicitlyConvertible<i8, int32_t>);
  static_assert(IsImplicitlyConvertible<i8, int64_t>);
  static_assert(NotConvertible<i8, uint8_t>);
  static_assert(NotConvertible<i8, uint16_t>);
  static_assert(NotConvertible<i8, uint32_t>);
  static_assert(NotConvertible<i8, uint64_t>);
  static_assert(NotConvertible<i8, size_t>);
}

template <class Sus, class Prim>
concept CanOperator = requires(Sus sus, Prim prim) {
  { sus + prim } -> std::same_as<Sus>;
  { sus - prim } -> std::same_as<Sus>;
  { sus* prim } -> std::same_as<Sus>;
  { sus / prim } -> std::same_as<Sus>;
  { sus % prim } -> std::same_as<Sus>;
  { sus& prim } -> std::same_as<Sus>;
  { sus | prim } -> std::same_as<Sus>;
  { sus ^ prim } -> std::same_as<Sus>;
  { prim + sus } -> std::same_as<Sus>;
  { prim - sus } -> std::same_as<Sus>;
  { prim* sus } -> std::same_as<Sus>;
  { prim / sus } -> std::same_as<Sus>;
  { prim % sus } -> std::same_as<Sus>;
  { prim& sus } -> std::same_as<Sus>;
  { prim | sus } -> std::same_as<Sus>;
  { prim ^ sus } -> std::same_as<Sus>;
  { sus += prim };
  { sus -= prim };
  { sus *= prim };
  { sus /= prim };
  { sus %= prim };
  { sus &= prim };
  { sus |= prim };
  { sus ^= prim };
};

// clang-format off
template <class Sus, class Prim>
concept CantOperator =
  !requires(Sus sus, Prim prim) { { sus + prim }; } &&
  !requires(Sus sus, Prim prim) { { sus - prim }; } &&
  !requires(Sus sus, Prim prim) { { sus* prim }; } &&
  !requires(Sus sus, Prim prim) { { sus / prim }; } &&
  !requires(Sus sus, Prim prim) { { sus % prim }; } &&
  !requires(Sus sus, Prim prim) { { sus& prim }; } &&
  !requires(Sus sus, Prim prim) { { sus | prim }; } &&
  !requires(Sus sus, Prim prim) { { sus ^ prim }; } &&
  !requires(Sus sus, Prim prim) { { prim + sus }; } &&
  !requires(Sus sus, Prim prim) { { prim - sus }; } &&
  !requires(Sus sus, Prim prim) { { prim* sus }; } &&
  !requires(Sus sus, Prim prim) { { prim / sus }; } &&
  !requires(Sus sus, Prim prim) { { prim % sus }; } &&
  !requires(Sus sus, Prim prim) { { prim& sus }; } &&
  !requires(Sus sus, Prim prim) { { prim | sus }; } &&
  !requires(Sus sus, Prim prim) { { prim ^ sus }; } &&
  !requires(Sus sus, Prim prim) { { sus += prim }; } &&
  !requires(Sus sus, Prim prim) { { sus -= prim }; } &&
  !requires(Sus sus, Prim prim) { { sus *= prim }; } &&
  !requires(Sus sus, Prim prim) { { sus /= prim }; } &&
  !requires(Sus sus, Prim prim) { { sus %= prim }; } &&
  !requires(Sus sus, Prim prim) { { sus &= prim }; } &&
  !requires(Sus sus, Prim prim) { { sus |= prim }; } &&
  !requires(Sus sus, Prim prim) { { sus ^= prim }; };
// clang-format on

template <class Lhs, class Rhs>
concept CanShift = requires(Lhs l, Rhs r) {
  { l << r } -> std::same_as<Lhs>;
  { l >> r } -> std::same_as<Lhs>;
};

// clang-format off
template <class Lhs, class Rhs>
concept CantShift =
  !requires(Lhs l, Rhs r) { { l << r }; } &&
  !requires(Lhs l, Rhs r) { { l >> r }; };
// clang-format on

TEST(i8, OperatorsWithPrimitives) {
  static_assert(CantOperator<i8, uint8_t>);
  static_assert(CantOperator<i8, uint16_t>);
  static_assert(CantOperator<i8, uint32_t>);
  static_assert(CantOperator<i8, uint64_t>);

  static_assert(CanOperator<i8, int8_t>);
  static_assert(CantOperator<i8, int16_t>);
  static_assert(CantOperator<i8, int32_t>);
  static_assert(CantOperator<i8, int64_t>);
  static_assert(CantOperator<i8, ENUM(, uint8_t)>);
  static_assert(CantOperator<i8, ENUM(, uint16_t)>);
  static_assert(CantOperator<i8, ENUM(, uint32_t)>);
  static_assert(CantOperator<i8, ENUM(, uint64_t)>);

  static_assert(CanOperator<i8, ENUM(, int8_t)>);
  static_assert(CantOperator<i8, ENUM(, int16_t)>);
  static_assert(CantOperator<i8, ENUM(, int32_t)>);
  static_assert(CantOperator<i8, ENUM(, int64_t)>);

  static_assert(CantOperator<i8, ENUM(class, uint8_t)>);
  static_assert(CantOperator<i8, ENUM(class, uint16_t)>);
  static_assert(CantOperator<i8, ENUM(class, uint32_t)>);
  static_assert(CantOperator<i8, ENUM(class, uint64_t)>);

  static_assert(CantOperator<i8, ENUM(class, int8_t)>);
  static_assert(CantOperator<i8, ENUM(class, int16_t)>);
  static_assert(CantOperator<i8, ENUM(class, int32_t)>);
  static_assert(CantOperator<i8, ENUM(class, int64_t)>);

  static_assert(CanShift<i8, uint8_t>);
  static_assert(CanShift<i8, uint16_t>);
  static_assert(CanShift<i8, uint32_t>);
  static_assert(CanShift<i8, uint64_t>);
  static_assert(CanShift<i8, ENUM(, uint8_t)>);
  static_assert(CanShift<i8, ENUM(, uint16_t)>);
  static_assert(CanShift<i8, ENUM(, uint32_t)>);
  static_assert(CanShift<i8, ENUM(, uint64_t)>);
  static_assert(CantShift<i8, ENUM(class, uint8_t)>);
  static_assert(CantShift<i8, ENUM(class, uint16_t)>);
  static_assert(CantShift<i8, ENUM(class, uint32_t)>);
  static_assert(CantShift<i8, ENUM(class, uint64_t)>);

  static_assert(CantShift<i8, int8_t>);

  static_assert(CantShift<int8_t, i8>);
  static_assert(CantShift<uint8_t, i8>);
  static_assert(CantShift<ENUM(, int8_t), i8>);
  static_assert(CantShift<ENUM(, uint8_t), i8>);
  static_assert(CantShift<ENUM(class, int8_t), i8>);
  static_assert(CantShift<ENUM(class, uint8_t), i8>);
}

TEST(i8, From) {
  using signed_char = signed char;

  static_assert(!sus::construct::From<i8, bool>);
  static_assert(sus::construct::From<i8, signed_char>);
  static_assert(!sus::construct::From<i8, size_t>);
  static_assert(sus::construct::From<i8, int8_t>);
  static_assert(!sus::construct::From<i8, int16_t>);
  static_assert(!sus::construct::From<i8, int32_t>);
  static_assert(!sus::construct::From<i8, int64_t>);
  static_assert(!sus::construct::From<i8, uint8_t>);
  static_assert(!sus::construct::From<i8, uint16_t>);
  static_assert(!sus::construct::From<i8, uint32_t>);
  static_assert(!sus::construct::From<i8, uint64_t>);
  static_assert(sus::construct::TryFrom<i8, bool>);
  static_assert(sus::construct::TryFrom<i8, signed_char>);
  static_assert(sus::construct::TryFrom<i8, size_t>);
  static_assert(sus::construct::TryFrom<i8, int8_t>);
  static_assert(sus::construct::TryFrom<i8, int16_t>);
  static_assert(sus::construct::TryFrom<i8, int32_t>);
  static_assert(sus::construct::TryFrom<i8, int64_t>);
  static_assert(sus::construct::TryFrom<i8, uint8_t>);
  static_assert(sus::construct::TryFrom<i8, uint16_t>);
  static_assert(sus::construct::TryFrom<i8, uint32_t>);
  static_assert(sus::construct::TryFrom<i8, uint64_t>);

  static_assert(sus::construct::From<i8, ENUM(, signed_char)>);
  static_assert(!sus::construct::From<i8, ENUM(, size_t)>);
  static_assert(sus::construct::From<i8, ENUM(, int8_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, int16_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, int32_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, int64_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, uint8_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, uint16_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, uint32_t)>);
  static_assert(!sus::construct::From<i8, ENUM(, uint64_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, signed_char)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, size_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, int8_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, int16_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, int32_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, int64_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, uint8_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, uint16_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, uint32_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(, uint64_t)>);

  static_assert(sus::construct::From<i8, ENUM(class, signed_char)>);
  static_assert(!sus::construct::From<i8, ENUM(class, size_t)>);
  static_assert(sus::construct::From<i8, ENUM(class, int8_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, int16_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, int32_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, int64_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, uint8_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, uint16_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, uint32_t)>);
  static_assert(!sus::construct::From<i8, ENUM(class, uint64_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, signed_char)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, size_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, int8_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, int16_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, int32_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, int64_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, uint8_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, uint16_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, uint32_t)>);
  static_assert(sus::construct::TryFrom<i8, ENUM(class, uint64_t)>);

  EXPECT_EQ(i8::from(signed_char{2}), 2_i8);
  EXPECT_EQ(i8::from(int8_t{2}), 2_i8);

  EXPECT_EQ(i8::try_from(signed_char{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(size_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(int8_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(int16_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(int32_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(int64_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(uint8_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(uint16_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(uint32_t{2}).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(uint64_t{2}).unwrap(), 2_i8);

  EXPECT_TRUE(i8::try_from(int16_t{i16::MIN}).is_err());
  EXPECT_TRUE(i8::try_from(int16_t{i16::MAX}).is_err());
  EXPECT_TRUE(i8::try_from(uint8_t{u8::MAX}).is_err());
  EXPECT_TRUE(i8::try_from(uint16_t{u16::MAX}).is_err());

  EXPECT_EQ(i8::from(ENUM(, signed char)::Z), 2_i8);
  EXPECT_EQ(i8::from(ENUM(, int8_t)::Z), 2_i8);

  EXPECT_EQ(i8::try_from(ENUM(, signed_char)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, size_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, int8_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, int16_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, int32_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, int64_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, uint8_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, uint16_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, uint32_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(, uint64_t)::Z).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(ENUM(class, uint64_t)::Z).unwrap(), 2_i8);

  EXPECT_TRUE(i8::try_from(ENUM(, int16_t)::MIN).is_err());
  EXPECT_TRUE(i8::try_from(ENUM(, int16_t)::MAX).is_err());
  EXPECT_TRUE(i8::try_from(ENUM(, uint8_t)::MAX).is_err());
  EXPECT_TRUE(i8::try_from(ENUM(, uint16_t)::MAX).is_err());

  static_assert(sus::construct::From<i8, i8>);
  static_assert(!sus::construct::From<i8, i16>);
  static_assert(!sus::construct::From<i8, i32>);
  static_assert(!sus::construct::From<i8, i64>);
  static_assert(!sus::construct::From<i8, isize>);
  static_assert(!sus::construct::From<i8, u8>);
  static_assert(!sus::construct::From<i8, u16>);
  static_assert(!sus::construct::From<i8, u32>);
  static_assert(!sus::construct::From<i8, u64>);
  static_assert(!sus::construct::From<i8, usize>);
  static_assert(!sus::construct::From<i8, uptr>);
  static_assert(sus::construct::TryFrom<i8, i8>);
  static_assert(sus::construct::TryFrom<i8, i16>);
  static_assert(sus::construct::TryFrom<i8, i32>);
  static_assert(sus::construct::TryFrom<i8, i64>);
  static_assert(sus::construct::TryFrom<i8, isize>);
  static_assert(sus::construct::TryFrom<i8, u8>);
  static_assert(sus::construct::TryFrom<i8, u16>);
  static_assert(sus::construct::TryFrom<i8, u32>);
  static_assert(sus::construct::TryFrom<i8, u64>);
  static_assert(sus::construct::TryFrom<i8, usize>);
  static_assert(sus::construct::TryFrom<i8, uptr>);

  EXPECT_EQ(i8::from(2_i8), 2_i8);

  EXPECT_EQ(i8::try_from(2_i8).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_i16).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_i32).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_i64).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_isize).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_u8).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_u16).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_u32).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_u64).unwrap(), 2_i8);
  EXPECT_EQ(i8::try_from(2_usize).unwrap(), 2_i8);

  EXPECT_TRUE(i8::try_from(i16::MIN).is_err());
  EXPECT_TRUE(i8::try_from(i16::MAX).is_err());
  EXPECT_TRUE(i8::try_from(u8::MAX).is_err());
  EXPECT_TRUE(i8::try_from(u16::MAX).is_err());
}

TEST(i8, InvokeEverything) {
  auto i = 10_i8, j = 11_i8;
  auto s = 3_u8;
  auto a = sus::Array<u8, sizeof(i8)>();

  (void)i.is_negative();
  (void)i.is_positive();
  (void)i.signum();

  (void)i.abs();
  (void)i.checked_abs();
  (void)i.overflowing_abs();
  (void)i.saturating_abs();
  (void)i.unsigned_abs();
  (void)i.wrapping_abs();
  (void)i.abs_diff(j);

  (void)i.checked_add(j);
  (void)i.checked_add_unsigned(s);
  (void)i.overflowing_add(j);
  (void)i.overflowing_add_unsigned(s);
  (void)i.saturating_add(j);
  (void)i.saturating_add_unsigned(s);
  (void)i.unchecked_add(unsafe_fn, j);
  (void)i.wrapping_add(j);
  (void)i.wrapping_add_unsigned(s);

  (void)i.checked_div(j);
  (void)i.overflowing_div(j);
  (void)i.saturating_div(j);
  (void)i.wrapping_div(j);

  (void)i.checked_mul(j);
  (void)i.overflowing_mul(j);
  (void)i.saturating_mul(j);
  (void)i.unchecked_mul(unsafe_fn, j);
  (void)i.wrapping_mul(j);

  (void)i.checked_neg();
  (void)i.overflowing_neg();
  (void)i.wrapping_neg();

  (void)i.checked_rem(j);
  (void)i.overflowing_rem(j);
  (void)i.wrapping_rem(j);

  (void)i.div_euclid(j);
  (void)i.checked_div_euclid(j);
  (void)i.overflowing_div_euclid(j);
  (void)i.wrapping_div_euclid(j);
  (void)i.rem_euclid(j);
  (void)i.checked_rem_euclid(j);
  (void)i.overflowing_rem_euclid(j);
  (void)i.wrapping_rem_euclid(j);

  (void)i.checked_shl(1_u32);
  (void)i.overflowing_shl(1_u32);
  (void)i.wrapping_shl(1_u32);
  (void)i.checked_shr(1_u32);
  (void)i.overflowing_shr(1_u32);
  (void)i.wrapping_shr(1_u32);

  (void)i.checked_sub(j);
  (void)i.checked_sub_unsigned(s);
  (void)i.overflowing_sub(j);
  (void)i.overflowing_sub_unsigned(s);
  (void)i.saturating_sub(j);
  (void)i.saturating_sub_unsigned(s);
  (void)i.unchecked_sub(unsafe_fn, j);
  (void)i.wrapping_sub(j);
  (void)i.wrapping_sub_unsigned(s);

  (void)i.count_ones();
  (void)i.count_zeros();
  (void)i.leading_ones();
  (void)i.leading_zeros();
  (void)i.trailing_ones();
  (void)i.trailing_zeros();
  (void)i.reverse_bits();
  (void)i.rotate_left(1_u32);
  (void)i.rotate_right(1_u32);
  (void)i.swap_bytes();

  (void)i.pow(1_u32);
  (void)i.checked_pow(1_u32);
  (void)i.overflowing_pow(1_u32);
  (void)i.wrapping_pow(1_u32);

  (void)i.checked_log2();
  (void)i.log2();
  (void)i.checked_log10();
  (void)i.log10();
  (void)i.checked_log(j);
  (void)i.log(j);

  (void)i.from_be(j);
  (void)i.from_le(j);
  (void)i.to_be();
  (void)i.to_le();
  (void)i.to_be_bytes();
  (void)i.to_le_bytes();
  (void)i.to_ne_bytes();
  (void)i.from_be_bytes(a);
  (void)i.from_le_bytes(a);
  (void)i.from_ne_bytes(a);

  i = -j;
  i = ~j;

  i = j + j;
  i = j - j;
  i = j * j;
  i = j / j;
  i = j % j;
  i = j & j;
  i = j | j;
  i = j ^ j;
  i = j << 1_u32;
  i = j >> 1_u32;

  i += j;
  i -= j;
  i *= j;
  i /= j;
  i %= j;
  i &= j;
  i |= j;
  i ^= j;
  i <<= 1_u32;
  i >>= 1_u32;

  [[maybe_unused]] auto b = i == j;
  [[maybe_unused]] auto z = i >= j;
}

TEST(i8, fmt) {
  static_assert(fmt::is_formattable<i8, char>::value);
  EXPECT_EQ(fmt::format("{}", -21_i8), "-21");
  EXPECT_EQ(fmt::format("{}", 123_i8), "123");
  EXPECT_EQ(fmt::format("{:+#x}", 123_i8), "+0x7b");
}

}  // namespace
