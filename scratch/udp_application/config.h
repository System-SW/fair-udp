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

#include <cstdint>

namespace ns3
{
  constexpr static unsigned int UAV_NUM = 15;
  constexpr static uint32_t TEST_TIME = 120; /* seconds */
  constexpr static uint32_t AVERAGE_INTERVAL = 1000; /* ms */
  constexpr static const char *XRANGE = "set xrange [0:+120]";

  enum target_protocol
    {
      FAIR_UDP,
      UDP,
      TCP,
    };
  constexpr static target_protocol TARGET_PROTO = UDP;
}
