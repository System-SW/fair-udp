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
#include "fair-udp-helper.h"
#include <string>
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("example-wifi");

enum special_nodes
  {
    WIFI_AP = 0,
    P2P_SERVER = 1,
  };

class DummyStream : public PacketSource
{
public:
  DummyStream(std::string msg):
    msg_(msg)
  {
  }

  Ptr<Packet>
  GetPacket() override
  {
    auto packet = Create<Packet>(reinterpret_cast<uint8_t *>(msg_.data()), msg_.size());
    return packet;
  }
private:
  std::string msg_;
};

void draw_please(FairUdpApp* client)
{
  client->Draw();
}

int
main(int argc, char *argv[])
{
  // wired part
  NodeContainer p2pNodes(2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("1ms"));

  auto p2pDevices = pointToPoint.Install(p2pNodes);

  // wifi part
  NodeContainer wifiStaNodes(30);
  auto wifiApNode = p2pNodes.Get(special_nodes::WIFI_AP);

  auto channel = YansWifiChannelHelper::Default();
  auto phy = YansWifiPhyHelper();
  phy.SetChannel(channel.Create());
  phy.SetErrorRateModel("ns3::YansErrorRateModel");

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


  FairUdpHelper please(InetSocketAddress(p2pInterfaces.GetAddress(special_nodes::P2P_SERVER), 7777));
  std::string msg;
  msg.resize(1024);
  DummyStream s(msg);

  auto clients = please.Install(wifiStaNodes);
  double jitter = 0.1;

  std::for_each(clients.Begin(), clients.End(), [&s, &jitter](auto client)
  {
    client->SetStartTime(Seconds(0));
    client->SetStopTime(Seconds(100));
    Simulator::Schedule(Seconds(jitter), &FairUdpApp::SendStream, static_cast<FairUdpApp *>(&*client), &s);
    jitter += 0.05;
  });


  auto server = CreateObject<FairUdpApp>();
  auto server_node = p2pNodes.Get(special_nodes::P2P_SERVER);
  server_node->AddApplication(server);

  Packet::EnablePrinting();

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  LogComponentEnable("FairUdpApp", LOG_LEVEL_INFO);

  Simulator::Stop(Seconds(100));

  Simulator::Run();
  Simulator::Destroy();

  auto client = clients.Get(10);

  draw_please(dynamic_cast<FairUdpApp *>(&*client));

  return 0;
}
