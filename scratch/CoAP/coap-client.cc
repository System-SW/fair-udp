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
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "coap-client.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CoAPClient");

NS_OBJECT_ENSURE_REGISTERED (CoAPClient);

TypeId CoAPClient::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::CoAPClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<CoAPClient>()
    .AddAttribute ("PacketSize",
                   "The packet size that this client transfer (bytes)",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CoAPClient::m_size),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the CoAP Client",
                   AddressValue (),
                   MakeAddressAccessor (&CoAPClient::m_Address),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the CoAP Client",
                   UintegerValue (5683),
                   MakeUintegerAccessor (&CoAPClient::m_Port),
                   MakeUintegerChecker<uint16_t> ());
    ;
  return tid;
}

CoAPClient::CoAPClient ()
{
  NS_LOG_FUNCTION (this);
}

CoAPClient::~CoAPClient ()
{
  NS_LOG_FUNCTION (this);
}

void
CoAPClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_Address = ip;
  m_Port = port;
}

void
CoAPClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_Address = addr;
}

void
CoAPClient::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
CoAPClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_Address) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_Address), m_Port));
        }
      else if (Ipv6Address::IsMatchingType(m_Address) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_Address), m_Port));
        }
      else if (InetSocketAddress::IsMatchingType (m_Address) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_Address);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_Address) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_Address);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_Address);
        }
    }


  m_sendEvent = Simulator::Schedule(Seconds(0.1), &CoAPClient::Put, this);
}

void
CoAPClient::StopApplication()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel(m_sendEvent);
}

void CoAPClient::HandleRecv(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this);

  Address addr;

  while (Ptr<Packet> p = socket->RecvFrom(addr))
    {
      CoAPHeader hdr;
      p-> PeekHeader(hdr);

      switch (hdr.GetClass())
        {
          using Class = CoAPHeader::Class;
        case Class::SUCCESS:
          using Success = CoAPHeader::Success;
          switch (hdr.GetCode<Class::SUCCESS>())
            {
            case Success::CREATED:
              HandleResponse<Success::CREATED>(p, addr);
              break;
            default:
              NS_ABORT_MSG("Not Implemented CoAP Success Code.");
              break;
            }
        default:
          NS_ABORT_MSG("Not Implemented CoAP Classes.");
          break;
        }
    }

}
