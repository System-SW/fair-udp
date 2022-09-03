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
 */

#include "fair_udp_header.h"

using namespace ns3;

TypeId
fair_udp_header::GetTypeId()
{
  static TypeId tid = TypeId("ns3::fair_udp_header")
    .SetParent<Header>()
    .AddConstructor<fair_udp_header>()
    ;
  return tid;
}

TypeId
fair_udp_header::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t
fair_udp_header::GetSerializedSize() const
{
  return 6;
}

void
fair_udp_header::Serialize(Buffer::Iterator start) const
{
  // 2 bytes first
  start.WriteU8(0xfe);
  start.WriteU8(0xef);

  // data part
  start.WriteHtonU32(data_);
}

uint32_t
fair_udp_header::Deserialize(Buffer::Iterator start)
{
  uint8_t tmp = start.ReadU8();
  NS_ASSERT(tmp == 0xfe);
  tmp = start.ReadU8();
  NS_ASSERT(tmp == 0xef);
  data_ = start.ReadNtohU32();
  return 6; // the number of bytes consumed
}

void
fair_udp_header::Print(std::ostream &os) const
{
  os << "Data = " << data_;
}

void
fair_udp_header::set_data(uint32_t data)
{
  data_ = data;
}

uint32_t
fair_udp_header::get_data() const
{
  return data_;
}
