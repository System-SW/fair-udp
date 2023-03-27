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
#include "coap-header.h"
#include "coap-client.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CoAPClientMethods");

void
CoAPClient::Put ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired());

  CoAPHeader hdr;
  CoAPHeader::PreparePut(hdr, 0, 0, m_mid++);
  Ptr<Packet> packet = Create<Packet>(m_size);
  packet->AddHeader(hdr);

  m_socket->Send(packet);
  m_sendEvent = Simulator::Schedule(Seconds(0.1), &CoAPClient::Put, this);
}

void
CoAPClient::HandlePut(Ptr<Packet> packet, Address addr)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO("got put!");
}
