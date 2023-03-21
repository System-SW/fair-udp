/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
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
#ifndef FDP_SERVER_H
#define FDP_SERVER_H

#include <map>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "sequence_util.h"
#include "fair-udp-header.h"

namespace ns3
{
  class Socket;
  class FdpClientConnection;

  class FdpServer : public Application
  {
  public:
    static TypeId GetTypeId ();
    FdpServer ();
    ~FdpServer () override;

    enum class FeedbackType
      {
        OK,
        SAME_NACK,
        NEW_NACK,
      };

  protected:
    void DoDispose () override;

  private:
    void StartApplication () override;
    void StopApplication () override;


    void HandleRecv (Ptr<Socket> socket);

    uint16_t m_port{0}; // Server Port Number that it binds to
    Ptr<Socket> m_socket{0};
    Ptr<Socket> m_socket6{0};

    std::map<Address, FdpClientConnection> m_connections;

    FdpClientConnection& GetConnection(Address address);

    // XXX: Needs to add some Per Client Statistics
  };

  class FdpClientConnection
  {
  private:
    const Address m_address; // only for InetSocketaddress or Inet6SocketAddress
    sequence_t m_seq{0};
    nack_seq_t m_nack_seq{0};
  public:
    FdpClientConnection(Address address);
    FdpServer::FeedbackType DetermineFeedback(FairUdpHeader header) const;

    // send nack or reset... ect
    Ptr<Packet> GenerateFeedback(FdpServer::FeedbackType ft, FairUdpHeader header);
  private:
    /* for client feedback */
    Ptr<Packet> MakeNack() const;
    Ptr<Packet> MakeReset() const;
  };
}


#endif /* FDP-SERVER_H */
