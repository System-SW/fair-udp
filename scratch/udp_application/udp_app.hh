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

#ifndef UDP_APP_H
#define UDP_APP_H

#include "ns3/socket.h"
#include "ns3/application.h"


namespace ns3
{
  using port_t = uint16_t;

  class simple_udp_app : public Application
  {
  public:
    simple_udp_app();
    virtual ~simple_udp_app();

    static TypeId GetTypeID();
    virtual TypeId GetInstanceTypeId() const override;

    void receive_handler(Ptr<Socket> socket);
    void send_msg(Ptr<Packet> packet, Ipv4Address dest, port_t port);

  private:
    void setup_receive_socket(Ptr<Socket> socket, port_t port);
    virtual void StartApplication() override;

    Ptr<Socket> socket_;
    port_t port_;
  };
  
} // namespace ns3

#endif /* UDP_APP_H */
