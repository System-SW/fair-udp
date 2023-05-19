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
#include "ns3/packet.h"
#include "fdp-header.h"

using namespace ns3;

constexpr static uint16_t BIT_12_MAX = uint16_t((0x1 << 11) - 1);


TypeId FDPMessageHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::FDPMessageHeader")
    .SetParent<Header>()
    .AddConstructor<FDPMessageHeader>()
    ;
  return tid;
}

TypeId FDPMessageHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t FDPMessageHeader::GetSerializedSize() const
{
  return sizeof(uint16_t);
}

void FDPMessageHeader::Serialize(Buffer::Iterator start) const
{
  union {
    uint8_t bytes[2];
    uint16_t combined;
  } transform{0};


  // XXX: have to check interval is in 12 bits value (overflow check)
  auto interval = std::min(BIT_12_MAX, m_interval);

  // combined endian to big endian
  transform.combined = interval;
  std::swap(transform.bytes[0], transform.bytes[1]);

  uint16_t field{0};

  field ^= (static_cast<uint16_t>(m_seq_bit) << 15);
  field ^= (uint16_t(m_msg_seq) << 12);
  field ^= (transform.combined << 1);
  start.WriteU16(field);
}

uint32_t FDPMessageHeader::Deserialize(Buffer::Iterator start)
{
  const uint16_t field = start.ReadU16();
  m_seq_bit = static_cast<bool>(field & (uint16_t(0x1) << 15));
  m_msg_seq = static_cast<uint16_t>(field & (0x2 << 13)) >> 13;

  union {
    uint8_t bytes[2];
    uint16_t combined;
  } transform{0};

  transform.combined = (field & (0xFFF << 1)) >> 1;
  std::swap(transform.bytes[0], transform.bytes[1]);

  m_interval = transform.combined;

  return GetSerializedSize();
}

void FDPMessageHeader::Print(std::ostream& os) const
{
  // XXX: implement this
  os << "FDPMessageHeader: "
     << " seq bit: " << m_seq_bit
     << " msg seq: " << m_msg_seq
     << " msg interval (ms): " << m_interval
     << '\n';
}

bool FDPMessageHeader::FlipSeqBit()
{
  return m_seq_bit = !m_seq_bit;
}

bool FDPMessageHeader::GetSeqBit() const
{
  return m_seq_bit;
}

unsigned int FDPMessageHeader::IncMsgSeq()
{
  m_msg_seq++;
  if (m_msg_seq > 2)
    {
      m_msg_seq = 0;
    }
  return m_msg_seq;
}

unsigned int FDPMessageHeader::GetMsgSeq() const
{
  return m_msg_seq;
}

Time FDPMessageHeader::GetMsgInterval() const
{
  return MilliSeconds(m_interval);
}

void FDPMessageHeader::SetMsgInterval(uint16_t interval_ms)
{
  m_interval = std::min(BIT_12_MAX, interval_ms);
}

void FDPMessageHeader::SetMsgInterval(Time interval_ms)
{
  auto interval = interval_ms.GetMilliSeconds();
  NS_ABORT_IF(interval > BIT_12_MAX);
  m_interval = interval;
}
