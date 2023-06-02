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
#include "fdp-receiver.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FdpReceiverCC");

Ptr<Packet>
FdpReceiverCC::GenerateFeedback(const FDPMessageHeader &hdr)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO(__FUNCTION__ << hdr);
  // XXX: update PrevArrival, m_RTT, m_seq_bit, m_msg_seq... ect
  if (GetSeqBit() != hdr.GetSeqBit()) // different seq bit!
    {
      if (hdr.GetMsgSeq() == 2)  // delayed final message, ignore it
        {
          // ignore
          return nullptr;
        }
      else
        {
          FlipSeqBit();         // final message may lossed
          return CreateNormalFeedback(hdr);
        }
    }

  if (hdr.GetMsgSeq() < 2)      // normal message handling
    {
      return CreateNormalFeedback(hdr);
    }
  else if (hdr.GetMsgSeq() == 2)  // handle final message
    {
      // just send the reset feedback!
      // the sender only measures the actual RTT with the reset feedback.
      auto reset_feedback = CreateFinalFeedback();
      FlipSeqBit();
      return reset_feedback;
    }
  return nullptr;               // no needs to response
}

Ptr<Packet>
FdpReceiverCC::CreateNormalFeedback(const FDPMessageHeader &hdr)
{
  // XXX: update m_RTT, m_seq_bit, m_msg_seq... ect
  m_RTT = Simulator::Now() - m_PrevArrival;
  m_PrevArrival = Simulator::Now();
  Time interval = hdr.GetMsgInterval();
  Time latency_diff = m_RTT / 2 - interval;
  if (latency_diff > MilliSeconds(10))
    {
      // feedback to the normal message
      auto feedback = Create<Packet>();
      FDPFeedbackHeader feedback_hdr;
      feedback_hdr.OffResetBit();
      feedback_hdr.SetSeqBit(GetSeqBit());
      feedback_hdr.SetMsgSeq(hdr.GetMsgSeq());
      feedback_hdr.SetLatency(latency_diff);
      feedback->AddHeader(feedback_hdr);
      return feedback;
    }
  return nullptr;               // no needs to response
}

Ptr<Packet>
FdpReceiverCC::CreateFinalFeedback()
{
  m_RTT = Simulator::Now() - m_PrevArrival;
  m_PrevArrival = Simulator::Now();
  // create the reset feedback packet and return it
  Ptr<Packet> feedback = Create<Packet>();
  FDPFeedbackHeader reset_hdr;
  reset_hdr.OnResetBit();
  reset_hdr.SetSeqBit(GetSeqBit());
  reset_hdr.SetMsgSeq(GetMsgSeq());
  feedback->AddHeader(reset_hdr);
  return feedback;
}

bool
FdpReceiverCC::GetSeqBit() const
{
  return m_seq_bit;
}

void
FdpReceiverCC::FlipSeqBit()
{
  m_seq_bit = !m_seq_bit;
}

uint8_t FdpReceiverCC::GetMsgSeq() const
{
  return m_msg_seq;
}

void FdpReceiverCC::SetMsgSeq(uint8_t msg_seq)
{
  NS_ABORT_IF(msg_seq > 2);
  m_msg_seq = msg_seq;
}
