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
#ifndef FAIR_UDP_HEADER_H
#define FAIR_UDP_HEADER_H

#include "sequence_util.h"
#include "ns3/header.h"

namespace ns3
{
  /*
    |-------------------------------------------------------|
    | 1 bit | 1 bit | 22bit (preserved) |  8 bit (unsigned) |
    |-------+-------+-------------------+-------------------|
    | NACK  | RESET | NACK Sequence     | Sequence Number   |
  */


  using sequence_t = uint8_t;
  using nack_seq_t = sequence<22>; // 22 bits uint
  class FairUdpHeader : public Header
  {
    static constexpr uint32_t SEQ_MASK = (sequence_t(~0u));
    static constexpr uint32_t OPT_MASK = ~SEQ_MASK;
  public:
    static constexpr size_t HEADER_SIZE = sizeof(uint32_t);
    enum class Bit: uint32_t
      {
        NACK = 0x1u << 31,       // use enclosed sequence number in the next message
        RESET = 0x1u << 30,      // request reset sequence number to 0
      };

    static TypeId GetTypeId()
    {
      static TypeId tid = TypeId("ns3::FairUdpHeader")
        .SetParent<Header>()
        .AddConstructor<FairUdpHeader>()
        ;
      return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
      return GetTypeId();
    }

    uint32_t GetSerializedSize() const override
    {
      return HEADER_SIZE; // the number of bytes consumed
    }

    void Serialize(Buffer::Iterator start) const override
    {
      // write protocol bit field
      start.WriteHtonU32(bit_field_);
    }
    
    uint32_t Deserialize(Buffer::Iterator start) override
    {
      bit_field_ = start.ReadNtohU32();          // read bit_field
      return HEADER_SIZE; // the number of bytes consumed
    }

    void Print(std::ostream& os) const override
    {
      os << " Sequence Number: " << GetSequence()
         << " NACK=" << IsOn<Bit::NACK>()
         << " Reset=" << IsOn<Bit::RESET>();
    }

    // method for set bit_field_
    FairUdpHeader& operator |= (Bit bit)
    {
      bit_field_ |= static_cast<uint32_t>(bit);
      return *this;
    }

    template <Bit bit>
    bool IsOn() const
    {
      return (bit_field_ & static_cast<uint32_t>(bit)) != 0;
    }

    void SetSequence(sequence_t seq)
    {
      bit_field_ &= OPT_MASK;     // clean up seq number
      bit_field_ |= seq;          // set seq number
    }

    sequence_t GetSequence() const
    {
      return static_cast<sequence_t>(bit_field_ & SEQ_MASK);
    }

    void SetNackSequence(nack_seq_t nack_seq)
    {
      NS_ASSERT(nack_seq.get() <= ((0x1u << 22) - 1));
      bit_field_ &= ~(((0x1u << 22) - 1) << 8);
      bit_field_ |= static_cast<uint32_t>(nack_seq.get()) << 8;
    }

    nack_seq_t GetNackSequence() const
    {
      uint32_t nack_seq = bit_field_ & ~(0x3u << 30);
      return nack_seq_t{nack_seq};
    }

  private:
    uint32_t bit_field_{0};
  };
}    

#endif /* FAIR_UDP_HEADER_H */
