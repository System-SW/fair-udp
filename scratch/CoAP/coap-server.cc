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
#include "coap-server.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CoAPServer");
NS_OBJECT_ENSURE_REGISTERED(CoAPServer);

TypeId CoAPServer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::CoAPServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<CoAPServer>()
    .AddAttribute ("RemotePort",
                   "The destination port of the CoAP Client",
                   UintegerValue (5683),
                   MakeUintegerAccessor (&CoAPServer::m_Port),
                   MakeUintegerChecker<uint16_t> ());
    ;
  return tid;
}

CoAPServer::CoAPServer ()
{
  NS_LOG_FUNCTION (this);
}

CoAPServer::~CoAPServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket->Close();
  m_socket6->Close();
}

void
CoAPServer::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
CoAPServer::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket(GetNode(), tid);
      InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_Port);
      if (m_socket->Bind(local) == -1)
        {
          NS_FATAL_ERROR("Failed to bind socket");
        }
    }
  m_socket->SetRecvCallback(MakeCallback(&CoAPServer::HandleRecv, this));

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),
                                                     m_Port);
      if (m_socket6->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket6->SetRecvCallback (MakeCallback (&CoAPServer::HandleRecv, this));
}

void
CoAPServer::StopApplication()
{
  NS_LOG_FUNCTION (this);
  m_socket->Close();
  m_socket6->Close();
}

void CoAPServer::HandleRecv(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this);

  Address addr;

  while (Ptr<Packet> p = socket->RecvFrom(addr))
    {
      CoAPHeader hdr;
      p->PeekHeader(hdr);

      switch (hdr.GetClass())
        {
          using Class = CoAPHeader::Class;
        case Class::METHOD:
          using Method = CoAPHeader::Method;
          switch (hdr.GetCode<Class::METHOD>())
            {
            case Method::PUT:
              NS_LOG_INFO(this << static_cast<uint8_t>(Method::PUT));
              HandleMethod<Method::PUT>(p, addr);
              break;
            default:
              NS_ABORT_MSG("Not Implemented CoAP Success Code.");
              break;
            }
          break;
        default:
          NS_ABORT_MSG("Not Implemented CoAP Classes.");
          break;
        }
    }

}

void
CoAPServer::SendPacket(Ptr<Packet> packet, Address addr)
{
  NS_LOG_FUNCTION(this);

  if (InetSocketAddress::IsMatchingType (addr))
    {
      m_socket->SendTo(packet, 0, addr);
    }
  else if (Inet6SocketAddress::IsMatchingType (addr))
    {
      m_socket6->SendTo(packet, 0, addr);
    }
  else
    {
      NS_ABORT_MSG(this << "SendPacket Method requires InetSocketAddress.");
    }
}
