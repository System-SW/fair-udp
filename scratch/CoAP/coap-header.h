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
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;

    uint32_t GetSerializedSize() const override;

    void Serialize(Buffer::Iterator start) const override;

    uint32_t Deserialize(Buffer::Iterator start) override;

    void Print(std::ostream& os) const override;

  private:
    union
    {
      uint8_t slot[4];
      uint32_t merged{0};
    } m_fixed_hdr;

    uint64_t m_token;
    /* no CoAP option implemented */
    constexpr static uint8_t EOH = 0xff; /* header end flag */
    // and below is payload

  public:
    uint8_t GetVersion() const;

    // use only lower 2bits
    void SetVersion(uint8_t version);

    uint8_t GetType() const;

    // use only lower 2bits
    void SetType(uint8_t type);

    uint8_t GetTKL() const;

    // use only lower 4bits
    void SetTKL(uint8_t tkl);

    uint8_t GetClass() const;

    // use only lower
    void SetClass(uint8_t cls);

    uint8_t GetCode() const;

    void SetCode(uint8_t code);

    uint16_t GetMID() const;

    void SetMID(uint16_t mid);

    uint64_t GetToken() const;

    void SetToken(uint64_t token);

    // Prepare CoAP Header
    enum Type : uint8_t   // 2bits
      {
        // Request
        CON = 0,
        NON = 1,

        // Response
        ACK = 2,
        RST = 3,
      };

    enum Class : uint8_t
      {
        METHOD = 0,
        SUCCESS = 2,
        CLIENT_ERR = 4,
        SERVER_ERR = 5,
        SIGNAL = 7,
      };

    enum Method : uint8_t
      {
        EMPTY = 0,
        GET = 1,
        POST = 2,
        PUT = 3,
        DELETE = 4,
        FETCH = 5,
        PATCH = 6,
        iPATCH = 7,
      };

    enum Success : uint8_t
      {
        CREATED = 1,
        DELETED = 2,
        VALIED = 3,
        CHANGED = 4,
        CONTENT = 5,
        CONTINUE = 31,
      };

    static void PreparePut(CoAPHeader &hdr, uint8_t tkl, uint64_t token, uint16_t mid, bool con = false);

  };

}
