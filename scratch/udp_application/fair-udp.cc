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
#include "fair-udp.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/ipv4-header.h"
#include "config.h"
#include <fstream>
#include <algorithm>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FairUdpApp");
NS_OBJECT_ENSURE_REGISTERED(FairUdpApp);

static bool
ValidateSequence(sequence_t my_sequence, FairUdpHeader header)
{
  if ((my_sequence + header.GetSequence()) % 2 == 0)
    {
      if (my_sequence != header.GetSequence())
        {
          return false;
        }
      else
        {
          // Expired Packet
          return true;
        }
    }
  return true;
}

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
FairUdpApp::SetupReceiveSocket(port_t port)
{
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
  if (socket_->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
}

void
FairUdpApp::StartApplication()    
{
  auto tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  socket_ = Socket::CreateSocket(GetNode(), tid);

  SetupReceiveSocket(port_);
  socket_->SetRecvCallback(MakeCallback(&FairUdpApp::ReceiveHandler, this));
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

      if (header.IsOn<FairUdpHeader::Bit::NACK>()) // client side
        {
          // reset my sequence number to the requested number
          if ((seq_number_ + header.GetSequence()) % 2)
            {
              seq_number_ = header.GetSequence();
              congestion_info_.PacketDropDetected(seq_number_);
            }
        }
      else if (header.IsOn<FairUdpHeader::Bit::RESET>()) // server side
        {
          connections_[from].sequence_number = 0;
        }
      else  // handle received message (server side)
        {
          if (connections_.find(from) == connections_.end())
            {
              NS_LOG_DEBUG("Connected");
            }

          if (ValidateSequence(connections_[from].sequence_number, header))
            {
              connections_[from].sequence_number += 2;
            }
          else
            {
              connections_[from].sequence_number++;
              SendNACK(from);
            }
        }
    }
}

void
FairUdpApp::SendMsg(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this << packet << InetSocketAddress::ConvertFrom(dest_).GetIpv4());

  FairUdpHeader header;
  header.SetSequence(seq_number_);
  seq_number_ += 2;
  packet->AddHeader(header);

  socket_->SendTo(packet, 0, InetSocketAddress::ConvertFrom(dest_));
}

void
FairUdpApp::SendNACK(Address dest)
{
  auto packet = Create<Packet>();

  FairUdpHeader header;
  header.SetSequence(connections_[dest].sequence_number);
  header |= FairUdpHeader::Bit::NACK;
  packet->AddHeader(header);

  NS_ABORT_IF(!InetSocketAddress::IsMatchingType(dest));
  auto ipv4_address = InetSocketAddress::ConvertFrom(dest);
  NS_LOG_INFO(this << packet << ipv4_address.GetIpv4());
  socket_->SendTo(packet, 0, ipv4_address);
}

void
FairUdpApp::SetDestAddr(Address dest)
{
  NS_ABORT_IF(!InetSocketAddress::IsMatchingType(dest));
  dest_ = dest;
}

void
FairUdpApp::SendStream(PacketSource* in)
{
  SendMsg(in->GetPacket());
  auto interval = congestion_info_.GetTransferInterval();
  Simulator::Schedule(MilliSeconds(interval), &FairUdpApp::SendStream, this, in);
}
