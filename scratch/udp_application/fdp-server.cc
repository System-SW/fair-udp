/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
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
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"
#include "fdp-server.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FdpServer");

NS_OBJECT_ENSURE_REGISTERED (FdpServer);

TypeId FdpServer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FdpServer")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<FdpServer>()
    .AddAttribute("Port",
                  "Server binds to this port",
                  UintegerValue(19574),
                  MakeUintegerAccessor(&FdpServer::m_port),
                  MakeUintegerChecker<uint16_t>());
  return tid;
}

FdpServer::FdpServer()
{
  NS_LOG_FUNCTION (this);
}

FdpServer::~FdpServer()
{
  NS_LOG_FUNCTION (this);
}

void FdpServer::DoDispose()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_port = 0;
  m_connections.clear();

  Application::DoDispose();
}

void FdpServer::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket(GetNode(), tid);
      InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
      if (m_socket->Bind(local) == -1)
        {
          NS_FATAL_ERROR("Failed to bind socket");
        }
    }
  m_socket->SetRecvCallback(MakeCallback(&FdpServer::HandleRecv, this));

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),
                                                     m_port);
      if (m_socket6->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket6->SetRecvCallback (MakeCallback (&FdpServer::HandleRecv, this));
}

void FdpServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket != 0)
    {
      m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

  if (m_socket6 != 0)
    {
      m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void FdpServer::HandleRecv(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom(from)) && packet->GetSize() != 0)
    {
      FairUdpHeader header;
      packet->RemoveHeader(header);
      auto& connection = GetConnection(from);

      // strange methods, if you have no needs to calculate nack frequencies
      // just merge does methods into connection class
      auto feedbackType = connection.DetermineFeedback(header);
      auto feedback = connection.GenerateFeedback(feedbackType, header);
      if (feedback != nullptr)
        {
          socket->SendTo(feedback, 0, from);
        }
    }
}

FdpClientConnection& FdpServer::GetConnection(Address address)
{
  NS_ASSERT(InetSocketAddress::IsMatchingType(address) ||
            Inet6SocketAddress::IsMatchingType(address));
  if (m_connections.find(address) == std::end(m_connections))
    {
      m_connections.emplace(address);
    }
  return *(&m_connections[address]);
}

FdpServer::FeedbackType
FdpClientConnection::DetermineFeedback(FairUdpHeader header) const
{
  if (m_seq == header.GetSequence())
    {
      return FdpServer::FeedbackType::OK;
    }
  else
    {
      return FdpServer::FeedbackType::SAME_NACK;
    }
  return FdpServer::FeedbackType::NEW_NACK;
}

FdpClientConnection::FdpClientConnection(Address address):
  m_address(address)
{
  NS_LOG_FUNCTION (this << address);
}

Ptr<Packet> FdpClientConnection::GenerateFeedback(FdpServer::FeedbackType ft,
                                                  FairUdpHeader header)
{
  switch (ft)
    {
    case FdpServer::FeedbackType::OK:
      m_seq++;
      return {nullptr};
    case FdpServer::FeedbackType::NEW_NACK:
      m_nack_seq++;
      m_seq = header.GetSequence() + 1;
      return MakeNack();
    case FdpServer::FeedbackType::SAME_NACK:
      m_seq = header.GetSequence() + 1;
      return MakeNack();
    default:
      NS_ABORT_MSG("should not reach here");
      break;
  }
}

Ptr<Packet> FdpClientConnection::MakeNack() const
{
  FairUdpHeader header;
  header |= FairUdpHeader::Bit::NACK;
  header.SetNackSequence(m_nack_seq);
  header.SetSequence(m_seq);
  Ptr<Packet> nack = CreateObject<Packet>();
  nack->AddHeader(header);
  return nack;
}

Ptr<Packet> FdpClientConnection::MakeReset() const
{
  FairUdpHeader header;
  header |= FairUdpHeader::Bit::RESET;
  Ptr<Packet> reset = CreateObject<Packet>();
  reset->AddHeader(header);
  return reset;
}
