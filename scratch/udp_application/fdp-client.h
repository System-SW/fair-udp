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
#ifndef FDP_CLIENT_H
#define FDP_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "fair-udp-header.h"

namespace ns3
{
  class Socket;
  class Packet;

  class FdpClient : public Application
  {
  public:
    static TypeId GetTypeId();

    FdpClient();

    virtual ~FdpClient();

    void SetRemote(Address ip, uint16_t port);

    void SetRemote(Address addr);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void Send();
    void HandleRecv(Ptr<Socket> socket);

    // for congestion control
    Time m_min_interval{0};
    Time m_max_interval{0};
    uint32_t m_size{0}; // packet payload size in bytes

    // default 1024 bytes/s (changes during congestion control)
    uint64_t m_bandwidth{1024};
    nack_seq_t m_nack_seq{0};
    sequence_t m_seq{0};

    void ReduceBandwidth();
    Time GetTransferInterval();

    // Ip
    Address m_serverAddress;
    uint16_t m_serverPort{0};

    Ptr<Socket> m_socket{0};
    EventId m_sendEvent{EventId()};
  };
  
}    

#endif /* FDP-CLIENT_H */
