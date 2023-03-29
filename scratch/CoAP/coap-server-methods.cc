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
#include "coap-server.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CoAPServerMethods");

template <>
void CoAPServer::HandleMethod<CoAPHeader::Method::PUT>
(Ptr<Packet> request, Address addr)
{
  NS_LOG_FUNCTION(this);
  CoAPHeader request_hdr;
  request->RemoveHeader(request_hdr);

  if (request_hdr.GetType() == CoAPHeader::Type::NON)
    {
      NS_LOG_INFO("Receive PUT");
      auto response =
        CoAPHeader::MakeResponse<CoAPHeader::Method::PUT,
                                 false>(request_hdr,
                                        m_mid++,
                                        CoAPHeader::Success::CREATED);
      SendPacket(response, addr);
    }
  else // CON
    {

    }
}
