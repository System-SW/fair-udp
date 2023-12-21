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
#include "fdp-header.h"
#include "tests.h"
#include "option.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CoAPExample");

enum TestNumber : int
  {
    CSMA = 1,
    HEADER = 2,
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
  server_app.Stop(Seconds(30.0));

  csma.EnablePcapAll("coap", true);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(30));
  Simulator::Run();
  Simulator::Destroy ();
}

void HeaderTest()
{
  NS_LOG_FUNCTION("Header Test");

  {
    CoAPHeader coap_hdr;
    FDPMessageHeader fdp_hdr;

    CoAPHeader::PreparePut(coap_hdr, 4, 0X1234, 123);

    fdp_hdr.SetMsgInterval(MilliSeconds(1234));
    fdp_hdr.SetMsgSeq(1);
    fdp_hdr.SetSeqBit(false);

    NS_LOG_INFO(fdp_hdr);

    Ptr<Packet> p = Create<Packet>();

    p->AddHeader(fdp_hdr);
    p->AddHeader(coap_hdr);

    NS_LOG_INFO("Now check deserialized headers!");

    CoAPHeader coap_de_hdr;
    FDPMessageHeader fdp_de_hdr;

    p->RemoveHeader(coap_de_hdr);
    p->RemoveHeader(fdp_de_hdr);

    NS_LOG_INFO(fdp_de_hdr);
  }

  NS_LOG_INFO("========== Feedback header test===========");

  {
    CoAPHeader coap_hdr;
    FDPFeedbackHeader fdp_hdr;

    CoAPHeader::PreparePut(coap_hdr, 4, 0X1234, 123);

    fdp_hdr.SetLatency(MilliSeconds(1234));
    fdp_hdr.SetMsgSeq(1);
    fdp_hdr.SetSeqBit(false);

    NS_LOG_INFO(fdp_hdr);
    NS_LOG_INFO(coap_hdr);

    Ptr<Packet> p = Create<Packet>();

    p->AddHeader(fdp_hdr);
    p->AddHeader(coap_hdr);

    NS_LOG_INFO("Now check deserialized headers!");

    CoAPHeader coap_de_hdr;
    FDPFeedbackHeader fdp_de_hdr;

    p->RemoveHeader(coap_de_hdr);
    p->RemoveHeader(fdp_de_hdr);

    NS_LOG_INFO(fdp_de_hdr);
    NS_LOG_INFO(coap_de_hdr);
  }
}

int main(int argc, char *argv[])
{
  int which_one;
  CommandLine cmd{__FILE__};
  cmd.AddValue("WhichTest",
               "1. csma test\n 2. header serialization test\n"
               "3. CoAP Transfer Test.\n",
               which_one);
  cmd.AddValue("UseFDP",
               "true: enable FDP, false: enable CoCoA\n",
               UseFDP);
  cmd.AddValue("SendTCP",
               "true: enable TCP",
               SendTCP);
  cmd.AddValue("PCAP_Name",
               "Pcap File Name with absolute path\n",
               PCAP_NAME);
  cmd.Parse(argc, argv);

  switch (which_one)
    {
    case TestNumber::CSMA:
      CsmaExample();
      break;
    case TestNumber::HEADER:
      HeaderTest();
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

