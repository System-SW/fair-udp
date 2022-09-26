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

#ifndef FUDP_CLIENT_HELPER_H
#define FUDP_CLIENT_HELPER_H

#include <memory>
#include <stdint.h>

#include "ns3/application-container.h"
#include "ns3/application.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include "fudp-application.h"
#include "fudp-client.h"

template <FudpFeature FEATURES>
class FudpClientHelper
{
public:
  FudpClientHelper (::ns3::Address const &dest);

  FudpClientHelper (::ns3::Ipv4Address dest, u16 port);

  ::ns3::ApplicationContainer Install (::ns3::Ptr<::ns3::Node> node) const;

  ::ns3::ApplicationContainer Install (::ns3::NodeContainer nodes) const;

private:
  ::ns3::Ptr<::ns3::Application> InstallOne (::ns3::Ptr<::ns3::Node> node) const;

  ::ns3::ObjectFactory _factory;

  ::ns3::Address _serverAddress;
};

template <FudpFeature FEATURES>
FudpClientHelper<FEATURES>::FudpClientHelper (::ns3::Address const &dest) : _serverAddress{dest}
{
  _factory.SetTypeId(FudpApplication::GetTypeId());
}

template <FudpFeature FEATURES>
FudpClientHelper<FEATURES>::FudpClientHelper (::ns3::Ipv4Address dest, u16 port)
    : FudpClientHelper{::ns3::InetSocketAddress{dest, port}}
{
}

template <FudpFeature FEATURES>
::ns3::ApplicationContainer FudpClientHelper<FEATURES>::Install (::ns3::Ptr<::ns3::Node> node) const
{
  return {InstallOne (node)};
}

template <FudpFeature FEATURES>
::ns3::ApplicationContainer FudpClientHelper<FEATURES>::Install (::ns3::NodeContainer nodes) const
{
  auto fudpApps = ::ns3::ApplicationContainer{};
  for (auto iter = nodes.Begin (); iter != nodes.End (); ++iter)
    {
      fudpApps.Add (InstallOne (*iter));
    }

  return fudpApps;
}

template <FudpFeature FEATURES>
::ns3::Ptr<::ns3::Application> FudpClientHelper<FEATURES>::InstallOne (::ns3::Ptr<::ns3::Node> node) const
{
  auto fudpClient = ::std::make_shared<FudpClient<FEATURES>> ();
  fudpClient->SetServerAddress (_serverAddress);
  fudpClient->SetServerPort(::ns3::InetSocketAddress::ConvertFrom(_serverAddress).GetPort());

  auto fudpApp = _factory.Create<FudpApplication> ();
  fudpApp->SetImpl (fudpClient);

  node->AddApplication (fudpApp);

  return fudpApp;
}

#endif
