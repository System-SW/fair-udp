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

#ifndef CONGESTION_CONTROL_H
#define CONGESTION_CONTROL_H

#include <cstdint>
#include "ns3/gnuplot.h"
#include "ns3/timer.h"
#include "fudp-header.h"

namespace ns3
{
  class CongestionInfo
  {
  public:
    CongestionInfo();
    CongestionInfo(uint64_t msg_size);
    void PacketDropDetected(sequence_t nack_seq);
    uint64_t GetTransferInterval();
    void ReduceBandwidth();
  private:
    uint64_t bandwidth_{1};     // 1 kb
    uint64_t msg_size_{1024};   // 1 kb
    uint64_t threshhold_{10};    // 10 kb
    uint8_t nack_counter_{0};
    sequence_t prev_nack_seq_{0};
  };
}    


#endif /* CONGESTION_CONTROL_H */
