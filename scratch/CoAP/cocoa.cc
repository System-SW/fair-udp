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
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "cocoa.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CoCoA");


CoCoA::CoCoA(std::function<void(Ptr<Packet>)> &&SendPacketFunction)
{
  NS_LOG_FUNCTION(this);

  m_SendPacketFunction =
    std::forward<std::function<void(Ptr<Packet>)>>(SendPacketFunction);
}

void
CoCoA::TransferNON(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this);

  CoAPHeader hdr;
  packet->RemoveHeader(hdr);
  hdr.SetType(CoAPHeader::Type::NON);
  packet->AddHeader(hdr);

  m_SendPacketFunction(packet);

  IncTC();

  m_Context();      // go back to context
}

void
CoCoA::TransferCON(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this);

  CoAPHeader hdr;
  packet->RemoveHeader(hdr);
  hdr.SetType(CoAPHeader::Type::CON);
  packet->AddHeader(hdr);

  m_ConStart = Simulator::Now();
  m_RC = 0;

  m_SendPacketFunction(packet);

  // may needs deep copy, but now just do shallow copy
  SetConPacket(packet);

  IncTC();
  m_AckWaitEvent = Simulator::Schedule(m_Estimator.GetRTO<EstimatorType::STRONG>(),
                                       MakeCallback(&CoCoA::Retransmit, this));
}

// ACK didn't arrive within RTO, so do the first retransmit
void
CoCoA::Retransmit()
{
  NS_LOG_FUNCTION(this);

  IncRC();
  m_SendPacketFunction(GetConPacket());
  m_AckWaitEvent = Simulator::Schedule(m_Estimator.GetRTO<EstimatorType::WEAK>(),
                                       MakeCallback(&CoCoA::TerminalTransmit, this));
}

void
CoCoA::TerminalTransmit()
{
  NS_LOG_FUNCTION(this);

  IncRC();
  if (GetRC() > MAX_TRANSMIT_TIME)
    {
      // stop retransmit
      // may reschedule NON transfer event.
      ClearConStates();
      m_Context();
      return;
    }

  m_SendPacketFunction(GetConPacket()); // retransmission
  m_AckWaitEvent = Simulator::Schedule(m_Estimator.GetRetransmissionRTO(),
                                       MakeCallback(&CoCoA::TerminalTransmit, this));
}

void
CoCoA::ClearConStates()
{
  NS_LOG_FUNCTION(this);

  m_AckWaitEvent.Cancel();
  m_ConStart = Seconds(0);
  m_RC = 0;
  SetConPacket(nullptr);  // free packet
}

void
CoCoA::IncTC()
{
  m_TC++;
}

uint32_t
CoCoA::GetTC() const
{
  return m_TC;
}

void
CoCoA::IncRC()
{
  m_RC++;
}

uint32_t
CoCoA::GetRC() const
{
  return m_RC;
}

void
CoCoA::SetConPacket(Ptr<Packet> packet)
{
  m_ConPacket = packet;
}

Ptr<Packet>
CoCoA::GetConPacket() const
{
  return m_ConPacket;
}

void
CoCoA::TransferMsg(Ptr<Packet> packet, std::function<void(void)> &&context)
{
  m_Context = std::forward<std::function<void(void)>>(context);
  if (GetTC() % 8 != 0)
    {
      TransferNON(packet);
    }
  else
    {
      TransferCON(packet);
    }
}

// ACK handler MUST check that the coap client is on CON transmission or not.
void
CoCoA::NotifyACK(Ptr<Packet> ack)
{
  if (m_ConStart == Seconds(0)) // its on NON procedure
    return;                     // ignore

  ClearConStates();
  switch (GetRC())
    {
    case 0:                       // normal
      {
        auto newRTT = Simulator::Now() - m_ConStart;
        m_Estimator.UpdatePeriods<EstimatorType::STRONG>(newRTT);
        break;
      }
    case 1:                       // retransmission
      {
        auto newRTT = Simulator::Now() - m_ConStart;
        m_Estimator.UpdatePeriods<EstimatorType::WEAK>(newRTT);
        break;
      }
    default:                      // ignore
      break;
    }
  m_Context();            // go back to context
}
