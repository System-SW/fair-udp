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
 * Author: Chang-Hui Kim <kch9001@gmail.com>, Daeseong Ki <kmeos1579@gmail.com>
 */

#ifndef FUDP_SERVER_HELPER_H
#define FUDP_SERVER_HELPER_H

#include <memory>
#include <stdint.h>

#include "ns3/application-container.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include "fudp-application.h"
#include "fudp-server.h"
#include "types.h"

template <FudpFeature FEATURES>
class FudpServerHelper
{
public:
  FudpServerHelper ();

  ::ns3::ApplicationContainer Install (::ns3::Ptr<::ns3::Node> node) const;

  ::ns3::ApplicationContainer Install (::ns3::NodeContainer nodes) const;

private:
  ::ns3::Ptr<::ns3::Application> InstallOne (::ns3::Ptr<::ns3::Node> node) const;

  ::ns3::ObjectFactory _factory;

  u16 _serverPort;
};

template <FudpFeature FEATURES>
FudpServerHelper<FEATURES>::FudpServerHelper () : _serverPort{}
{
  _factory.SetTypeId(FudpApplication::GetTypeId());
}

template <FudpFeature FEATURES>
::ns3::ApplicationContainer FudpServerHelper<FEATURES>::Install (::ns3::Ptr<::ns3::Node> node) const
{
  return {InstallOne (node)};
}

template <FudpFeature FEATURES>
::ns3::ApplicationContainer FudpServerHelper<FEATURES>::Install (::ns3::NodeContainer nodes) const
{
  auto fudpApps = ::ns3::ApplicationContainer{};
  for (auto iter = nodes.Begin (); iter != nodes.End (); ++iter)
    {
      fudpApps.Add (InstallOne (*iter));
    }

  return fudpApps;
}

template <FudpFeature FEATURES>
::ns3::Ptr<::ns3::Application> FudpServerHelper<FEATURES>::InstallOne (::ns3::Ptr<::ns3::Node> node) const
{
  auto fudpServer = ::std::make_shared<FudpServer<FEATURES>> ();
  fudpServer->SetServerPort(7777);

  auto fudpApp = _factory.Create<FudpApplication> ();
  fudpApp->SetImpl (fudpServer);

  node->AddApplication (fudpApp);

  return fudpApp;
}

#endif
