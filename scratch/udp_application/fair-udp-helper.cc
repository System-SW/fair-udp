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

#include <algorithm>
#include "fair-udp.h"
#include "fair-udp-helper.h"
#include "ns3/inet-socket-address.h"

using namespace ns3;

FairUdpHelper::FairUdpHelper(Address dest):
  dest_(dest)
{
  factory_.SetTypeId(FairUdpApp::GetTypeID());
}

FairUdpHelper::FairUdpHelper(Ipv4Address dest, uint16_t port)
{
  FairUdpHelper(InetSocketAddress(dest, port));
}

ApplicationContainer
FairUdpHelper::Install(Ptr<Node> node) const
{
  return { InstallPriv(node) };
}

ApplicationContainer
FairUdpHelper::Install(NodeContainer nodes) const
{
  ApplicationContainer apps;
  std::for_each(nodes.Begin(), nodes.End(), [&apps, this](auto node)
  {
    apps.Add(InstallPriv(node));
  });

  return apps;
}

Ptr<Application>
FairUdpHelper::InstallPriv(Ptr<Node> node) const
{
  auto app = factory_.Create<FairUdpApp>();
  app->SetDestAddr(dest_);
  node->AddApplication(app);

  return app;
}
