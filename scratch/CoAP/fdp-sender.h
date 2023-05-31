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
#ifndef FDP_SENDER_H
#define FDP_SENDER_H
#include <functional>
#include "ns3/nstime.h"
#include "coap-header.h"

namespace ns3
{
  class Socket;
  class Packet;

  class FdpSenderCC
  {
  private:
    Time m_RTT{MilliSeconds(2000)};
    Time m_RTO{MilliSeconds(2000)};
    Time m_RTTVAR{0};
    bool m_seq_bit{false};
    uint8_t m_msg_seq{0};       // 0, 1, 2

    Time m_PrevTransfer{0};
    EventId m_ResetEvent;
    uint8_t m_recent_feedback_msg_seq{0};

  public:
    FdpSenderCC();

    void TransferMessage(Ptr<Socket> socket, Ptr<Packet> packet, CoAPHeader &hdr);
    EventId ScheduleTransfer(std::function<void()> &&callback);
    void HandleFeedback(Ptr<Packet> packet);
    /*
     * 구현에 대한 간단한 뇌피셜을 끄적여봄 "Tue May 23 21:42:43 2023"
     * 우선 일반적인 Feedback에 반응하기 위해서는
     * 1. feedback을 수신한 시간을 저장
     * 2. data packet을 전송한 시간도 저장
     * 3. feedback의 Sequence 순서를 확인
     *
     * Reset의 경우, RTT와 RTO를 기준으로 
     * 1. 일단 RTT를 기다림, 그 안에 도착하면 RESET 성공
     * 2. RTT를 초과하면 RTO 동안 기다리는 절차로 전환
     * 3. RTO를 초과하면 RESET 실패, 그에 따른 절차수행
     * 이를 구현하면 서로 다른 이벤트 핸들러가 필요하게 되는데, 이벤트 핸들러 간에 순서는 고정이므로
     * 미리 다음 이벤트 핸들러를 예약해 놓는 방식 생각
     */

    Time GetRTT() const;
    Time GetRTO() const;

  private:
    void HandleResetFeedback();
    bool GetSeqBit() const;
    void FlipSeqBit();
    void IncMsgSeq();
    uint8_t GetMsgSeq() const;
    void UpdateRTT(Time new_rtt);
    void UpdateRTO(Time new_rtt);
  };
}    

#endif /* FDP_SENDER_H */
