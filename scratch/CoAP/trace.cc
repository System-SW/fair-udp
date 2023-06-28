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
#include <regex>
#include <sstream>
#include <fstream>
#include "tests.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

static std::string
ParseNodeId(const std::string& context)
{
  // key: "/NodeList/[i]/ApplicationList/*/ns3::CoAPClient|CoAPServer/???"
  static std::regex node_number_re{"[0-9]+"};
  auto re_iter = std::sregex_iterator(context.begin(), context.end(),
                                      node_number_re);
  return *re_iter->begin();
}

void
TransferSpeedCollector::CollectSpeed(std::string context, ns3::Time rtt)
{
  auto current_time = ns3::Simulator::Now();
  auto record = std::make_tuple(current_time, rtt);
  m_MsgIntervals[context].push_back(record);
}

TransferSpeedCollector::~TransferSpeedCollector ()
{
  // key: "/NodeList/[i]/ApplicationList/*/ns3::CoAPClient/MsgInterval"

  for (auto [node_str, record] : m_MsgIntervals)
    {
      std::ostringstream oss;
      auto node_id = ParseNodeId(node_str);
      oss << "./log/" << node_id << ".csv";
      std::ofstream csv{oss.str()};

      csv << "Time(s),Interval(s)\n";
      for (auto [x_val, y_val] : record)
        {
          csv << x_val.GetSeconds() << ',' << y_val.GetSeconds() << '\n';
        }
    }
}


// Latency Recoder

LatencyRecoder::LatencyRecoder(std::string errorRateFile, std::string latencyFilePrefix):
  m_ErrorRateFileName{errorRateFile}, m_LatencyFileName{latencyFilePrefix}
{
}    

// for CoAPClient Side
void
LatencyRecoder::RecordTransfer(std::string context, const ns3::Ptr<ns3::Packet> p)
{
  auto current_time = ns3::Simulator::Now();
  PUID_t packet_id = p->GetUid();
  record_t record = { false, current_time, packet_id };
  m_LatencyRecords[context].push_back(record);
}


// for CoAPServer Side
void
LatencyRecoder::RecordReceive(std::string context, const ns3::Ptr<ns3::Packet> p)
{
  PUID_t packet_id = p->GetUid();
  auto& record_list = m_LatencyRecords[context];
  auto target = std::find_if(record_list.begin(), record_list.end(),
                             [packet_id](record_t record)
                             {
                               return std::get<2>(record) == packet_id &&
                                 std::get<0>(record) == false;
                             });

  if (target != record_list.end()) // find it!
    {
      // modify value
      auto current_time = ns3::Simulator::Now();
      record_t& record = *target;
      std::get<0>(record) = true;
      std::get<1>(record) = current_time - std::get<1>(record);
    }
  else
    {
      std::cerr << "strange target detected\n";
    }
}

LatencyRecoder::~LatencyRecoder()
{
  RecordErrorRate();
  RecordLatency();
}

void
LatencyRecoder::RecordErrorRate() const
{
  // calculate Error Rate for each CoAP Clients and writes them to an single file.
  std::ofstream errorfile{m_ErrorRateFileName + ".csv"};

  for (const auto& [node, records] : m_LatencyRecords)
    {
      errorfile << node << ',';
      std::size_t lossed = 0;
      std::for_each(records.cbegin(), records.cend(), [&lossed](auto record)
      {
        if (!std::get<0>(record))
          lossed++;
      });
      double error_rate = lossed / (double) records.size();
      errorfile << error_rate << '\n';
    }
}

void
LatencyRecoder::RecordLatency() const
{
  // calculate Packet transmission latency for each packet and
  // record them per CoAP Client file
  for (const auto& [node, records] : m_LatencyRecords)
    {
      std::ofstream latencyfile{m_ErrorRateFileName + node + ".csv"};
      latencyfile << "Latency(s)\n"; // write csv header

      for (auto record : records)
        {
          // we can measure the latency from received packet only
          if (std::get<0>(record))
            {
              latencyfile << std::get<1>(record).GetSeconds() << '\n';
            }
        }
    }
}
