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
#include <string>
#include <tuple>
#include <algorithm>
#include "ns3/core-module.h"
#include "ns3/nstime.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "coap-helper.h"
#include "tests.h"

using namespace ns3;
using namespace std::string_literals;

enum GroundNodes
  {
    GC = 0,
    AP = 1,
  };

struct WifiTestArgs
{
  Time simulationTime;

};

static auto SERVER_BANDWIDTH = "1000Mbps"s;
static std::size_t NUM_UAVS = 40;
static Time SIMUL_TIME = Seconds(30);

static NetDeviceContainer
InstallP2P(NodeContainer& Nodes)
{
  auto p2pHelper = PointToPointHelper{};
  p2pHelper.SetDeviceAttribute("DataRate", StringValue(SERVER_BANDWIDTH));
  p2pHelper.SetChannelAttribute("Delay", StringValue("1ms"));
  return p2pHelper.Install(Nodes);
}

static std::tuple<NetDeviceContainer, NetDeviceContainer>
SettingWifi(Ptr<Node> apNode, NodeContainer& staNodes)
{
  auto channel = YansWifiChannelHelper::Default();
  auto phy = YansWifiPhyHelper();
  phy.SetChannel(channel.Create());

  WifiMacHelper mac;
  auto ssid = Ssid("ns-3-ssid");

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ac);
  mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing",
              BooleanValue(false));
  auto staDevices = wifi.Install(phy, mac, staNodes);

  mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
  auto apDevices = wifi.Install(phy, mac, apNode);

  // generate pcap file
  phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.EnablePcap("coap-fdp", apDevices.Get(0));

  return {apDevices, staDevices};
}

static void
AllocatePositionsForDrones(NodeContainer& staNodes)
{
  // for drones
  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator", "rho",
                                DoubleValue(30), "X", DoubleValue(50),
                                "Y", DoubleValue(50));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(staNodes);
}


static void
AllocatePositionForAP(Ptr<Node> apNode)
{
  // for ground nodes
  MobilityHelper mobility;
  auto position_alloc = CreateObject<ListPositionAllocator>();
  position_alloc->Add(Vector (50, 50, 0));
  mobility.SetPositionAllocator(position_alloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(apNode);
}

static void
InstallInternetStack(NodeContainer& nodes)
{
  InternetStackHelper stack;
  stack.Install(nodes);
}

[[nodiscard]] static ApplicationContainer
InstallCoAPServer(Ptr<Node> server, Time start = Seconds(0), Time end = SIMUL_TIME)
{
  CoAPServerHelper installer;
  installer.SetAttribute("RemotePort", UintegerValue(19574));
  auto server_app = installer.Install(server);
  server_app.Start(start);
  server_app.Stop(end);
  return server_app;
}

[[nodiscard]] static ApplicationContainer
InstallCoAPClient(NodeContainer &clients, Address dest, Time start = Seconds(0.1),
                  Time end = SIMUL_TIME)
{
  CoAPClientHelper installer{dest};
  auto client_app = installer.Install(clients);
  client_app.Start(start);
  client_app.Stop(end);
  return client_app;
}

[[nodiscard]] static ApplicationContainer
InstallTcpSink(Ptr<Node> server, Address serverAddr, Time start = Seconds(0),
               Time end = SIMUL_TIME)
{
  PacketSinkHelper helper{"ns3::TcpSocketFactory", serverAddr};
  auto app = helper.Install(server);
  app.Start(start);
  app.Stop(end);
  return app;
}

[[nodiscard]] static ApplicationContainer
InstallTcpOnOff(NodeContainer &nodes, Address dest, Time start = Seconds(1),
                Time end = SIMUL_TIME)    
{
  OnOffHelper helper{"ns3::TcpSocketFactory", dest};
  helper.SetAttribute("Remote", AddressValue(dest));
  helper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=10]"));
  helper.SetAttribute("OffTime",
                      StringValue("ns3::ConstantRandomVariable[Constant=10]"));
  auto app = helper.Install(nodes);
  app.Start(start);
  app.Stop(end);
  return app;
}


void WifiTest()
{
  // wired part
  auto p2pNodes = NodeContainer{2};
  auto p2pDevices = InstallP2P(p2pNodes);
  auto wifiStaNodes = NodeContainer(NUM_UAVS);

  auto [apDevices, staDevices] = SettingWifi(p2pNodes.Get(GroundNodes::AP),
                                             wifiStaNodes);
  AllocatePositionsForDrones(wifiStaNodes);
  AllocatePositionForAP(p2pNodes.Get(GroundNodes::AP));

  InstallInternetStack(p2pNodes);
  InstallInternetStack(wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase("10.1.1.0", "255.255.255.0");
  auto p2pInterfaces = address.Assign(p2pDevices);

  address.SetBase("10.1.3.0", "255.255.255.0");
  address.Assign(staDevices);
  address.Assign(apDevices);

  const auto serverIpv4 = p2pInterfaces.GetAddress(GroundNodes::GC);
  const auto serverPort = 19574;
  const auto serverAddress = InetSocketAddress{serverIpv4, serverPort};

  auto coap_server = InstallCoAPServer(p2pNodes.Get(GroundNodes::GC));
  auto coap_clients = InstallCoAPClient(wifiStaNodes, serverAddress);

  auto tcp_server = InstallTcpSink(p2pNodes.Get(GroundNodes::GC), serverAddress);
  auto tcp_clients = InstallTcpOnOff(wifiStaNodes, serverAddress);

  TransferSpeedCollector collector;
  LatencyRecoder latencyRecoder{"./error/", "latency_"};

  std::for_each(wifiStaNodes.Begin(), wifiStaNodes.End(), [&collector,
                                                           &latencyRecoder](auto node)
  {
    std::ostringstream oss;
    oss << "/NodeList/" << node->GetId()
        << "/ApplicationList/*/";

    Config::Connect(oss.str() + "$ns3::CoAPClient/MsgInterval",
                    MakeCallback(&TransferSpeedCollector::CollectSpeed, &collector));

    Config::Connect(oss.str() + "$ns3::CoAPClient/MsgTransfer",
                    MakeCallback(&LatencyRecoder::RecordTransfer, &latencyRecoder));
  });

  {
    std::ostringstream oss;
    oss << "/NodeList/" << p2pNodes.Get(GroundNodes::GC)->GetId()
        << "/ApplicationList/*/$ns3::CoAPServer/PacketReceived";
    Config::Connect(oss.str(), MakeCallback(&LatencyRecoder::RecordReceive, &latencyRecoder));
  }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop(SIMUL_TIME);
  Simulator::Run();
  Simulator::Destroy();
}
