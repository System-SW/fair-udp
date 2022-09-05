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
 */

#include "ns3/log.h"
#include "fair-udp.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/ipv4-header.h"
#include "fair-udp-header.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FairUdpApp");
NS_OBJECT_ENSURE_REGISTERED(FairUdpApp);

TypeId
FairUdpApp::GetTypeID()
{
  static TypeId tid = TypeId("ns3::FairUdpApp")
    .AddConstructor<FairUdpApp>()
    .SetParent<Application>();

  return tid;
}

TypeId
FairUdpApp::GetInstanceTypeId() const
{
  return FairUdpApp::GetTypeID();
}

FairUdpApp::FairUdpApp()
{
  port_ = 7777;
}

FairUdpApp::~FairUdpApp()
{
  socket_->Close();
}

void
FairUdpApp::SetupReceiveSocket(Ptr<Socket> socket, port_t port)    
{
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
  if (socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
}

void
FairUdpApp::StartApplication()    
{
  auto tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  socket_ = Socket::CreateSocket(GetNode(), tid);

  SetupReceiveSocket(socket_, port_);

  socket_->SetRecvCallback(MakeCallback(&FairUdpApp::ReceiveHandler, this));

  socket_ = Socket::CreateSocket(GetNode(), tid);
}

void
FairUdpApp::ReceiveHandler(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  Address from;

  if (auto packet = socket->RecvFrom(from))
    {
      FairUdpHeader header;
      packet->RemoveHeader(header);

      NS_LOG_INFO("Handle message (size): " << packet->GetSize()
                  << " Sequence Number: " << header.GetData()
                  << " at time " << Now().GetSeconds());
      NS_LOG_INFO(packet->ToString());
    }
}

void
FairUdpApp::SendMsg(Ptr<Packet> packet, Ipv4Address dest, port_t port)
{
  NS_LOG_FUNCTION(this << packet << dest << port);
  FairUdpHeader header;
  header.SetData(seq_number_++);
  packet->AddHeader(header);

  socket_->SendTo(packet, 0, InetSocketAddress(dest, port));
}
