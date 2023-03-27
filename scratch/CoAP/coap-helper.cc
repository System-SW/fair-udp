/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#include <algorithm>
#include <utility>
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "coap-helper.h"
#include "coap-client.h"

using namespace ns3;

CoAPHelper::CoAPHelper ()
{
  m_factory.SetTypeId(CoAPClient::GetTypeId());
}

CoAPHelper::CoAPHelper (Address ip, uint16_t port):
  CoAPHelper{}
{
  SetAttribute("RemoteAddress", AddressValue(ip));
  SetAttribute("RemotePort", UintegerValue(port));
}

CoAPHelper::CoAPHelper (Address addr):
  CoAPHelper{}
{
  SetAttribute("RemoteAddress", AddressValue(addr));
}    

void
CoAPHelper::SetAttribute(std::string name, const AttributeValue &value)
{
  m_factory.Set(name, value);
}

ApplicationContainer
CoAPHelper::Install(Ptr<Node> node) const
{
  return { InstallPriv(node) };
}

ApplicationContainer
CoAPHelper::Install(NodeContainer c) const
{
  ApplicationContainer apps;
  std::for_each(c.Begin(), c.End(), [&apps, this](auto node)
  {
    apps.Add(InstallPriv(node));
  });

  return apps;
}

Ptr<Application>
CoAPHelper::InstallPriv(Ptr<Node> node) const
{
  auto app = m_factory.Create<CoAPClient>();
  node->AddApplication(app);
  return app;
}
