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
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "coap-client.h"
#include "coap-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CoAPExample");

int main(int argc, char *argv[])
{
  CommandLine cmd{__FILE__};
  cmd.Parse(argc, argv);

  NS_LOG_INFO("Create nodes.");
  NodeContainer n{2};

  InternetStackHelper internet;
  internet.Install(n);


  NS_LOG_INFO("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  NetDeviceContainer d = csma.Install (n);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (d);
  auto serverAddress = Address(i.GetAddress (1));


  NS_LOG_INFO ("Create Applications.");
  CoAPHelper coap{serverAddress};
  ApplicationContainer apps = coap.Install(n);
  apps.Start(Seconds(1.0));
  apps.Stop(Seconds(2.0));

  csma.EnablePcapAll("coap", true);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(3));
  Simulator::Run();
  Simulator::Destroy ();
  return 0;
}

