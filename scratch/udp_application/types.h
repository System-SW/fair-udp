/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Chang-Hui Kim <kch9001@gmail.com>, Daeseong Ki <kmeos1579@gmail.com>
 */

#ifndef TYPES_H 
#define TYPES_H 

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#define _TYPES_DEFINE_NUM_UDL(IN_TYPE, OUT_TYPE)                                                                       \
  inline constexpr auto operator"" _##OUT_TYPE(IN_TYPE n)->OUT_TYPE { return static_cast<OUT_TYPE>(n); }

using i8 = std::int8_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, i8);

using u8 = std::uint8_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, u8);

using i16 = std::int16_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, i16);

using u16 = std::uint16_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, u16);

using i32 = std::int32_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, i32);

using u32 = std::uint32_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, u32);

using i64 = std::int64_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, i64)

using u64 = std::uint64_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, u64)

using f32 = float;
_TYPES_DEFINE_NUM_UDL(long double, f32)

using f64 = double;
_TYPES_DEFINE_NUM_UDL(long double, f64)

using f128 = long double;
_TYPES_DEFINE_NUM_UDL(long double, f128)

using sz = std::size_t;
_TYPES_DEFINE_NUM_UDL(unsigned long long, sz)

using ssz = std::conditional_t<std::is_same_v<sz, u32>, i32, i64>;
_TYPES_DEFINE_NUM_UDL(unsigned long long, ssz)

#undef _TYPESL_DEFINE_NUM_UDL

#endif
