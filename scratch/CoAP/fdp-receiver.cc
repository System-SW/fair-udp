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
FdpReceiverCC::GenerateFeedback(FDPMessageHeader &hdr)
{
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
          return CreateNormalFeedback();
        }
    }

  if (hdr.GetMsgSeq() < 2)      // normal message handling
    {
      return CreateNormalFeedback();
    }
  else                          // handle final message
    {
      // just send the reset feedback!
      // the sender only measures the actual RTT with the reset feedback.
      return CreateFinalFeedback();
    }
  return nullptr;               // no needs to response
}

Ptr<Packet>
FdpReceiverCC::CreateNormalFeedback()
{
  // Time interval = hdr.GetMsgInterval();
  // Time latency_diff = m_RTT / 2 - interval;
  // if (latency_diff > MilliSeconds(10))
  //   {
  //     // feedback to the normal message
  //     return feedback;
  //   }
}

Ptr<Packet>
FdpReceiverCC::CreateFinalFeedback()
{

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
