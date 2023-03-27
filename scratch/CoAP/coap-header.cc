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
#include <random>
#include "coap-header.h"

using namespace ns3;
// XXX: Assume Big Endian!!!

TypeId CoAPHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::CoAPHeader")
    .SetParent<Header>()
    .AddConstructor<CoAPHeader>()
    ;
  return tid;
}

TypeId CoAPHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t CoAPHeader::GetSerializedSize() const
{
  return sizeof(m_fixed_hdr) + GetTKL() + sizeof(uint8_t);
}

void CoAPHeader::Serialize(Buffer::Iterator start) const
{
  start.WriteU32(m_fixed_hdr.merged);
  auto tkl = GetTKL();
  union {
    uint64_t token;
    uint8_t buffer[4];
  } convert;
  convert.token = m_token;
  start.Write(convert.buffer, tkl);
  start.Write(&EOH, 1);
}

uint32_t CoAPHeader::Deserialize(Buffer::Iterator start)
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

void CoAPHeader::Print(std::ostream& os [[maybe_unused]]) const
{
  /* XXX: implement Print */
}

uint8_t CoAPHeader::GetVersion() const
{
  return (0xC0 & m_fixed_hdr.slot[0]) >> 6;
}

// use only lower 2bits
void CoAPHeader::SetVersion(uint8_t version)
{
  m_fixed_hdr.slot[0] |= ((0x3 & version) << 6);
}

uint8_t CoAPHeader::GetType() const
{
  return (0x30 & m_fixed_hdr.slot[0]) >> 4;
}

// use only lower 2bits
void CoAPHeader::SetType(CoAPHeader::Type type)
{
  auto type_val = static_cast<uint8_t>(type);
  m_fixed_hdr.slot[0] |= ((0x3 & type_val) << 4);
}

uint8_t CoAPHeader::GetTKL() const
{
  return (0xF & m_fixed_hdr.slot[0]);
}

// use only lower 4bits
void CoAPHeader::SetTKL(uint8_t tkl)
{
  m_fixed_hdr.slot[0] |= (0xF & tkl);
}

CoAPHeader::Class CoAPHeader::GetClass() const
{
  return CoAPHeader::Class((m_fixed_hdr.slot[1] >> 5) & 0x07);
}

// use only lower
void CoAPHeader::SetClass(CoAPHeader::Class cls)
{
  auto cls_val = static_cast<uint8_t>(cls);
  m_fixed_hdr.slot[1] |= ((0x7 & cls_val) << 5);
}

template<>
void CoAPHeader::SetCode<CoAPHeader::Class::METHOD>(CoAPHeader::Method code)
{
  auto method = static_cast<uint8_t>(code);
  m_fixed_hdr.slot[1] |= (0x1F & method);
}

template <>
void CoAPHeader::SetCode<CoAPHeader::Class::SUCCESS>(CoAPHeader::Success code)
{
  auto success = static_cast<uint8_t>(code);
  m_fixed_hdr.slot[1] |= (0x1F & success);
}

uint16_t CoAPHeader::GetMID() const
{
  return (m_fixed_hdr.slot[2] << 8) | m_fixed_hdr.slot[3];
}

void CoAPHeader::SetMID(uint16_t mid)
{
  union split
  {
    uint16_t merged;
    uint8_t slot[2];
  } s_mid;
  s_mid.merged = mid;

  // XXX: Assume this is little endian machine
  m_fixed_hdr.slot[2] = s_mid.slot[1];
  m_fixed_hdr.slot[3] = s_mid.slot[0];
}

uint64_t CoAPHeader::GetToken() const
{
  return m_token;
}

void CoAPHeader::SetToken(uint64_t token)
{
  m_token = token;
}

void
CoAPHeader::PreparePut(CoAPHeader &hdr, uint8_t tkl, uint64_t token, uint16_t mid, bool con)
{
  if (con)
    {
      hdr.SetType(Type::CON);
    }
  else
    {
      hdr.SetType(Type::NON);
    }

  hdr.SetTKL(tkl);
  hdr.SetMID(mid);
  hdr.SetClass(Class::METHOD);
  hdr.SetCode<Class::METHOD>(Method::PUT);
  hdr.SetToken(token);
}
