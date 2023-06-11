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
#pragma once
#ifndef COAP_NODE_H
#define COAP_NODE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "coap-header.h"
#include "cocoa.h"


namespace ns3
{
  class Socket;
  class Packet;

  class CoAPClient : public Application
  {
  public:
    static TypeId GetTypeId();

    CoAPClient();

    virtual ~CoAPClient();
    
    void SetRemote(Address ip, uint16_t port);

    void SetRemote(Address addr);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;


    void HandleRecv(Ptr<Socket> socket);

    // Methods
    void Put();

    template <CoAPHeader::Success Response>
    void HandleResponse(Ptr<Packet> response, Address addr);

    void SendPing(uint64_t token); // start ping-pong signaling

    Time MeasureRTTWithPingPong(CoAPHeader pong_hdr);

    void SendPacket(Ptr<Packet> packet) const;

    uint32_t m_size{0}; // packet payload size in bytes (for PUT)
    uint16_t m_mid{0};  // message id

    Time m_ping_time{0};        // ping send time
    Time m_Rtt{0};              // measured actual RTT

    // Ip
    Address m_Address;
    uint16_t m_Port{0};

    Ptr<Socket> m_socket{0};
    EventId m_sendEvent{EventId()};

    // CoCoA Congestion Controller
    CoCoA m_CongestionController{MakeCallback(&CoAPClient::SendPacket, this)};
  };

}    

#endif /* COAP_NODE_H */
