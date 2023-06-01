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
#pragma once
#ifndef FDP_RECEIVER_H
#define FDP_RECEIVER_H
#include "ns3/nstime.h"
#include "fdp-header.h"

namespace ns3
{
  class Packet;
  class Socket;
  /*
   * FDP Receiver Congestion Control Algorithm
   * Before procesessing fdp message, Receiver has to match the message sequence,
   * and it's current message sequence.
   *
   * 1. normal message (numberd 0 or 1)
   *    Receiver compares the actual latency and the interval written in the message.
   *    if latency is larger than 10ms, Receiver sends the feedback message with
   *    increased latency.
   *
   * 2. final message (numberd 2)
   *    Receiver waits up to twice as long as the message transfer interval.
   *    After receiving the final message, Receiver sends the reset feedback to
   *    Sender. And flip the sequence bit.
   */
  class FdpReceiverCC
  {
  private:
    Time m_PrevArrival{-1};
    Time m_RTT{-1};
    bool m_seq_bit{false};
    uint8_t m_msg_seq{0};

  public:
    Ptr<Packet> GenerateFeedback(FDPMessageHeader &hdr);

  private:
    Ptr<Packet> CreateNormalFeedback();
    Ptr<Packet> CreateFinalFeedback();
    bool GetSeqBit() const;
    void FlipSeqBit();
    uint8_t GetMsgSeq() const;
    void SetMsgSeq(uint8_t msg_seq);
  };
}

#endif /* FDP_RECEIVER_H */
