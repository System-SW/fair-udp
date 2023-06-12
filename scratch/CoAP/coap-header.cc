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
  m_fixed_hdr.merged = start.ReadU32();

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

CoAPHeader::Type CoAPHeader::GetType() const
{
  return static_cast<CoAPHeader::Type>((0x30 & m_fixed_hdr.slot[0]) >> 4);
}

// use only lower 2bits
void CoAPHeader::SetType(CoAPHeader::Type type)
{
  auto type_val = static_cast<uint8_t>(type);
  m_fixed_hdr.slot[0] &= ~(m_fixed_hdr.slot[0] & (0x3u << 4));
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
CoAPHeader::PreparePut(CoAPHeader &hdr, uint8_t tkl, uint64_t token, uint16_t mid,
                       bool con)
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
  hdr.SetClassAndCode<Class::METHOD>(Method::PUT);
  hdr.SetToken(token);
}

template <>
Ptr<Packet>
CoAPHeader::MakeResponse<CoAPHeader::Method::PUT, false>(CoAPHeader request_hdr,
                                                         uint16_t mid,
                                                         CoAPHeader::Success code)
{
  Ptr<Packet> response = Create<Packet>();
  CoAPHeader hdr;
  auto tkl = request_hdr.GetTKL();
  auto token = request_hdr.GetToken();

  hdr.SetTKL(tkl);
  hdr.SetMID(mid);
  hdr.SetType(CoAPHeader::Type::NON);
  hdr.SetClassAndCode<Class::SUCCESS>(code);
  hdr.SetToken(token);

  response->AddHeader(hdr);
  return response;
}

template <>
Ptr<Packet>
CoAPHeader::MakeResponse<CoAPHeader::Method::PUT, true>(CoAPHeader request_hdr,
                                                         uint16_t mid,
                                                         CoAPHeader::Success code)
{
  Ptr<Packet> response = Create<Packet>();
  CoAPHeader hdr;
  auto tkl = request_hdr.GetTKL();
  auto token = request_hdr.GetToken();

  hdr.SetTKL(tkl);
  hdr.SetMID(mid);
  hdr.SetType(CoAPHeader::Type::CON);
  hdr.SetClass(CoAPHeader::Class::SUCCESS);
  hdr.SetCode<CoAPHeader::Class::SUCCESS>(code);
  hdr.SetToken(token);

  response->AddHeader(hdr);
  return response;
}

CoAPHeader
CoAPHeader::MakePing(uint8_t tkl, uint64_t token)
{
  CoAPHeader hdr;
  hdr.SetType(Type::NON);
  hdr.SetClassAndCode<Class::SIGNAL>(Signal::PING);
  hdr.SetTKL(tkl);
  hdr.SetToken(token);
  return hdr;
}

CoAPHeader
CoAPHeader::MakePong(CoAPHeader ping_hdr)
{
  CoAPHeader hdr;
  hdr.SetType(Type::NON);
  hdr.SetClassAndCode<Class::SIGNAL>(Signal::PONG);
  hdr.SetTKL(ping_hdr.GetTKL());
  hdr.SetToken(ping_hdr.GetToken());
  return hdr;
}
