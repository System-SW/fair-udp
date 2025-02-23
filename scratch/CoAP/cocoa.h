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

    class EstimatorImpl
    {
    private:
      Time m_RTT_strong{Seconds(0)};
      Time m_RTT_weak{Seconds(0)};
      Time m_E_strong;
      Time m_E_weak;
      Time m_RTTVAR_strong{Seconds(0)};
      Time m_RTTVAR_weak{Seconds(0)};
    public:
      template <EstimatorType type>
      Time GetRTT() const
      {
        if constexpr (type == EstimatorType::STRONG)
          {
            return m_RTT_strong;
          }
        else
          {
            return m_RTT_weak;
          }
      }

      template <EstimatorType type>
      Time GetRTTVAR() const
      {
        if constexpr (type == EstimatorType::STRONG)
          {
            return m_RTTVAR_strong;
          }
        else
          {
            return m_RTTVAR_weak;
          }
      }

      template <EstimatorType type>
      Time GetE() const
      {
        if constexpr (type == EstimatorType::STRONG)
          {
            return m_E_strong;
          }
        else
          {
            return m_E_weak;
          }
      }

      template <EstimatorType type>
      void UpdatePeriods(Time newRTT)
      {
        constexpr static double alpha = 0.25;
        constexpr static double beta = 0.125;

        if constexpr (type == EstimatorType::STRONG)
          {
            m_RTTVAR_strong = (1 - beta) * GetRTTVAR<type>()
              + beta * Seconds(std::abs((GetRTT<type>() - newRTT).GetSeconds()));
            m_RTT_strong = (1 - alpha) * GetRTT<type>()
              + alpha * newRTT;
            constexpr static unsigned int K = 4;
            m_E_strong = newRTT + K * GetRTTVAR<type>();
          }
        else
          {
            m_RTTVAR_weak = (1 - beta) * GetRTTVAR<type>()
              + beta * Seconds(std::abs((GetRTT<type>() - newRTT).GetSeconds()));
            m_RTT_weak = (1 - alpha) * GetRTT<type>()
              + alpha * newRTT;
            constexpr static unsigned int K = 1;
            m_E_weak = newRTT + K * GetRTTVAR<type>();
          }
      }
    };

    EstimatorImpl m_Impl;
    Time m_OverallRTO{Seconds(2)};

  public:
    Time GetRTO() const
    {
      return m_OverallRTO;
    }

    template <EstimatorType type>
    void UpdatePeriods(Time newRTT)
    {
      m_Impl.UpdatePeriods<type>(newRTT);
      if constexpr (type == EstimatorType::STRONG)
        {
          m_OverallRTO = 0.5 * m_Impl.GetE<type>() + 0.5 * m_OverallRTO;
        }
      else
        {
          m_OverallRTO = 0.25 * m_Impl.GetE<type>() + 0.75 * m_OverallRTO;
        }
    }

    void VariableBackOff()
    {
      double VBF;
      if (m_OverallRTO < Seconds(1))
        VBF = 3;
      else if (m_OverallRTO < Seconds(3))
        VBF = 2;
      else
        VBF = 1.5;
      m_OverallRTO = m_OverallRTO * VBF;
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
    Time GetRTO() const
    {
      return m_Estimator.GetOverallRTO();
    }

    void VariableBackOff()
    {
      m_Estimator.VariableBackOff();
    }
  };
}    

#endif /* COCOA_H */
