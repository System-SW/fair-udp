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
 * Author: Chang-Hui Kim <kch9001@gmail.com>, Daeseong-Ki <kmeos1579@gmail.com>
 */

#ifndef FUDP_CLIENT_H
#define FUDP_CLIENT_H

#include <array>
#include <cstdlib>
#include <type_traits>

#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/log.h"

#include "congestion-control.h"
#include "fudp-application.h"
#include "fudp-header.h"

template <bool>
struct FudpClientSequenceState
{
  FudpSequence<FudpSeqNumberingNormal> sequence;
};

template <>
struct FudpClientSequenceState<true>
{
  FudpSequence<FudpSeqNumberingZigzag> sequence;
};

template <bool>
struct FudpClientHealthProbeState
{
};

template <>
struct FudpClientHealthProbeState<true>
{
  bool healthy = false;
};

template <bool>
struct FudpNackSequenceState
{
};

template <>
struct FudpNackSequenceState<true>
{
  nack_seq_t nack_seq{0};
};

template <FudpFeature FEATURES>
struct FudpClientState : public FudpClientSequenceState<ContainsZigzag (FEATURES)>,
                         public FudpClientHealthProbeState<ContainsHealthProbe (FEATURES)>,
                         public FudpNackSequenceState<ContainsNackSequence (FEATURES)>
{
  ::ns3::CongestionInfo congestionInfo;
  bool terminated = false;
};

template <FudpFeature FEATURES>
class FudpClient : public FudpApplicationImpl
{
public:
  FudpClient () = default;

  FudpClient (FudpClient const &) = delete;

  ~FudpClient () = default;

  ::ns3::Address const &GetServerAddress () const;

  void SetServerAddress (::ns3::Address const &);

  u16 GetServerPort () const;

  void SetServerPort (u16);

  ::ns3::Socket &GetSocket ();

  FudpClientState<FEATURES> &GetState ();

  void ScheduleTraffic ();

  void SendTraffic ();

  void StartApplication () override;

  void StopApplication () override;

private:
  void OnRecv (::ns3::Ptr<::ns3::Socket>);

  ::ns3::Address _serverAddress;

  u16 _serverPort;

  ::ns3::Ptr<::ns3::Socket> _socket;

  FudpClientState<FEATURES> _state;
};

template <FudpFeature FEATURES>
FudpClientState<FEATURES> &FudpClient<FEATURES>::GetState ()
{
  return _state;
}

template <FudpFeature FEATURES>
::ns3::Socket &FudpClient<FEATURES>::GetSocket ()
{
  return *_socket;
}

template <FudpFeature FEATURES>
u16 FudpClient<FEATURES>::GetServerPort () const
{
  return _serverPort;
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::SetServerPort (u16 newServerPort)
{
  _serverPort = newServerPort;
}

template <FudpFeature FEATURES>
::ns3::Address const &FudpClient<FEATURES>::GetServerAddress () const
{
  return _serverAddress;
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::SetServerAddress (::ns3::Address const &addr)
{
  _serverAddress = addr;
}

template <FudpFeature FEATURES, ::std::enable_if_t<!ContainsHealthProbe (FEATURES), int> = 0>
void HandleOverflow (FudpClientState<FEATURES> &)
{
  // do nothing
}

template <FudpFeature FEATURES, ::std::enable_if_t<ContainsHealthProbe (FEATURES), int> = 0>
void HandleOverflow (FudpClientState<FEATURES> &state)
{
  if (state.sequence.Overflowed ())
    {
      if (!state.healthy)
        {
          state.congestionInfo.ReduceBandwidth ();
        }
      state.healthy = false;
    }
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::ScheduleTraffic ()
{
  if (GetState ().terminated)
    {
      return;
    }

  auto const afterMs = GetState ().congestionInfo.GetTransferInterval ();
  ::ns3::Simulator::Schedule (::ns3::MilliSeconds (afterMs), &FudpClient<FEATURES>::SendTraffic, this);
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::SendTraffic ()
{
  static auto dummyData = ::std::array<u8, 1024>{};

  auto header = FudpHeader{};
  header.SetSequence (GetState ().sequence++);

  if constexpr (ContainsNackSequence (FEATURES))
    {
      header.SetNackSequence (GetState ().nack_seq);
    }

  auto packet = ::ns3::Create<::ns3::Packet> (dummyData.data (), dummyData.size ());
  packet->AddHeader (header);

  GetSocket ().SendTo (packet, 0, ::ns3::InetSocketAddress::ConvertFrom (GetServerAddress ()));
  HandleOverflow<FEATURES> (GetState ());

  ScheduleTraffic ();
}

template <FudpFeature FEATURES, ::std::enable_if_t<!ContainsZigzag (FEATURES) &&
                                                   !ContainsNackSequence (FEATURES), int> = 0>
void HandleNACK (FudpClientState<FEATURES> &state, FudpHeader const &header)
{
  state.sequence = header.GetSequence ();
  state.congestionInfo.PacketDropDetected (state.sequence.Get ());
}

template <FudpFeature FEATURES, ::std::enable_if_t<ContainsZigzag (FEATURES) &&
                                                   !ContainsNackSequence (FEATURES), int> = 0>
void HandleNACK (FudpClientState<FEATURES> &state, FudpHeader const &header)
{
  if (((state.sequence.Get () ^ header.GetSequence ()) & 1) != 0)
    {
      state.sequence = header.GetSequence ();
      state.congestionInfo.PacketDropDetected (state.sequence.Get ());
    }
}

template <FudpFeature FEATURES, ::std::enable_if_t<!ContainsZigzag (FEATURES) &&
                                                   ContainsNackSequence (FEATURES), int> = 0>
void HandleNACK (FudpClientState<FEATURES> &state, FudpHeader const &header)
{
  if (state.nack_seq == header.GetNackSequence ())
    {
      return;
    }
  else if (state.nack_seq < header.GetNackSequence ())
    {
      state.sequence = header.GetSequence ();
      state.nack_seq = header.GetNackSequence ();
      state.congestionInfo.ReduceBandwidth ();
    }
}

template <FudpFeature FEATURES, ::std::enable_if_t<ContainsHealthProbe (FEATURES), int> = 0>
void HandleRESET (FudpClientState<FEATURES> &state, FudpHeader const &header)
{
  state.healthy = (state.healthy || header.Has<FudpHeader::Bit::RESET> ());
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::OnRecv (::ns3::Ptr<::ns3::Socket> socket)
{
  auto address = ::ns3::Address{};
  if (auto packet = _socket->RecvFrom (address))
    {
      auto header = FudpHeader{};
      packet->RemoveHeader (header);

      if (header.Has<FudpHeader::Bit::NACK> ())
        {
          HandleNACK (GetState (), header);
          return;
        }

      if constexpr (ContainsHealthProbe (FEATURES))
        if (header.Has<FudpHeader::Bit::RESET> ())
          {
            HandleRESET (GetState (), header);
          }
    }
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::StartApplication ()
{
  auto const tid = ::ns3::TypeId::LookupByName ("ns3::UdpSocketFactory");
  _socket = ::ns3::Socket::CreateSocket (GetContainer ().GetNode (), tid);
  if (_socket->Bind (::ns3::InetSocketAddress{::ns3::Ipv4Address::GetAny (), GetServerPort ()}) == -1)
    {
      NS_FATAL_ERROR ("failed to bind socket");
    }

  _socket->SetRecvCallback (::ns3::MakeCallback (&FudpClient<FEATURES>::OnRecv, this));

  GetState ().terminated = false;
}

template <FudpFeature FEATURES>
void FudpClient<FEATURES>::StopApplication ()
{
  GetState ().terminated = true;
}

#endif
