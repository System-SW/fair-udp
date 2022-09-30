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
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <string>

#include "fudp-client-helper.h"
#include "fudp-server-helper.h"
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
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/scheduler.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-client.h"
#include "ns3/wifi-module.h"

#include "config.h"
#include "fudp-header.h"
#include "fudp-application.h"
#include "fudp-client.h"
#include "fudp-server.h"

#define PHASE_INTERVAL 30
#define PHASE_INTERVAL_S "30"
#define PHASE_INTERVAL_HALF_S "15"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("example-wifi");

enum SpecialNodes {
  WIFI_AP = 0,
  P2P_SERVER = 1,
};

template <FudpFeature FUDP_FEATURES>
decltype (auto) GenerateFudpPhaseSetupFunc (::ns3::Ptr<::ns3::Node> serverNode, ::ns3::NodeContainer &clientNodes)
{
  return [serverNode, &clientNodes] (auto const &serverAddress, auto const startTime, auto const endTime) {
    auto fudpClientHelper = FudpClientHelper<FUDP_FEATURES>{serverAddress};
    auto fudpClientApps = fudpClientHelper.Install (clientNodes);
    for (auto iter = fudpClientApps.Begin (); iter != fudpClientApps.End (); ++iter)
      {
        auto &app = reinterpret_cast<FudpApplication &> (**iter);
        app.SetStartTime (Seconds (startTime));
        app.SetStopTime (Seconds (endTime));

        auto &fudpClient = app.GetImpl<FudpClient<FUDP_FEATURES>> ();
        ::ns3::Simulator::Schedule (MilliSeconds (startTime * 1000 + 1), &FudpClient<FUDP_FEATURES>::SendTraffic,
                                    &fudpClient);
      }

    auto fudpServerHelper = FudpServerHelper<FUDP_FEATURES> ();
    fudpServerHelper.SetServerPort (::ns3::InetSocketAddress::ConvertFrom (serverAddress).GetPort ());
    auto fudpServerApps = fudpServerHelper.Install (serverNode);
    fudpServerApps.Start (Seconds (startTime));
    fudpServerApps.Stop (Seconds (endTime));

    auto tcpServerHelper = PacketSinkHelper{"ns3::TcpSocketFactory", serverAddress};
    auto tcpServerApps = tcpServerHelper.Install (serverNode);
    tcpServerApps.Start (Seconds (startTime));
    tcpServerApps.Stop (Seconds (endTime));

    auto tcpClientHelper =
        OnOffHelper{"ns3::TcpSocketFactory", ::ns3::InetSocketAddress::ConvertFrom (serverAddress).GetIpv4 ()};
    tcpClientHelper.SetAttribute ("Remote", AddressValue{serverAddress});
    tcpClientHelper.SetAttribute ("OnTime",
                                  StringValue ("ns3::ConstantRandomVariable[Constant=" PHASE_INTERVAL_HALF_S "]"));
    tcpClientHelper.SetAttribute ("OffTime",
                                  StringValue ("ns3::ConstantRandomVariable[Constant=" PHASE_INTERVAL_HALF_S "]"));

    auto tcpClientApps = tcpClientHelper.Install (clientNodes);
    tcpClientApps.Start (Seconds (startTime));
    tcpClientApps.Stop (Seconds (endTime));
  };
}

int main (int argc, char *argv[])
{
  using namespace std::string_literals;

  auto SIMUL_TIME = PHASE_INTERVAL * 7;
  auto PROTOCOL = "udp"s;
  auto SERVER_BANDWIDTH = "1000Mbps"s;
  auto NUM_UAVS = UAV_NUM;

  auto cmd = CommandLine{__FILE__};
  cmd.AddValue ("protocol", "", PROTOCOL);
  cmd.AddValue ("server_bandwidth", "", SERVER_BANDWIDTH);
  cmd.AddValue ("uavs", "", NUM_UAVS);
  cmd.Parse (argc, argv);

  {
    constexpr auto ALLOWED_PROTOCOLS = ::std::array{"udp", "fudp"};
    if (::std::all_of (ALLOWED_PROTOCOLS.begin (), ALLOWED_PROTOCOLS.end (),
                       [&PROTOCOL] (auto v) { return PROTOCOL != v; }))
      {
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

  SeedManager::SetSeed (1423);
  auto rng = CreateObject<UniformRandomVariable> ();

  auto const serverIpv4 = p2pInterfaces.GetAddress (SpecialNodes::P2P_SERVER);
  auto const serverPortOffset = 7777_u16;

  auto phaseIndex = 0_u16;
  auto const CallPhaseSetupFunc = [&phaseIndex, &serverIpv4] (auto const &setupFunc) {
    u16 const serverPort = serverPortOffset + phaseIndex;
    setupFunc (::ns3::InetSocketAddress{serverIpv4, serverPort}, PHASE_INTERVAL * phaseIndex,
               PHASE_INTERVAL * (phaseIndex + 1));
    ++phaseIndex;
  };

  auto const SetupPhase1 = [&] (::ns3::Address const &serverAddress, auto const startTime, auto const endTime) {
    auto const serverPort = ::ns3::InetSocketAddress::ConvertFrom (serverAddress).GetPort ();

    u32 const max_packet_size = 1024;
    auto udpClientHelper = UdpClientHelper{serverIpv4, serverPort};
    udpClientHelper.SetAttribute ("MaxPackets", UintegerValue (UINT32_MAX));
    udpClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (2)));
    udpClientHelper.SetAttribute ("PacketSize", UintegerValue (max_packet_size));

    auto udpClientApps = udpClientHelper.Install (wifiStaNodes);
    udpClientApps.Start (Seconds (startTime));
    udpClientApps.Stop (Seconds (endTime));

    auto udpServerApps = UdpServerHelper{serverPort}.Install (p2pNodes.Get (SpecialNodes::P2P_SERVER));
    udpServerApps.Start (Seconds (startTime));
    udpServerApps.Stop (Seconds (endTime));
  };

  auto const SetupPhase2 = [&] (::ns3::Address const &serverAddress, auto const startTime, auto const endTime) {
    auto tcpServerHelper = PacketSinkHelper{"ns3::TcpSocketFactory", serverAddress};
    auto tcpServerApps = tcpServerHelper.Install (p2pNodes.Get (SpecialNodes::P2P_SERVER));
    tcpServerApps.Start (Seconds (startTime));
    tcpServerApps.Stop (Seconds (endTime));

    auto tcpClientHelper = OnOffHelper{"ns3::TcpSocketFactory", p2pInterfaces.GetAddress (SpecialNodes::P2P_SERVER)};
    tcpClientHelper.SetAttribute ("Remote", AddressValue{serverAddress});
    tcpClientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" PHASE_INTERVAL_S "]"));
    tcpClientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

    auto tcpClientApps = tcpClientHelper.Install (wifiStaNodes);
    tcpClientApps.Start (Seconds (startTime));
    tcpClientApps.Stop (Seconds (endTime));
  };

  auto const SetupPhase3 = [&] (::ns3::Address const &serverAddress, auto const startTime, auto const endTime) {
    auto const serverPort = ::ns3::InetSocketAddress::ConvertFrom (serverAddress).GetPort ();

    u32 const max_packet_size = 1024;
    auto udpClientHelper = UdpClientHelper{serverIpv4, serverPort};
    udpClientHelper.SetAttribute ("MaxPackets", UintegerValue (UINT32_MAX));
    udpClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (2)));
    udpClientHelper.SetAttribute ("PacketSize", UintegerValue (max_packet_size));

    auto udpClientApps = udpClientHelper.Install (wifiStaNodes);
    udpClientApps.Start (Seconds (startTime));
    udpClientApps.Stop (Seconds (endTime));

    auto udpServerApps = UdpServerHelper{serverPort}.Install (p2pNodes.Get (SpecialNodes::P2P_SERVER));
    udpServerApps.Start (Seconds (startTime));
    udpServerApps.Stop (Seconds (endTime));

    auto tcpServerHelper = PacketSinkHelper{"ns3::TcpSocketFactory", serverAddress};
    auto tcpServerApps = tcpServerHelper.Install (p2pNodes.Get (SpecialNodes::P2P_SERVER));
    tcpServerApps.Start (Seconds (startTime));
    tcpServerApps.Stop (Seconds (endTime));

    auto tcpClientHelper = OnOffHelper{"ns3::TcpSocketFactory", p2pInterfaces.GetAddress (SpecialNodes::P2P_SERVER)};
    tcpClientHelper.SetAttribute ("Remote", AddressValue{serverAddress});
    tcpClientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" PHASE_INTERVAL_S "]"));
    tcpClientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

    auto tcpClientApps = tcpClientHelper.Install (wifiStaNodes);
    tcpClientApps.Start (Seconds (startTime));
    tcpClientApps.Stop (Seconds (endTime));
  };

  auto serverNode = p2pNodes.Get (SpecialNodes::P2P_SERVER);
  CallPhaseSetupFunc (SetupPhase1);
  CallPhaseSetupFunc (SetupPhase2);
  CallPhaseSetupFunc (SetupPhase3);
  CallPhaseSetupFunc (GenerateFudpPhaseSetupFunc<0> (serverNode, wifiStaNodes));
  CallPhaseSetupFunc (GenerateFudpPhaseSetupFunc<FUDP_FEATURE_NACK_SEQUENCE> (serverNode, wifiStaNodes));
  CallPhaseSetupFunc (GenerateFudpPhaseSetupFunc<FUDP_FEATURE_HEALTH_PROBE> (serverNode, wifiStaNodes));
  CallPhaseSetupFunc (
      GenerateFudpPhaseSetupFunc<FUDP_FEATURE_NACK_SEQUENCE | FUDP_FEATURE_HEALTH_PROBE> (serverNode, wifiStaNodes));

  // generate trace file
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  p2pHelper.EnablePcapAll (PROTOCOL);
  phy.EnablePcap (PROTOCOL, apDevices.Get (SpecialNodes::WIFI_AP));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (SIMUL_TIME));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
