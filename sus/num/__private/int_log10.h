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

#include <concepts>
#include <stdint.h>

#include "sus/macros/inline.h"
#include "sus/macros/pure.h"

// Based on https://doc.rust-lang.org/nightly/src/core/num/int_log10.rs.html
namespace sus::num::__private::int_log10 {

// 0 < val < 100_000
sus_pure_const sus_always_inline constexpr uint32_t less_than_5(
    uint32_t val) noexcept {
  // Similar to u8, when adding one of these constants to val,
  // we get two possible bit patterns above the low 17 bits,
  // depending on whether val is below or above the threshold.
  constexpr uint32_t C1 = 0b011'00000000000000000 - 10;     // 393206
  constexpr uint32_t C2 = 0b100'00000000000000000 - 100;    // 524188
  constexpr uint32_t C3 = 0b111'00000000000000000 - 1000;   // 916504
  constexpr uint32_t C4 = 0b100'00000000000000000 - 10000;  // 514288

  // Value of top bits:
  //                +c1  +c2  1&2  +c3  +c4  3&4   ^
  //         0..=9  010  011  010  110  011  010  000 = 0
  //       10..=99  011  011  011  110  011  010  001 = 1
  //     100..=999  011  100  000  110  011  010  010 = 2
  //   1000..=9999  011  100  000  111  011  011  011 = 3
  // 10000..=99999  011  100  000  111  100  100  100 = 4
  return (((val + C1) & (val + C2)) ^ ((val + C3) & (val + C4))) >> 17;
}

// 0 < val <= u8::MAX
sus_pure_const sus_always_inline constexpr uint32_t u8(uint8_t val) {
  return less_than_5(uint32_t{val});
}

// 0 < val <= u16::MAX
sus_pure_const sus_always_inline constexpr uint32_t u16(uint16_t val) noexcept {
  return less_than_5(uint32_t{val});
}

// 0 < val <= u32::MAX
sus_pure_const sus_always_inline constexpr uint32_t u32(uint32_t val) noexcept {
  auto log = uint32_t{0};
  if (val >= 100'000) {
    val /= 100'000;
    log += 5;
  }
  return log + less_than_5(val);
}

// 0 < val <= u64::MAX
sus_pure_const sus_always_inline constexpr uint32_t u64(uint64_t val) noexcept {
  auto log = uint32_t{0};
  if (val >= 10'000'000'000) {
    val /= 10'000'000'000;
    log += 10;
  }
  if (val >= 100'000) {
    val /= 100'000;
    log += 5;
  }
  return log + less_than_5(static_cast<uint32_t>(val));
}

sus_pure_const sus_always_inline constexpr uint32_t usize(
    std::same_as<uint32_t> auto val) noexcept {
  return u32(val);
}

sus_pure_const sus_always_inline constexpr uint32_t usize(
    std::same_as<uint64_t> auto val) noexcept {
  return u64(val);
}

sus_pure_const sus_always_inline constexpr uint32_t uptr(
    std::same_as<uint32_t> auto val) noexcept {
  return u32(val);
}

sus_pure_const sus_always_inline constexpr uint32_t uptr(
    std::same_as<uint64_t> auto val) noexcept {
  return u64(val);
}

sus_pure_const sus_always_inline constexpr uint32_t i8(int8_t val) {
  return u8(static_cast<uint8_t>(val));
}

sus_pure_const sus_always_inline constexpr uint32_t i16(int16_t val) noexcept {
  return u16(static_cast<uint16_t>(val));
}

sus_pure_const sus_always_inline constexpr uint32_t i32(int32_t val) noexcept {
  return u32(static_cast<uint32_t>(val));
}

sus_pure_const sus_always_inline constexpr uint32_t i64(int64_t val) noexcept {
  return u64(static_cast<uint64_t>(val));
}

sus_pure_const sus_always_inline constexpr uint32_t isize(
    int32_t val) noexcept {
  return usize(static_cast<uint32_t>(val));
}

constexpr sus_always_inline uint32_t isize(int64_t val) noexcept {
  return usize(static_cast<uint64_t>(val));
}

}  // namespace sus::num::__private::int_log10
