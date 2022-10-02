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

#ifndef FUDP_SERVER_H
#define FUDP_SERVER_H

#include <iostream>
#include "ns3/abort.h"
#include "ns3/address.h"
#include "ns3/fatal-error.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"

#include <functional>
#include <type_traits>
#include <unordered_map>

#include "fudp-application.h"
#include "fudp-client.h"
#include "fudp-header.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "optref.h"

template <FudpFeature FEATURES, bool = ContainsNackSequence (FEATURES)>
struct NackStates;

template <FudpFeature FEATURES>
struct NackStates<FEATURES, true>
{
  enum class Status
    {
      OK,
      NEW_NACK,
      SAME_NACK,
    };
};

template <FudpFeature FEATURES>
using Status = typename NackStates<FEATURES>::Status;

template <FudpFeature FEATURES>
struct FudpConnection : public FudpClientSequenceState<ContainsZigzag (FEATURES)>,
                        public FudpNackSequenceState<ContainsNackSequence (FEATURES)>
{
};

template <FudpFeature FEATURES>
class FudpServer : public FudpApplicationImpl
{
  struct AddressHash
  {
    sz operator() (::ns3::Address const &address) const
    {
      NS_ABORT_IF (!::ns3::InetSocketAddress::IsMatchingType (address));
      return ::std::hash<u32> () (::ns3::InetSocketAddress::ConvertFrom (address).GetIpv4 ().Get ());
    }
  };

public:
  FudpServer () = default;

  ~FudpServer () = default;

  u16 GetServerPort () const;

  void SetServerPort (u16);

  void EstablishConnection (::ns3::Address const &);

  OptRef<FudpConnection<FEATURES>> GetConnection (::ns3::Address const &);

  void StartApplication () override;

  void StopApplication () override{};

private:
  void SendNACK (::ns3::Address const &);

  void SendHealthProbe (::ns3::Address const &);

private:
  void OnRecv (::ns3::Ptr<::ns3::Socket> socket);

  u16 _serverPort;

  ::ns3::Ptr<::ns3::Socket> _socket;

  ::std::unordered_map<::ns3::Address, FudpConnection<FEATURES>, AddressHash> _connections;
};

template <FudpFeature FEATURES>
u16 FudpServer<FEATURES>::GetServerPort () const
{
  return _serverPort;
}

template <FudpFeature FEATURES>
void FudpServer<FEATURES>::SetServerPort (u16 newServerPort)
{
  _serverPort = newServerPort;
}

template <FudpFeature FEATURES>
void FudpServer<FEATURES>::EstablishConnection (::ns3::Address const &address)
{
  _connections[address] = {};
}

template <FudpFeature FEATURES>
OptRef<FudpConnection<FEATURES>> FudpServer<FEATURES>::GetConnection (::ns3::Address const &address)
{
  auto const iter = _connections.find (address);
  if (iter == _connections.end ())
    {
      return ::std::nullopt;
    }

  return iter->second;
}

template <FudpFeature FEATURES>
void FudpServer<FEATURES>::SendNACK (::ns3::Address const &dest)
{
  auto &connection = static_cast<FudpConnection<FEATURES> &> (*GetConnection (dest));

  auto header = FudpHeader{};
  header.On<FudpHeader::Bit::NACK> ();
  header.SetSequence (connection.sequence);

  if constexpr (ContainsNackSequence (FEATURES))
    {
      header.SetNackSequence (connection.nack_seq);
    }

  auto packet = ::ns3::Create<::ns3::Packet> ();
  packet->AddHeader (header);

  NS_ABORT_IF (!::ns3::InetSocketAddress::IsMatchingType (dest));
  _socket->SendTo (packet, 0, ::ns3::InetSocketAddress::ConvertFrom (dest));
}

template <FudpFeature FEATURES>
void FudpServer<FEATURES>::SendHealthProbe (::ns3::Address const &dest)
{
  auto header = FudpHeader{};
  header.On<FudpHeader::Bit::RESET> ();

  auto packet = ::ns3::Create<::ns3::Packet> ();
  packet->AddHeader (header);

  NS_ABORT_IF (!::ns3::InetSocketAddress::IsMatchingType (dest));
  _socket->SendTo (packet, 0, ::ns3::InetSocketAddress::ConvertFrom (dest));
}

template <FudpFeature FEATURES, ::std::enable_if_t<!ContainsZigzag (FEATURES) &&
                                                   !ContainsNackSequence (FEATURES), int> = 0>
bool ValidateHeader (FudpConnection<FEATURES> const &connection, FudpHeader const &header)
{
  return connection.sequence.Get () == header.GetSequence ();
}

template <FudpFeature FEATURES, ::std::enable_if_t<ContainsZigzag (FEATURES) &&
                                                   !ContainsNackSequence (FEATURES), int> = 0>
bool ValidateHeader (FudpConnection<FEATURES> const &connection, FudpHeader const &header)
{
  return (((connection.sequence.Get () ^ header.GetSequence ()) & 1) != 0) ||
         (connection.sequence.Get () == header.GetSequence ());
}

template <FudpFeature FEATURES, ::std::enable_if_t<!ContainsZigzag (FEATURES) &&
                                                   ContainsNackSequence (FEATURES), int> = 0>
Status<FEATURES> ValidateHeader (const FudpConnection<FEATURES> &connection, const FudpHeader &header)
{
  if (connection.sequence.Get () == header.GetSequence ())
    {
      if (connection.nack_seq == header.GetNackSequence ())
        {
          return Status<FEATURES>::OK;
        }
      else
        {
          return Status<FEATURES>::SAME_NACK;
        }
    }
  return Status<FEATURES>::NEW_NACK;
}

template <FudpFeature FEATURES>
void FudpServer<FEATURES>::OnRecv (::ns3::Ptr<::ns3::Socket> socket)
{
  auto address = ::ns3::Address{};
  if (auto packet = _socket->RecvFrom (address))
    {
      if (!GetConnection (address))
        {
          // NS_LOG_DEBUG ("Connected");
          EstablishConnection (address);
        }

      auto &connection = static_cast<FudpConnection<FEATURES> &> (*GetConnection (address));

      auto header = FudpHeader{};
      packet->RemoveHeader (header);

      if constexpr (ContainsNackSequence (FEATURES))
        {
          switch (ValidateHeader(connection, header))
            {
            case Status<FEATURES>::OK:
              connection.sequence++;
              break;
            case Status<FEATURES>::NEW_NACK:
                connection.nack_seq++;
            case Status<FEATURES>::SAME_NACK:
              {
                connection.sequence = header.GetSequence () + 1;
                SendNACK (address);
              }
              break;
            default:
              break;
            }
        }
      else
        {
          if (!ValidateHeader (connection, header))
            {
              if constexpr (ContainsZigzag (FEATURES))
                {
                  connection.sequence = connection.sequence.Get () + 1;
                }

              SendNACK (address);
            }
          else
            {
              ++connection.sequence;
            }
        }

      if constexpr (ContainsHealthProbe (FEATURES))
        {
          if (connection.sequence.Overflowed ())
            {
              SendHealthProbe (address);
            }
        }
    }
}

template <FudpFeature FEATURES>
void FudpServer<FEATURES>::StartApplication ()
{
  auto const tid = ::ns3::TypeId::LookupByName ("ns3::UdpSocketFactory");
  _socket = ::ns3::Socket::CreateSocket (GetContainer ().GetNode (), tid);
  if (_socket->Bind (::ns3::InetSocketAddress (::ns3::Ipv4Address::GetAny (), GetServerPort ())))
    {
      NS_FATAL_ERROR ("failed to bind socket");
    }

  _socket->SetRecvCallback (::ns3::MakeCallback (&FudpServer<FEATURES>::OnRecv, this));
}

#endif
