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
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "fdp-sender.h"
#include "fdp-header.h"
#include "fdp-common.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FdpSenderCC");

FdpSenderCC::FdpSenderCC()
{
}

void
FdpSenderCC::TransferMessage(Ptr<Socket> socket, Ptr<Packet> packet,
                             CoAPHeader &coap_hdr)
{
  NS_LOG_FUNCTION(this);
  Time now = Simulator::Now();
  auto diff = (now - m_PrevTransfer).GetMilliSeconds();
  auto interval = CastMilliSecondsToUint16(diff);
  m_PrevTransfer = now;

  FDPMessageHeader hdr;
  hdr.SetSeqBit(GetSeqBit());
  hdr.SetMsgInterval(MilliSeconds(interval));
  hdr.SetMsgSeq(GetMsgSeq());
  NS_LOG_INFO(__FUNCTION__ << hdr);

  packet->AddHeader(hdr);
  packet->AddHeader(coap_hdr);
  socket->Send(packet);

  IncMsgSeq();
}

EventId
FdpSenderCC::ScheduleTransfer(std::function<void ()> &&callback)
{
  NS_LOG_FUNCTION(this << GetRTT());
  if (GetMsgSeq() == 0)   // just sent third message, so move to reset procedure.
    {
      // do not flip sequence bit till finish reset procedure.
      // if fails to receive reset feedback, then go back to normal status.
      m_ResetEvent =
        Simulator::Schedule(GetRTO(),
                            [this, cb = std::forward<std::function<void()>>(callback)]
                            {
                              FlipSeqBit();
                              m_RTT = GetRTO();
                              UpdateRTO(GetRTT());
                              cb();
                            });
      return m_ResetEvent;
    }
  else
    {
      return Simulator::Schedule(GetRTO(),
                                 std::forward<std::function<void()>>(callback));
    }
}

/*
 * Algorithm 정리
 * 1. 우선 Feedback이 handling할 것인지 확인한다.
 *    1) Sequence Bit가 자신의 값과 동일한가?
 *    2) Message Sequence가 이미 처리한 값 보다 큰 값인가?
 *
 * 2. Client는 일반전송과 RESET 대기의 두가지 상태를 가진다.
 *    1번 조건을 통과한 경우
 *    1) 일반전송 상태
 *       feedback에 기재된 latency 증분과 실제 RTT 간에 최대값으로 RTT와 RTO를 최신화
 *    2) RESET 대기 상태
 *       작성한 알고리즘 대로 동작
 */
void
FdpSenderCC::HandleFeedback(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this);
  FDPFeedbackHeader hdr;
  packet->PeekHeader(hdr);

  if (GetSeqBit() == hdr.GetSeqBit())
    {
      m_recent_feedback_msg_seq = hdr.GetMsgSeq(); // update msg seq
      if (!hdr.GetResetBit()) // normal state
        {
          Time rtt_feed = GetRTT() + 2 * hdr.GetLatency();
          Time diff = Simulator::Now() - m_PrevTransfer;
          Time RTT_x = std::max(rtt_feed, diff);
          UpdateRTT(RTT_x);
          UpdateRTO(RTT_x);
        }
      else // reset state
        {
          // do some job
          if (!m_ResetEvent.IsExpired())
            {
              m_ResetEvent.Cancel();
            }
          HandleResetFeedback();
          // XXX: need to reschedule send event
          FlipSeqBit();
        }
    }
}

Time FdpSenderCC::GetRTT()
{
  if (m_RTT <= MilliSeconds(10))
    {
      m_RTT = MilliSeconds(10);
    }
  return m_RTT;
}

Time FdpSenderCC::GetRTO() const
{
  return m_RTO;
}

void FdpSenderCC::HandleResetFeedback()
{
  NS_LOG_FUNCTION(this);
  Time rtt_act = Simulator::Now() - m_PrevTransfer;
  // NS_ABORT_IF(rtt_act > m_RTO);
  m_RTT = rtt_act;
  UpdateRTO(rtt_act);
}

bool FdpSenderCC::GetSeqBit() const
{
  return m_seq_bit;
}

void FdpSenderCC::FlipSeqBit()
{
  m_seq_bit = !m_seq_bit;
}

void FdpSenderCC::IncMsgSeq()
{
  m_msg_seq++;
  if (m_msg_seq > 2)
    m_msg_seq = 0;
}

uint8_t FdpSenderCC::GetMsgSeq() const
{
  return m_msg_seq;
}

void FdpSenderCC::UpdateRTT(Time new_rtt)
{
  NS_LOG_FUNCTION(this);
  // CoCoA like RTT update.
  constexpr static double alpha = 0.25;
  m_RTT = (1 - alpha) * new_rtt + alpha * new_rtt;
}

void FdpSenderCC::UpdateRTO(Time new_rtt)
{
  NS_LOG_FUNCTION(this);
  // CoCoA like RTO update.
  constexpr static double beta = 0.125;
  m_RTTVAR = (1 - beta) * m_RTTVAR +
    Seconds(beta * std::abs((GetRTT() - new_rtt).GetSeconds()));
  Time RTO_x = GetRTT() + m_RTTVAR;
  m_RTO = 0.25 * RTO_x + 0.75 * m_RTO;
}

