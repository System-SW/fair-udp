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
#include "ns3/header.h"
#include "ns3/nstime.h"
#include <stdint.h>
#pragma once
#ifndef FDP_HEADER_H
#define FDP_HEADER_H

namespace ns3
{
  constexpr static inline uint16_t BIT_12_MAX = uint16_t((0x1 << 11) - 1);

  class Packet;

  class FDPMessageHeader : public Header
  {
  public:
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;

    uint32_t GetSerializedSize() const override;

    void Serialize(Buffer::Iterator start) const override;

    uint32_t Deserialize(Buffer::Iterator start) override;

    void Print(std::ostream& os) const override;

    void SetSeqBit(bool seq_bit);

    bool GetSeqBit() const;
    
    void SetMsgSeq(unsigned int msg_seq);

    unsigned int GetMsgSeq() const;

    Time GetMsgInterval() const; // return ms value

    void SetMsgInterval(uint16_t interval_ms);

    void SetMsgInterval(Time interval_ms);

  private:
    bool m_seq_bit{false};
    uint8_t m_msg_seq{0};
    uint16_t m_interval{0};
  };


  class FDPFeedbackHeader : public Header
  {
  public:
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;

    uint32_t GetSerializedSize() const override;

    void Serialize(Buffer::Iterator start) const override;

    uint32_t Deserialize(Buffer::Iterator start) override;

    void Print(std::ostream& os) const override;

    void OnResetBit();

    void OffResetBit();

    bool FlipSeqBit();

    bool GetSeqBit() const;

    unsigned int IncMsgSeq();

    unsigned int GetMsgSeq() const;

    Time GetLatency() const;    // return ms value

    void SetLatency(uint16_t interval_ms);

    void SetLatency(Time interval_ms);

  private:
    bool m_reset_bit{false};
    bool m_seq_bit{false};
    uint8_t m_msg_seq{0};
    uint16_t m_latency{0};      // 12 bits
  };

}

#endif /* FDP_HEADER_H */
