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

NS_LOG_COMPONENT_DEFINE ("CoAPClientMethods");

void
CoAPClient::Put ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired());

  // XXX: fdp may control CoAP Message Transfer.
  // 1. Normal Message Transfer
  //    if the message sequence number is 0 or 1, then it is normal message
  //    1) Record Message Transfer Interval into message
  //    2) Normal Message Transfer Interval is measured RTT
  //    3) if you receive FDP feedback from server, then recalculate RTT and RTO
  // 
  // 2. RESET Procedure
  //    if the message sequence number is 2, then it is the reset message
  //    After transfering the reset message, the client has to wait for reset feedback at least RTO.
  //    1) the client receives the reset feedback before RTT
  //       update RTT with measured value.
  //       note: this case is the only case that the fdp allows the client to reduce its transfer interval.
  // 
  //    2) the client receives the reset feedback after RTT but before RTO
  //       update RTT with measured value.
  // 
  //    3) the client fails to receive the reset feedback within RTO
  //       set RTO as RTT value.

  CoAPHeader hdr;
  CoAPHeader::PreparePut(hdr, 0, 0, m_mid++);
  Ptr<Packet> packet = Create<Packet>(m_size);
  packet->AddHeader(hdr);

  // XXX: FDP congestion controller takes control of send message.
  // m_socket->Send(packet);
  // m_sendEvent = Simulator::Schedule(Seconds(0.1), &CoAPClient::Put, this);
  m_sendEvent = m_CongestionController.TransferMessage(m_socket, packet, MakeCallback(&CoAPClient::Put, this));
}

template <>
void CoAPClient::HandleResponse<CoAPHeader::Success::CREATED>
(Ptr<Packet> response, Address addr)
{
  NS_LOG_INFO("Created!");
  CoAPHeader hdr;
  response->PeekHeader(hdr);

  if (hdr.GetType() == CoAPHeader::Type::NON)
    {
      NS_LOG_INFO("Put Result " << hdr);
    }
  else  // CON
    {

    }
}

void
CoAPClient::SendPing(uint64_t token)
{
  NS_LOG_FUNCTION(this);
  // XXX: hard coded token
  auto ping_hdr = CoAPHeader::MakePing(2, 0x1234);
  Ptr<Packet> ping = Create<Packet>();
  ping->AddHeader(ping_hdr);
  m_ping_time = Simulator::Now();
  m_socket->Send(ping);
}

Time
CoAPClient::MeasureRTTWithPingPong(CoAPHeader pong_hdr)
{
  NS_LOG_FUNCTION(this);
  m_Rtt = Simulator::Now() - m_ping_time;
  m_ping_time = Time{0};
  NS_LOG_INFO("ping pong done " << m_Rtt);
  return m_Rtt;
}
