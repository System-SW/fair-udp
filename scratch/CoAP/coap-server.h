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
#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "coap-header.h"

namespace ns3
{
  class Socket;
  class Packet;

  class CoAPServer : public Application
  {
  public:
    static TypeId GetTypeId();

    CoAPServer();

    virtual ~CoAPServer();
    
  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;


    void HandleRecv(Ptr<Socket> socket);

    // Methods
    template <CoAPHeader::Method M>
    void HandleMethod(Ptr<Packet> request, Address addr);

    void SendPacket(Ptr<Packet> packet, Address addr);

    // send pong to respond to ping
    void ResponedToPing(CoAPHeader ping_hdr, Address addr);

    // uint32_t m_size{0}; // packet payload size in bytes (for GET)
    uint16_t m_mid{0};     // message id

    uint16_t m_Port{5683};
    Ptr<Socket> m_socket{0};
    Ptr<Socket> m_socket6{0};
  };
}    

#endif /* COAP_SERVER_H */
