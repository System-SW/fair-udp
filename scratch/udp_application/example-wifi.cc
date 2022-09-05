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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "fair-udp.h"
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("example-wifi");

enum special_nodes
  {
    WIFI_AP = 0,
    P2P_SERVER = 1,
  };

class FairUdphelper
{
public:
  FairUdphelper(Ptr<FairUdpApp> client, Ipv4Address dest, port_t port):
    client_{client}, dest_{dest}, port_{port}
  {
  }

  void
  SendPacket(size_t data_len)
  {
    auto packet = Create<Packet>(data_len);

    Simulator::Schedule(MilliSeconds(100), &FairUdpApp::SendMsg, client_, packet, dest_, port_);
    Simulator::Schedule(MilliSeconds(100), &FairUdphelper::SendPacket, this, data_len);
  }

private:

  Ptr<FairUdpApp> client_;
  Ipv4Address dest_;
  port_t port_;
};   

int
main(int argc, char *argv[])
{
  // wired part
  NodeContainer p2pNodes(2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));

  auto p2pDevices = pointToPoint.Install(p2pNodes);

  // wifi part
  NodeContainer wifiStaNodes(3);
  auto wifiApNode = p2pNodes.Get(special_nodes::WIFI_AP);

  auto channel = YansWifiChannelHelper::Default();
  auto phy = YansWifiPhyHelper();
  phy.SetChannel(channel.Create());

  WifiMacHelper mac;
  auto ssid = Ssid("ns-3-ssid");

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ac);
  mac.SetType("ns3::StaWifiMac",
              "Ssid", SsidValue (ssid),
              "ActiveProbing", BooleanValue(false));
  auto staDevices = wifi.Install(phy, mac, wifiStaNodes);

  mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
  auto apDevices = wifi.Install(phy, mac, wifiApNode);

  // mobility for stas
  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0.0),
                                "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(5.0),
                                "GridWidth", UintegerValue(3),
                                "LayoutType", StringValue("RowFirst"));

  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
  mobility.Install(wifiStaNodes);

  // mobility for ap
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNode);

  // internet setting
  InternetStackHelper stack;
  stack.Install(p2pNodes.Get(special_nodes::P2P_SERVER));
  stack.Install(wifiApNode);
  stack.Install(wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase("10.1.1.0", "255.255.255.0");
  auto p2pInterfaces = address.Assign(p2pDevices);

  address.SetBase("10.1.3.0", "255.255.255.0");
  address.Assign(staDevices);
  address.Assign(apDevices);

  // assign apps to endpoints
  auto client = CreateObject<FairUdpApp>();
  auto client_node = wifiStaNodes.Get(0);
  client_node->AddApplication(client);
  client->SetStartTime(Seconds(0));
  client->SetStopTime(Seconds(10));

  auto server = CreateObject<FairUdpApp>();
  auto server_node = p2pNodes.Get(special_nodes::P2P_SERVER);
  server_node->AddApplication(server);


  FairUdphelper app_helper(client, p2pInterfaces.GetAddress(special_nodes::P2P_SERVER), 7777);
  app_helper.SendPacket(1024);


  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  LogComponentEnable("FairUdpApp", LOG_LEVEL_INFO);

  Simulator::Stop(Seconds(10.0));

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
