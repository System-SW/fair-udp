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
#include "ns3/header.h"

namespace ns3
{

  class CoAPHeader : public Header
  {
  public:
    static TypeId GetTypeId()
    {
      static TypeId tid = TypeId("ns3::CoAPHeader")
        .SetParent<Header>()
        .AddConstructor<CoAPHeader>()
        ;
      return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
      return GetTypeId();
    }

    uint32_t GetSerializedSize() const override
    {
      return sizeof(m_fixed_hdr) + GetTKL() + sizeof(uint8_t);
    }

    void Serialize(Buffer::Iterator start) const override
    {
      start.WriteHtonU32(m_fixed_hdr.merged);
      auto tkl = GetTKL();
      union {
        uint64_t token;
        uint8_t buffer[4];
      } convert;
      convert.token = m_token;
      start.Write(convert.buffer, tkl);
      start.Write(&EOH, 1);
    }

    uint32_t Deserialize(Buffer::Iterator start) override
    {
      m_fixed_hdr.merged = start.ReadNtohU32();

      auto tkl = GetTKL();
      union {
        uint64_t token;
        uint8_t buffer[4];
      } convert{0};
      start.Read(convert.buffer, tkl);
      m_token = convert.token;

      start.Read(convert.buffer, 1); // get rids of EOH

      return sizeof(m_fixed_hdr) + GetTKL() + sizeof(uint8_t);
    }

    void Print(std::ostream& os [[maybe_unused]]) const override
    {
      /* XXX: implement Print */
    }

  private:
    union
    {
      uint8_t slot[4];
      uint32_t merged;
    } m_fixed_hdr{0};

    uint64_t m_token;
    /* no CoAP option implemented */
    constexpr static uint8_t EOH = 0xff; /* header end flag */
    // and below is payload

  public:
    uint8_t GetVersion() const
    {
      return (0xC0 & m_fixed_hdr.slot[0]) >> 6;
    }

    // use only lower 2bits
    void SetVersion(uint8_t version)
    {
      m_fixed_hdr.slot[0] |= ((0x3 & version) << 6);
    }

    uint8_t GetType() const
    {
      return (0x30 & m_fixed_hdr.slot[0]) >> 4;
    }

    // use only lower 2bits
    void SetType(uint8_t type)
    {
      m_fixed_hdr.slot[0] |= ((0x3 & type) << 4);
    }

    uint8_t GetTKL() const
    {
      return (0xF & m_fixed_hdr.slot[0]);
    }

    // use only lower 4bits
    void SetTKL(uint8_t tkl)
    {
      m_fixed_hdr.slot[0] |= (0xF & tkl);
    }

    uint8_t GetClass() const
    {
      return (m_fixed_hdr.slot[1] >> 5) & 0x07;
    }

    // use only lower
    void SetClass(uint8_t cls)
    {
      m_fixed_hdr.slot[1] |= ((0x7 & cls) << 5);
    }

    uint8_t GetCode() const
    {
      return (m_fixed_hdr.slot[1]) & 0x1F;
    }

    void SetCode(uint8_t code)
    {
      m_fixed_hdr.slot[1] |= (0x1F & code);
    }

    uint16_t GetMID() const
    {
      return (m_fixed_hdr.slot[2] << 8) | m_fixed_hdr.slot[3];
    }

    void SetMID(uint16_t mid)
    {
      union split
      {
        uint16_t merged;
        uint8_t slot[2];
      } s_mid;
      s_mid.merged = mid;

      // XXX: Maybe cause endian problem?
      m_fixed_hdr.slot[2] = s_mid.slot[0];
      m_fixed_hdr.slot[3] = s_mid.slot[1];
    }

    uint64_t GetToken() const
    {
      return m_token;
    }

    void SetToken(uint64_t token)
    {
      m_token = token;
    }
    
  };
}
