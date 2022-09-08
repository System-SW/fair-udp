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

#include "congestion-control.h"
#include "ns3/core-module.h"
#include "config.h"
#include <numeric>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CongestionInfo");

CongestionInfo::CongestionInfo(uint64_t msg_size):
  CongestionInfo()
{
  msg_size_ = msg_size;
}

CongestionInfo::CongestionInfo()
{
}

void
CongestionInfo::PacketDropDetected(uint16_t nack_seq)
{
  if (prev_nack_seq_ != nack_seq)
    {
      nack_counter_ = 0;
      prev_nack_seq_ = nack_seq;
    }

  nack_counter_++;
  if (nack_counter_ >= 2)
    {
      threshhold_ = bandwidth_ / 2;
      NS_LOG_INFO("threshhold " << threshhold_);
      bandwidth_ = 1;           // slow start
    }
}

uint64_t
CongestionInfo::GetTransferInterval()
{
  auto prev_bandwidth = bandwidth_;

  if (bandwidth_ < threshhold_)
    {
      bandwidth_ *= 2;
    }
  else
    {
      bandwidth_++;
    }

  auto interval = msg_size_ / prev_bandwidth;
  if (interval == 0)
    {
      bandwidth_ = msg_size_;
      interval = 1;
    }
  bandwidth_info_.Add(msg_size_);
  return interval;
}

BandwidthInfo::BandwidthInfo()
{
  start_ = Now();
  bandwidth_data_.SetTitle("Fair UDP Bandwidth");
  bandwidth_data_.SetStyle(Gnuplot2dDataset::LINES_POINTS);
}

void
BandwidthInfo::Start()
{
  start_ = Now();
  Simulator::Schedule(MilliSeconds(AVERAGE_INTERVAL), &BandwidthInfo::Update, this);
}

void
BandwidthInfo::Update()
{
  auto duration = Now() - start_;
  bandwidth_data_.Add(Now().GetSeconds(), (double)transferred_bytes_ / duration.GetMilliSeconds()); // kb/s now
  // transferred_bytes_ = 0;
  // start_ = Now();
  Simulator::Schedule(MilliSeconds(AVERAGE_INTERVAL), &BandwidthInfo::Update, this);
}

void
BandwidthInfo::Add(uint64_t bytes)
{
  transferred_bytes_ += bytes;
}
