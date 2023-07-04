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
#pragma once
#ifndef FDP_CLIENT_SERVER_HELPER_H
#define FDP_CLIENT_SERVER_HELPER_H

#include <cstdint>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "fdp-client.h"
#include "fdp-server.h"

namespace ns3
{
  class FdpServerHelper
  {
  public:
    FdpServerHelper();

    FdpServerHelper(uint16_t port);

    void SetAttribute(std::string name, const AttributeValue &value);

    ApplicationContainer Install(Ptr<Node> node) const;
    ApplicationContainer Install(NodeContainer c) const;

  private:
    Ptr<Application> InstallPriv(Ptr<Node> node) const;
    ObjectFactory m_factory;
  };

  class FdpClientHelper
  {
  public:
    FdpClientHelper();

    FdpClientHelper(Address ip, uint16_t port);

    FdpClientHelper(Address addr);

    void SetAttribute(std::string name, const AttributeValue &value);

    ApplicationContainer Install(Ptr<Node> node) const;
    ApplicationContainer Install(NodeContainer c) const;

  private:
    Ptr<Application> InstallPriv(Ptr<Node> node) const;
    ObjectFactory m_factory;
  };
}    

#endif /* FDP_CLIENT_SERVER_HELPER_H */
