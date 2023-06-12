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
#ifndef COCOA_H
#define COCOA_H
#include <functional>
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "coap-header.h"

namespace ns3
{
  class Socket;
  class Packet;

  enum class EstimatorType
    {
      STRONG,
      WEAK,
    };

  class Estimator
  {
  private:

    template <unsigned int K>
    class EstimatorImpl
    {
    private:
      Time m_RTT;
      Time m_RTO;
      Time m_RTTVAR{Seconds(0)};
    public:
      EstimatorImpl(Time InitRTT, Time InitRTO):
        m_RTT{InitRTT}, m_RTO{InitRTO}
      {
      }

      Time GetRTT() const
      {
        return m_RTT;
      }

      Time GetRTO() const
      {
        return m_RTO;
      }

      void UpdatePeriods(Time newRTT)
      {
        constexpr static double alpha = 0.25;
        constexpr static double beta = 0.125;
        m_RTT = (1 - alpha) * m_RTT + alpha * newRTT;
        m_RTTVAR = (1 - beta) * m_RTTVAR
          + beta * Seconds(std::abs((m_RTT - newRTT).GetSeconds()));
        m_RTO = m_RTT + K * m_RTTVAR;
      }
    };

    using WeakEstimator = EstimatorImpl<1>;
    using StrongEstimator = EstimatorImpl<4>;


    WeakEstimator m_WeakEst{MilliSeconds(2000), MilliSeconds(2000)};
    StrongEstimator m_StrongEst{MilliSeconds(2000), MilliSeconds(2000)};
    Time m_OverallRTO{Seconds(2)};

  public:
    template <EstimatorType type>
    Time GetRTT() const
    {
      if constexpr (type == EstimatorType::STRONG)
        {
          return m_StrongEst.GetRTT();
        }
      else
        {
          return m_WeakEst.GetRTT();
        }
    }

    template <EstimatorType type>
    Time GetRTO() const
    {
      if constexpr (type == EstimatorType::STRONG)
        {
          return m_StrongEst.GetRTO();
        }
      else
        {
          return m_WeakEst.GetRTO();
        }
    }

    template <EstimatorType type>
    void UpdatePeriods(Time newRTT)
    {
      if constexpr (type == EstimatorType::STRONG)
        {
          m_StrongEst.UpdatePeriods(newRTT);
          m_OverallRTO = 0.5 * m_StrongEst.GetRTO() + 0.5 * m_OverallRTO;
        }
      else
        {
          m_WeakEst.UpdatePeriods(newRTT);
          m_OverallRTO = 0.25 * m_WeakEst.GetRTO() + 0.75 * m_OverallRTO;
        }
    }

    Time GetRetransmissionRTO() const
    {
      double VBF;
      if (m_OverallRTO < Seconds(1))
        VBF = 3;
      else if (m_OverallRTO < Seconds(3))
        VBF = 2;
      else
        VBF = 1.5;
      return VBF * m_OverallRTO;
    }

    Time GetOverallRTO() const
    {
      return m_OverallRTO;
    }
  };

  /*
   * CoAP의 CON 형식 통신시 RTO 이내로 ACK를 수신하지 못하면 메시지를 재전송한다.
   * 재전송 횟수는 사용자가 설정할 수 있는데, 기본적인 값은 네번이다.
   *
   * CoCoA는 ACK를 수신하는 시점을 최초전송, 재전송, 두번 이상의 재전송을 구분하여 RTT, RTO를 갱신한다.
   *
   * 1. 최초전송 후 ACK 수신
   *    CoCoA는 CON 메시지 최초전송 후 RTO 동안 ACK를 기다린다.
   *    시간 내에 ACK를 수신하면 측정한 RTT로 Strong 계산식을 이용해 RTT, RTO를 갱신한다.
   *    만약 RTO 이내로 ACK를 수신하지 못하면 메시지를 재전송한다.
   *
   * 2. 재전송 후 ACK 수신
   *    1번 조건으로 인해 메시지를 재전송한 후 다시 RTO 만큼 ACK를 기다린다.
   *    시간 내에 ACK를 수신하면 측정한 RTT로 Weak 계산식을 이용해 RTT, RTO를 갱신한다.
   *    만약 RTO 이내로 ACK를 수신하지 못하면 메시지를 재전송한다.
   *
   * 3. 두번째 재전송 부터의 동작
   *    2번 조건으로 인해 메시지를 재전송하는 시점으로 부터 더 이상의 RTT, RTO 갱신은 수행하지 않는다.
   *    대신 RTO RE라는 값을 RTO의 VBF 만큼의 배수로 계산하여 재전송을 수행한다.
   *    메시지 재전송은 CoAP의 최재 재전송 횟수까지만 전송한다.
   *    두번째 메시지 재전송 이후 CoCoA는 ACK 수신으로 측정한 RTT 값을 사용하지 않는다.
   *    그 이유는 수신한 ACK가 첫번째 메시지로 발생한 것인지 알 수 없기 때문이다.
   */

  class CoCoA
  {
  private:
    constexpr static uint32_t MAX_TRANSMIT_TIME = 4;
    Estimator m_Estimator;
    std::size_t m_TransmitCounter{0};
    std::function<void(Ptr<Packet>)> m_SendPacketFunction;

    // transfer NON msg with m_RTO interval
    void TransferNON(Ptr<Packet> packet);

  public:
    CoCoA(std::function<void(Ptr<Packet>)> &&SendPacketFunction);

  private:

    // for CON transfer
    Time m_ConStart;
    uint32_t m_TC{0};
    uint32_t m_RC{0};  // Retransmission counter
    EventId m_AckWaitEvent;
    Ptr<Packet> m_ConPacket{nullptr};
    std::function<void(void)> m_Context; // Return to NON context

    void TransferCON(Ptr<Packet> packet);
    void Retransmit();
    void TerminalTransmit();
    void ClearConStates();

    void IncTC();
    uint32_t GetTC() const;
    void IncRC();
    uint32_t GetRC() const;
    void SetConPacket(Ptr<Packet> packet);
    Ptr<Packet> GetConPacket() const;

  public:
    void TransferMsg(Ptr<Packet> packet,
                     std::function<void(void)> &&context = [](){});
    void NotifyACK(Ptr<Packet> ack);
  };
}    

#endif /* COCOA_H */
