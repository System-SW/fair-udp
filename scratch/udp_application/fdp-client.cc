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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
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
          .AddAttribute ("MinInterval", "Minimum packet transfer interval limit", TimeValue (MilliSeconds (100)),
                         MakeTimeAccessor (&FdpClient::m_min_interval), MakeTimeChecker ())
          .AddAttribute ("MaxInterval", "Maximum packet transfer interval limit", TimeValue (Seconds (1.0)),
                         MakeTimeAccessor (&FdpClient::m_max_interval), MakeTimeChecker ())
          .AddAttribute ("PacketSize", "The packet size that this client transfer (KB)", UintegerValue (512),
                         MakeUintegerAccessor (&FdpClient::m_size), MakeUintegerChecker<uint32_t> (1024))
          .AddAttribute ("RemoteAddress", "The destination Address of the FDP Client", AddressValue (),
                         MakeAddressAccessor (&FdpClient::m_serverAddress), MakeAddressChecker ())
          .AddAttribute ("RemotePort", "The destination port of the FDP Server", UintegerValue (19574),
                         MakeUintegerAccessor (&FdpClient::m_serverPort), MakeUintegerChecker<uint16_t> ());
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

#ifdef NS3_LOG_ENABLE
  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_serverAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_serverAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_serverAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_serverAddress);
    }
  else if (InetSocketAddress::IsMatchingType (m_serverAddress))
    {
      peerAddressStringStream << InetSocketAddress::ConvertFrom (m_serverAddress).GetIpv4 ();
    }
  else if (Inet6SocketAddress::IsMatchingType (m_serverAddress))
    {
      peerAddressStringStream << Inet6SocketAddress::ConvertFrom (m_serverAddress).GetIpv6 ();
    }
  m_peerAddressString = peerAddressStringStream.str();
#endif // NS3_LOG_ENABLE

  // XXX: Implement this
  // m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  // m_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &FdpClient::Send, this);
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

  // create packet
  Ptr<Packet> packet = Create<Packet>(m_size + sizeof(uint32_t));
  packet->AddHeader(header);

  m_socket->Send(packet);
  // XXX: add congestion control to dynamically adjust packet transmission interval
  m_sendEvent = Simulator::Schedule(Seconds(10), &FdpClient::Send, this);
}
