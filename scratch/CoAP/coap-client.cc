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
#include "option.h"
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
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("MsgInterval",
                     "notify msg transfer interval.",
                     MakeTraceSourceAccessor(&CoAPClient::m_MsgIntervalCallback),
                     "ns3::CoAPClient::MsgIntervalCB")
    .AddTraceSource("MsgTransfer",
                    "notify msg transfer.",
                    MakeTraceSourceAccessor(&CoAPClient::m_TransferCallback),
                    "ns3::CoAPClient::TransferPacketCB")
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
  m_socket->Close();
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
CoAPClient::NotifyMsgInterval()
{
  Time rtt;
  if (UseFDP)
    {
      rtt = m_FdpCC.GetRTO();
    }
  else
    {
      rtt = m_CoCoACC.GetRTO();
    }

  m_MsgIntervalCallback(rtt);
  if (!Simulator::IsFinished())
    {
      Simulator::Schedule(Seconds(1),
                          MakeCallback(&CoAPClient::NotifyMsgInterval, this));
    }
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


  m_socket->SetRecvCallback(MakeCallback(&CoAPClient::HandleRecv, this));
  m_sendEvent = Simulator::Schedule(Seconds(0.1), &CoAPClient::Put, this);
  NotifyMsgInterval();
  // Simulator::Schedule(Seconds(0.1), &CoAPClient::SendPing, this, 0x1234);
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
      p->PeekHeader(hdr);

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
          break;
        case Class::SIGNAL:
          using Signal = CoAPHeader::Signal;
          switch (hdr.GetCode<Class::SIGNAL>())
            {
            case Signal::PONG:
              MeasureRTTWithPingPong(hdr);
              break;
            case Signal::UNASSIGNED:
              NS_LOG_INFO("Handle FDP Feedback.");
              if (UseFDP)
                {
                  p->RemoveHeader(hdr); // remove CoAP header
                  m_FdpCC.HandleFeedback(p);
                  if (m_sendEvent.IsExpired())
                    {
                      m_sendEvent =
                        m_FdpCC.ScheduleTransfer(MakeCallback(&CoAPClient::Put, this));
                    }
                }
              break;
            default:
              NS_ABORT_MSG("Not Implemented CoAP Classes.");
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
CoAPClient::SendPacket(Ptr<Packet> packet) const
{
  m_socket->Send(packet);
}

void
CoAPClient::NotifyPacketTransmission(Ptr<const Packet> p)
{
  m_TransferCallback(p);
}
