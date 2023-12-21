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
#pragma once
#ifndef TESTS_H
#define TESTS_H

#include <unordered_map>
#include <string>
#include <fstream>
#include <tuple>
#include <vector>
#include "ns3/nstime.h"

void WifiTest();

// for tracing

class TransferSpeedCollector
{
public:
  void CollectSpeed(std::string context, ns3::Time rtt);
  ~TransferSpeedCollector();

private:
  std::unordered_map<std::string,
                     std::vector<std::tuple<ns3::Time, ns3::Time>>> m_MsgIntervals;
};

namespace ns3
{
  class Packet;
}

class LatencyRecoder
{
public:
  using PUID_t = uint64_t;
  using record_t = std::tuple<bool, ns3::Time, PUID_t>;

  LatencyRecoder(std::string errorRateFile, std::string latencyFilePrefix);
  void RecordTransfer(std::string context, ns3::Ptr<const ns3::Packet>);
  void RecordReceive(std::string context, ns3::Ptr<const ns3::Packet>);
  ~LatencyRecoder();

private:
  void RecordErrorRate() const;
  void RecordLatency() const;
  PUID_t GenerateNewPacketId();

  const std::string m_ErrorRateFileName;
  const std::string m_LatencyFileName;
  PUID_t m_PacketIdCounter{0};
  std::unordered_map<std::string,
                     std::vector<std::tuple<ns3::Time, record_t>>> m_LatencyRecords;
};

#endif /* TESTS_H */
