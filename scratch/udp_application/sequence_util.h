/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
 * Author: Chang-Hui Kim <kch9001@gmail.com>
 */
#pragma once
#ifndef SEQUENCE_UTIL_H
#define SEQUENCE_UTIL_H

// get maximum number of given bited-unsigned-integer
// for instance, get the maximum value of 22bit unsigned integer.

#include <type_traits>
#include <cstdint>

namespace ns3 {

template <unsigned char MIN, unsigned char v, unsigned char MAX>
struct with_in
{
  constexpr static bool value = MIN <= v and v <= MAX;
};
template <unsigned char MIN, unsigned char v, unsigned char MAX>
constexpr static bool with_in_v = with_in<MIN, v, MAX>::value;

// 8bits, 16bits, 32bits, 64bits
template <unsigned char bits, typename T = void>
struct containable_field
{};

template <unsigned char bits>
struct containable_field<bits, std::enable_if_t<with_in_v<1, bits, 8>>>
{
  using type = uint8_t;
};

template <unsigned char bits>
struct containable_field<bits, std::enable_if_t<with_in_v<9, bits, 16>>>
{
  using type = uint16_t;
};

template <unsigned char bits>
struct containable_field<bits, std::enable_if_t<with_in_v<17, bits, 32>>>
{
  using type = uint32_t;
};

template <unsigned char bits>
struct containable_field<bits, std::enable_if_t<with_in_v<33, bits, 64>>>
{
  using type = uint64_t;
};
template <unsigned char bits>
using containable_field_t = typename containable_field<bits>::type;

template <unsigned char bits>
struct all_one_field
{
  using field_type = containable_field_t<bits>;
  constexpr static field_type value = ~(field_type{0});
};
template <unsigned char bits>
constexpr static auto all_one_field_v = all_one_field<bits>::value;

template <unsigned char bits>
struct maxima
{
  using field_type = typename containable_field<bits>::type;

  constexpr static field_type max_bit_field()
  {
    auto max_value = all_one_field_v<bits>;
    constexpr auto shift_value = sizeof(field_type) * 8 - bits;
    return max_value & (max_value >> shift_value); // remove higier bits
  }

  constexpr static field_type value = max_bit_field();
};
template <unsigned char bits>
constexpr static auto max_value = maxima<bits>::value;

// XXX: need specialization for multiples of 8 (8bits, 16bits...)
template <unsigned char bits>
class sequence
{
private:
  containable_field_t<bits> seq_{0};

public:
  sequence() = default;

  sequence(containable_field_t<bits> seq) : seq_(seq)
  {
    if(seq_ >= max_value<bits>)
      {
        seq_ = 0; // overflowed
      }
  }

  containable_field_t<bits> get() const
  {
    return seq_;
  }

  void set(containable_field<bits> seq)
  {
    seq_ = seq < max_value<bits> ? seq : 0;
  }

  sequence<bits> operator++(int)
  {
    seq_++;
    if(seq_ >= max_value<bits>)
    {
      seq_ = 0; // overflowed
    }
    return { get() };
  }
};

} // namespace fdp

#endif /* SEQUENCE_UTIL_H */
