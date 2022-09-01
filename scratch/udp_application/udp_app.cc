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
#include "udp_app.hh"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("simple_udp_app");
NS_OBJECT_ENSURE_REGISTERED(simple_udp_app);

TypeId
simple_udp_app::GetTypeID()
{
  static TypeId tid = TypeId("ns3::simple_udp_app")
    .AddConstructor<simple_udp_app>()
    .SetParent<Application>();

  return tid;
}

TypeId
simple_udp_app::GetInstanceTypeId() const
{
  return simple_udp_app::GetTypeID();
}

simple_udp_app::simple_udp_app()
{
  port_ = 7777;
}

simple_udp_app::~simple_udp_app() {}

void
simple_udp_app::setup_receive_socket(Ptr<Socket> socket, port_t port)    
{
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
  if (socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
}

void
simple_udp_app::StartApplication()    
{
  auto tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  socket_ = Socket::CreateSocket(GetNode(), tid);

  setup_receive_socket(socket_, port_);

  socket_->SetRecvCallback(MakeCallback(&simple_udp_app::receive_handler, this));

  socket_ = Socket::CreateSocket(GetNode(), tid);
}

void
simple_udp_app::receive_handler(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  Address from;

  while (auto packet = socket->RecvFrom(from))
    {
      NS_LOG_INFO("Handle message (size): " << packet->GetSize()
                  << " at time " << Now().GetSeconds());
      NS_LOG_INFO(packet->ToString());
    }
}

void
simple_udp_app::send_msg(Ptr<Packet> packet, Ipv4Address dest, port_t port)
{
  NS_LOG_FUNCTION(this << packet << dest << port);
  // socket_->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(dest), port));
  // socket_->Send(packet);
  socket_->SendTo(packet, 0, dest);

  Simulator::Schedule(MilliSeconds(100), &simple_udp_app::send_msg, this, packet,
                      dest, port);
}
