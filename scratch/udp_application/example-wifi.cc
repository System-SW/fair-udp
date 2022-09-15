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
#include "ns3/random-variable-stream.h"
#include "ns3/udp-client-server-helper.h"
#include "fair-udp.h"
#include "fair-udp-helper.h"
#include "config.h"
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
  static int id = 1;
  client->Draw("node" + std::to_string(id));
  id++;
}

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
  NodeContainer wifiStaNodes(UAV_NUM);
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
  mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                "rho", DoubleValue(10.0),
                                "X", DoubleValue(50),
                                "Y", DoubleValue(50));

  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel");
  // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiStaNodes);


  // mobility for ap
  MobilityHelper mobility_ap;
  auto position_alloc = CreateObject<ListPositionAllocator>();
  position_alloc->Add(Vector(50, 50, 0));
  mobility_ap.SetPositionAllocator(position_alloc);
  mobility_ap.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility_ap.Install(wifiApNode);

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

  if constexpr (TARGET_PROTO == FAIR_UDP)
    {
      FairUdpHelper please(InetSocketAddress(p2pInterfaces.GetAddress(special_nodes::P2P_SERVER), 7777));
      std::string msg;
      msg.resize(1024);
      DummyStream s(msg);

      auto clients = please.Install(wifiStaNodes);

      SeedManager::SetSeed(1423);
      std::for_each(clients.Begin(), clients.End(), [&s](auto client)
      {
        auto random_generator = CreateObject<UniformRandomVariable>();
        auto jitter = random_generator->GetInteger(0, 1000);
        client->SetStartTime(Seconds(0));
        Simulator::Schedule(MilliSeconds(jitter), &FairUdpApp::SendStream, static_cast<FairUdpApp *>(&*client), &s);
      });

      clients.Stop(Seconds(TEST_TIME));

      auto server = CreateObject<FairUdpApp>();
      auto server_node = p2pNodes.Get(special_nodes::P2P_SERVER);
      server_node->AddApplication(server);
      LogComponentEnable("FairUdpApp", LOG_LEVEL_INFO);

      // generate trace file
      phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
      pointToPoint.EnablePcapAll("fair-udp");
      phy.EnablePcap("fair-udp", apDevices.Get(special_nodes::WIFI_AP));
    }
  else                          // udp for now
    {
      UdpServerHelper server(7777);
      auto apps = server.Install(p2pNodes.Get(special_nodes::P2P_SERVER));
      apps.Start(Seconds(0));
      apps.Stop(Seconds(TEST_TIME));

      uint32_t max_packet_size = 1024;
      Time interval = MilliSeconds(1);
      uint32_t max_packet_count = UINT32_MAX;
      auto server_addr = p2pInterfaces.GetAddress(special_nodes::P2P_SERVER);
      UdpClientHelper client(server_addr, 7777);
      client.SetAttribute ("MaxPackets", UintegerValue (max_packet_count));
      client.SetAttribute ("Interval", TimeValue (interval));
      client.SetAttribute ("PacketSize", UintegerValue (max_packet_size));
      apps = client.Install(wifiStaNodes);
      apps.Start(Seconds(0));
      apps.Stop(Seconds(TEST_TIME));

      // generate trace file
      phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
      pointToPoint.EnablePcapAll("normal-udp");
      phy.EnablePcap("normal-udp", apDevices.Get(special_nodes::WIFI_AP));
    }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Simulator::Stop(Seconds(TEST_TIME));
  Simulator::Run();
  Simulator::Destroy();

  // std::for_each(clients.Begin(), clients.End(), [](auto client)
  // {
  //   draw_please(dynamic_cast<FairUdpApp *>(&*client));
  // });

  return 0;
}
