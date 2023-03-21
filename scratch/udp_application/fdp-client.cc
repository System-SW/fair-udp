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
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/random-variable-stream.h"
#include "fdp-client.h"
#include "fair-udp-header.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FdpClient");

NS_OBJECT_ENSURE_REGISTERED (FdpClient);

TypeId FdpClient::GetTypeId ()
{
  static TypeId tid =
    TypeId ("ns3::FdpClient")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<FdpClient> ()
    .AddAttribute ("MinInterval",
                   "Minimum packet transfer interval limit",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&FdpClient::m_min_interval),
                   MakeTimeChecker ())
    .AddAttribute ("MaxInterval",
                   "Maximum packet transfer interval limit",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&FdpClient::m_max_interval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize",
                   "The packet size that this client transfer (bytes)",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&FdpClient::m_size),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the FDP Client",
                   AddressValue (),
                   MakeAddressAccessor (&FdpClient::m_serverAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the FDP Server",
                   UintegerValue (19574),
                   MakeUintegerAccessor (&FdpClient::m_serverPort),
                   MakeUintegerChecker<uint16_t> ());
  return tid;
}

FdpClient::FdpClient ()
{
  NS_LOG_FUNCTION (this);
}

FdpClient::~FdpClient ()
{
  NS_LOG_FUNCTION (this);
}

void
FdpClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_serverAddress = ip;
  m_serverPort = port;
}

void
FdpClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_serverAddress = addr;
}

void
FdpClient::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
FdpClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_serverAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress), m_serverPort));
        }
      else if (Ipv6Address::IsMatchingType(m_serverAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_serverAddress), m_serverPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_serverAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_serverAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_serverAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_serverAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_serverAddress);
        }
    }
  m_socket->SetRecvCallback (MakeCallback(&FdpClient::HandleRecv, this));
  // auto rng = CreateObject<UniformRandomVariable>();
  // m_sendEvent = Simulator::Schedule (MilliSeconds (rng->GetInteger(0, 1000)),
  //                                    &FdpClient::Send, this);
  m_sendEvent = Simulator::Schedule (MilliSeconds (0),
                                     &FdpClient::Send, this);
}

void
FdpClient::StopApplication()
{
  NS_LOG_FUNCTION(this);
  Simulator::Cancel(m_sendEvent);
}

void
FdpClient::Send ()
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_sendEvent.IsExpired());

  // prepare for packet header and contents
  FairUdpHeader header;
  header.SetNackSequence(m_nack_seq);
  header.SetSequence(m_seq++);

  // create packet
  Ptr<Packet> packet = Create<Packet>(m_size + sizeof(uint32_t));
  packet->AddHeader(header);

  if (m_seq == 0)
    {
      if (!m_reset_successed)
        {
          ReduceBandwidth();
        }
      m_reset_successed = false;
    }
  m_socket->Send(packet);
  m_sendEvent = Simulator::Schedule(GetTransferInterval(), &FdpClient::Send, this);
}

void FdpClient::HandleRecv(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom(from)))
    {
      FairUdpHeader header;
      packet->RemoveHeader(header);
      if (header.IsOn<FairUdpHeader::Bit::NACK>())
        {
          NS_LOG_INFO("NACK");
          m_reset_successed = true;
          if (m_nack_seq.get() < header.GetNackSequence().get())
            {
              m_nack_seq = header.GetNackSequence();
              ReduceBandwidth();
            }
          else if (m_nack_seq.get() == header.GetNackSequence().get())
            {
              ReduceBandwidth();
            }
        }
      else if (header.IsOn<FairUdpHeader::Bit::RESET>())
        {
          NS_LOG_INFO("RESET");
          m_reset_successed = true;
        }
    }
}

void FdpClient::ReduceBandwidth()
{
  m_bandwidth /= 2;
  auto min_bandwidth = (m_size / m_max_interval.GetSeconds());
  if (m_bandwidth < min_bandwidth)
    {
      m_bandwidth = min_bandwidth;
    }
}

Time FdpClient::GetTransferInterval()
{
  auto max_bandwidth = (m_size / m_min_interval.GetSeconds());
  auto min_bandwidth = (m_size / m_max_interval.GetSeconds());
  m_bandwidth++;
  if (m_bandwidth < min_bandwidth)
    {
      m_bandwidth = min_bandwidth;
    }
  return Seconds(m_size / std::min(max_bandwidth, static_cast<double>(m_bandwidth)));
}
