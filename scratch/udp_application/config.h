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
  constexpr unsigned int UAV_NUM = 60;
  constexpr uint32_t TEST_TIME = 120; /* seconds */
  constexpr uint32_t AVERAGE_INTERVAL = 2000; /* ms */
  constexpr const char *XRANGE = "set xrange [0:+120]";
}
