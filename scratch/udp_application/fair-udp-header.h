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

#ifndef FAIR_UDP_HEADER_H
#define FAIR_UDP_HEADER_H

#include "ns3/header.h"

namespace ns3
{
  /*
    | Protocol ID   (32 bit)                                |
    |-------------------------------------------------------|
    | 1 bit | 1 bit | 14bit (preserved) | 16 bit (unsigned) |
    |-------+-------+-------------------+-------------------|
    | NACK  | RESET |                   | Sequence Number   |
   */
  class FairUdpHeader : public Header
  {
  public:
    static constexpr uint32_t PROTOCOL_ID = 0x12345678;
    static constexpr size_t HEADER_SIZE = 2 * sizeof(uint32_t);
    enum class Bit: uint32_t
      {
        NACK = 0x1 << 30,       // use enclosed sequence number in the next message
        RESET = 0x1 << 29,      // request reset sequence number to 0
      };

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // method for set bit_field_
    FairUdpHeader& operator |= (Bit bit);

    template <Bit bit>
    bool IsOn() const
    {
      return (bit_field_ & static_cast<uint32_t>(bit)) != 0;
    }

    void SetSequence(uint16_t seq);
    uint16_t GetSequence() const;

  private:
    uint32_t bit_field_{0};
  };
}    

#endif /* FAIR_UDP_HEADER_H */
