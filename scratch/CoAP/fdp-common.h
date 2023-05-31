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
 * Author: Chang-Hui Kim <kch9001@gmail.com>
 */
#pragma once
#ifndef FDP_COMMON_H
#define FDP_COMMON_H
#include <cstdint>
#include "ns3/abort.h"

constexpr static inline uint16_t BIT_12_MAX = uint16_t((0x1 << 11) - 1);

inline uint16_t CastMilliSecondsToUint16(int64_t diff)
{
  NS_ABORT_IF(diff < 0);
  diff = std::min(int64_t(BIT_12_MAX), diff);
  uint16_t interval = static_cast<uint16_t>(diff);
  return interval;
}

#endif /* FDP_COMMON_H */
