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
#include "fdp-client-server-helper.h"
#include "fdp-server.h"
#include "fdp-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

using namespace ns3;

FdpServerHelper::FdpServerHelper ()
{
  m_factory.SetTypeId(FdpServer::GetTypeId());
}

FdpServerHelper::FdpServerHelper(uint16_t port):
  FdpServerHelper()
{
  SetAttribute("Port", UintegerValue(port));
}

void
FdpServerHelper::SetAttribute(std::string name, const AttributeValue &value)
{
  m_factory.Set(name, value);
}

ApplicationContainer
FdpServerHelper::Install(Ptr<Node> node) const
{
  return { InstallPriv(node) };
}

ApplicationContainer
FdpServerHelper::Install(NodeContainer c) const
{
  ApplicationContainer apps;
  std::for_each(c.Begin(), c.End(), [&apps, this](auto node)
  {
    InstallPriv(node);
  });

  return apps;
}

Ptr<Application>
FdpServerHelper::InstallPriv(Ptr<Node> node) const
{
  auto app = m_factory.Create<FdpServerHelper>();
  node->AddApplication(app);
  return app;
}

FdpClientHelper::FdpClientHelper ()
{
  m_factory.SetTypeId(FdpClient::GetTypeId());
}

FdpClientHelper::FdpClientHelper (Address ip, uint16_t port):
  FdpClientHelper()
{
  SetAttribute("RemoteAddress", AddressValue(ip));
  SetAttribute("RemotePort", UintegerValue(port));
}

FdpClientHelper::FdpClientHelper (Address address):
  FdpClientHelper()
{
  SetAttribute("RemoteAddress", AddressValue(address));
}

void
FdpClientHelper::SetAttribute(std::string name, const AttributeValue &value)
{
  m_factory.Set(name, value);
}

ApplicationContainer
FdpClientHelper::Install(Ptr<Node> node) const
{
  return { InstallPriv(node) };
}

ApplicationContainer
FdpClientHelper::Install(NodeContainer c) const
{
  ApplicationContainer apps;
  std::for_each(apps.Begin(), apps.End(), [&apps, this](auto node)
  {
    InstallPriv(node);
  });

  return apps;
}

Ptr<Application>
FdpClientHelper::InstallPriv(Ptr<Node> node) const
{
  auto app = m_factory.Create<FdpClientHelper>();
  node->AddApplication(app);
  return app;
}
