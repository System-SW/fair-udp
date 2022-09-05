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

#include "fair-udp-header.h"

using namespace ns3;

TypeId
FairUdpHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::FairUdpHeader")
    .SetParent<Header>()
    .AddConstructor<FairUdpHeader>()
    ;
  return tid;
}

TypeId
FairUdpHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t
FairUdpHeader::GetSerializedSize() const
{
  return 6;
}

void
FairUdpHeader::Serialize(Buffer::Iterator start) const
{
  // write protocol id
  start.WriteU32(PROTOCOL_ID);

  // write protocol bit field
  start.WriteHtonU32(bit_field_);
}

uint32_t
FairUdpHeader::Deserialize(Buffer::Iterator start)
{
  NS_ASSERT(PROTOCOL_ID == start.ReadU32()); // check Protocol ID
  bit_field_ = start.ReadNtohU32();          // read bit_field

  return sizeof(uint32_t) * 2; // the number of bytes consumed
}

void
FairUdpHeader::Print(std::ostream &os) const
{
  os << " Sequence Number: " << GetSequence()
     << " NACK=" << IsOn<Bit::NACK>()
     << " Reset=" << IsOn<Bit::RESET>();
}

FairUdpHeader &
FairUdpHeader::operator |= (Bit bit)    
{
  bit_field_ |= static_cast<uint32_t>(bit);
  return *this;
}

void
FairUdpHeader::SetSequence(uint16_t seq)
{
  bit_field_ &= 0xFFFF0000;     // clean up seq number
  bit_field_ |= seq;            // set seq number
}

uint16_t
FairUdpHeader::GetSequence() const
{
  return static_cast<uint16_t>(bit_field_ & 0x0000FFFF);
}
