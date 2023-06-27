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
#include "tests.h"
#include "ns3/simulator.h"

void
TransferSpeedCollector::CollectSpeed(std::string context, ns3::Time rtt)
{
  auto current_time = ns3::Simulator::Now();
  auto record = std::make_tuple(current_time, rtt);
  m_MsgIntervals[context].push_back(record);
}

TransferSpeedCollector::~TransferSpeedCollector ()
{
  WriteToCSV();
}

void
TransferSpeedCollector::WriteToCSV()
{
  // key: "/NodeList/[i]/ApplicationList/*/ns3::CoAPClient/MsgInterval"
  static std::regex node_number_re("[0-9]+");

  for (auto [node_str, record] : m_MsgIntervals)
    {
      std::ostringstream oss;
      auto re_iter = std::sregex_iterator(node_str.begin(), node_str.end(),
                                          node_number_re);
      oss << "./log/" << *re_iter->begin() << ".csv";
      std::ofstream csv{oss.str()};

      csv << "Time(s),Interval(s)\n";
      for (auto [x_val, y_val] : record)
        {
          csv << x_val.GetSeconds() << ',' << y_val.GetSeconds() << '\n';
        }
    }
}
