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

#ifndef UDP_APP_H
#define UDP_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"
#include <unordered_map>


namespace ns3
{
  using port_t = uint16_t;

  // reference from packet-sink.h
  struct AddressHash
  {
    size_t operator() (const Address &x) const
    {
      NS_ABORT_IF(!InetSocketAddress::IsMatchingType(x));
      auto a = InetSocketAddress::ConvertFrom(x);
      return std::hash<uint32_t>()(a.GetIpv4().Get());
    }
  };

  struct connection
  {
    uint16_t sequence_number{0};
  };

  class PacketSource
  {
  public:
    virtual Ptr<Packet> GetPacket() = 0;
    virtual ~PacketSource() {}
  };

  class CongestionInfo
  {
  public:
    CongestionInfo();
    CongestionInfo(uint64_t msg_size);
    void PacketDropDetected();
    uint64_t GetTransferInterval();
  private:
    uint64_t bandwidth_{1};     // 1 kb
    uint64_t msg_size_{1024};   // 1 kb
  };

  class FairUdpApp : public Application
  {
  public:
    FairUdpApp();
    virtual ~FairUdpApp();

    static TypeId GetTypeID();
    TypeId GetInstanceTypeId() const override;

    void SendStream(PacketSource *in);
    void SetDestAddr(Address dest);
  private:
    void ReceiveHandler(Ptr<Socket> socket);
    void SetupReceiveSocket(port_t port);
    void StartApplication() override;
    void SendNACK(Address dest);
    void SendMsg(Ptr<Packet> packet);

    Ptr<Socket> socket_;
    port_t port_;
    uint16_t seq_number_{0};
    Address dest_;
    std::unordered_map<Address, connection, AddressHash> connections_;
    CongestionInfo congestion_info_;
  };
  
} // namespace ns3

#endif /* UDP_APP_H */
