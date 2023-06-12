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
#include "coap-server.h"
#include "coap-helper.h"
#include "coap-header.h"
#include "tests.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CoAPExample");

enum TestNumber : int
  {
    CSMA = 1,
    WIFI = 3,
  };

void CsmaExample()
{
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
  CoAPClientHelper coap_client{serverAddress};
  ApplicationContainer client_app = coap_client.Install(n.Get(0));
  client_app.Start(Seconds(1.0));
  client_app.Stop(Seconds(120.0));

  CoAPServerHelper coap_server;
  ApplicationContainer server_app = coap_server.Install(n.Get(1));
  server_app.Start(Seconds(0));
  server_app.Stop(Seconds(120.0));

  csma.EnablePcapAll("coap", true);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(120));
  Simulator::Run();
  Simulator::Destroy ();
}

int main(int argc, char *argv[])
{
  int which_one;
  CommandLine cmd{__FILE__};
  cmd.AddValue("WhichTest",
               "1. csma test\n 2. header serialization test\n",
               which_one);
  cmd.Parse(argc, argv);

  switch (which_one)
    {
    case TestNumber::CSMA:
      CsmaExample();
      break;
    case TestNumber::WIFI:
      WifiTest();
      break;
    default:
      NS_LOG_ERROR("No such test number!" << cmd);
      break;
    }
  return 0;
}

