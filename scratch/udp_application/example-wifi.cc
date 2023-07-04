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

#include "config.h"
#include "fdp-client-server-helper.h"
#include "ns3/application-container.h"
#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/on-off-helper.h"
#include "ns3/onoff-application.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-module.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-client.h"
#include "ns3/wifi-module.h"

#include "config.h"
#include "fudp-header.h"
#include "fudp-application.h"
#include "fudp-client.h"
#include "fudp-server.h"

#define PHASE_INTERVAL 40
#define PHASE_INTERVAL_S "40"
#define PHASE_INTERVAL_HALF_S "10"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("example-wifi");

enum SpecialNodes {
  WIFI_AP = 0,
  P2P_SERVER = 1,
};


int main (int argc, char *argv[])
{
  using namespace std::string_literals;

  auto SIMUL_TIME = PHASE_INTERVAL + (PHASE_INTERVAL + 10) * 3;
  auto PROTOCOL = "udp"s;
  auto SERVER_BANDWIDTH = "1000Mbps"s;
  auto NUM_UAVS = UAV_NUM;

  auto cmd = CommandLine{__FILE__};
  cmd.AddValue ("protocol", "", PROTOCOL);
  cmd.AddValue ("server_bandwidth", "", SERVER_BANDWIDTH);
  cmd.AddValue ("uavs", "", NUM_UAVS);
  cmd.AddValue ("simul_time", "", SIMUL_TIME);
  cmd.Parse (argc, argv);

  {
    constexpr auto ALLOWED_PROTOCOLS = ::std::array{"udp", "fdp"};
    if (::std::all_of (ALLOWED_PROTOCOLS.begin (), ALLOWED_PROTOCOLS.end (),
                       [&PROTOCOL] (auto v) { return PROTOCOL != v; }))
      {
        NS_LOG_ERROR("Unproper protocol name");
        ::std::exit (-1);
      }
  }

  // wired part
  auto p2pNodes = NodeContainer{2};

  auto p2pHelper = PointToPointHelper{};
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue (SERVER_BANDWIDTH));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("1ms"));

  auto p2pDevices = p2pHelper.Install (p2pNodes);

  // wifi part
  auto wifiStaNodes = NodeContainer{NUM_UAVS};
  auto wifiApNode = p2pNodes.Get (SpecialNodes::WIFI_AP);

  auto channel = YansWifiChannelHelper::Default ();
  auto phy = YansWifiPhyHelper ();
  phy.SetChannel (channel.Create ());

  WifiMacHelper mac;
  auto ssid = Ssid ("ns-3-ssid");

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211ac);
  mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
  auto staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  auto apDevices = wifi.Install (phy, mac, wifiApNode);

  // mobility for stas
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (10.0), "X", DoubleValue (50),
                                 "Y", DoubleValue (50));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel");
  // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  // mobility for ap
  MobilityHelper mobility_ap;
  auto position_alloc = CreateObject<ListPositionAllocator> ();
  position_alloc->Add (Vector (50, 50, 0));
  mobility_ap.SetPositionAllocator (position_alloc);
  mobility_ap.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap.Install (wifiApNode);

  // internet setting
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (SpecialNodes::P2P_SERVER));
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  auto p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  auto const serverIpv4 = p2pInterfaces.GetAddress (SpecialNodes::P2P_SERVER);
  auto const serverPort = 19574;
  auto const serverAddress = InetSocketAddress{serverIpv4, serverPort};

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Setup UDP clients and server
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (PROTOCOL == "fdp")
    {
      // LogComponentEnable ("FdpClient", LOG_LEVEL_INFO);
      // LogComponentEnable ("FdpServer", LOG_LEVEL_INFO);

      FdpServerHelper server;
      auto server_app = server.Install(p2pNodes.Get(SpecialNodes::P2P_SERVER));
      server_app.Start(Seconds(0));


      FdpClientHelper client{serverAddress};
      client.SetAttribute("MinInterval", TimeValue(MilliSeconds(1)));
      client.SetAttribute("MaxInterval", TimeValue(MilliSeconds(50)));
      auto client_apps = client.Install(wifiStaNodes);
      client_apps.Start(Seconds(1));
    }
  else // if (PROTOCOL == "udp")
    {
      UdpServerHelper udpServerHelper{serverPort};
      auto udpServer = udpServerHelper.Install(p2pNodes.Get(SpecialNodes::P2P_SERVER));
      udpServer.Start(Seconds(0));

      uint32_t max_packet_size = 1024;
      UdpClientHelper udpClientHelper{serverIpv4, serverPort};
      udpClientHelper.SetAttribute ("MaxPackets", UintegerValue (UINT32_MAX));
      udpClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));
      udpClientHelper.SetAttribute ("PacketSize", UintegerValue (max_packet_size));

      auto udpClients = udpClientHelper.Install(wifiStaNodes);
      udpClients.Start(Seconds(1));
    }

    u32 const max_packet_size = 1024;
    auto udpClientHelper = UdpClientHelper{serverIpv4, serverPort};
    udpClientHelper.SetAttribute ("MaxPackets", UintegerValue (UINT32_MAX));
    udpClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (2)));
    udpClientHelper.SetAttribute ("PacketSize", UintegerValue (max_packet_size));

  auto tcpServerHelper = PacketSinkHelper{"ns3::TcpSocketFactory", serverAddress};
  auto tcpServerApp = tcpServerHelper.Install (p2pNodes.Get (SpecialNodes::P2P_SERVER)).Get (0);
  tcpServerApp->SetStartTime (Seconds (0));

  // https://www.nsnam.org/doxygen/tcp-star-server_8cc_source.html
  auto tcpClientHelper = OnOffHelper{"ns3::TcpSocketFactory", p2pInterfaces.GetAddress (SpecialNodes::P2P_SERVER)};
  tcpClientHelper.SetAttribute ("Remote", AddressValue{serverAddress});
  tcpClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
  tcpClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
  auto tcpClients = tcpClientHelper.Install(wifiStaNodes);
  tcpClients.Start(Seconds(1));

  // generate trace file
  // p2pHelper.EnablePcapAll (PROTOCOL);
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.EnablePcap (PROTOCOL, apDevices.Get (SpecialNodes::WIFI_AP));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (SIMUL_TIME));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
