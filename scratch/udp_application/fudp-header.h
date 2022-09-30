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
 * Author: Chang-Hui Kim <kch9001@gmail.com>, Daeseong Ki <kmeos1579@gmail.com>
 */

#ifndef FUDP_HEADER_H
#define FUDP_HEADER_H

#include <limits>
#include <numeric>
#include <iostream>

#include "ns3/header.h"

#include "types.h"

/**
 * | Protocol ID   (32 bit)                                |
 * |-------------------------------------------------------|
 * | 1 bit | 1 bit | 22bit (preserved) |  8 bit (unsigned) |
 * |-------+-------+-------------------+-------------------|
 * | NACK  | RESET |                   | Sequence Number   |
 * |-------------------------------------------------------|
 * | NACK Sequence (32 bit)                                |
 */

using sequence_t = u8;
using nack_seq_t = u32;

struct FudpSeqNumberingMehtod_
{
};

struct FudpSeqNumberingNormal : public FudpSeqNumberingMehtod_
{
  static void Inc (sequence_t &seq_num)
  {
    seq_num += 1;
  }

  static bool Overflowed (sequence_t seq_num)
  {
    return seq_num == 0;
  }
};

struct FudpSeqNumberingZigzag : public FudpSeqNumberingMehtod_
{
  static void Inc (sequence_t &seq_num)
  {
    seq_num += 2;
  }

  static bool Overflowed (sequence_t seq_num)
  {
    return seq_num < 2;
  }
};

template <typename FudpSeqNumberingMethod>
struct FudpSequence
{
  static_assert (::std::is_base_of<FudpSeqNumberingMehtod_, FudpSeqNumberingMethod>::value);

public:
  FudpSequence () = default;

  FudpSequence (sequence_t seq_num) : _seq_num{seq_num}
  {
  }

  FudpSequence (FudpSequence const &) = default;

  FudpSequence &operator= (FudpSequence const &other)
  {
    _seq_num = other._seq_num;
    return *this;
  }

  FudpSequence &operator= (sequence_t v)
  {
    _seq_num = v;
    return *this;
  }

  FudpSequence &operator++ ()
  {
    FudpSeqNumberingMethod::Inc (_seq_num);
    return *this;
  }

  FudpSequence operator++ (int)
  {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool Overflowed () const
  {
    return FudpSeqNumberingMethod::Overflowed (_seq_num);
  }

  sequence_t operator* () const
  {
    return _seq_num;
  }

  sequence_t Get() const {
    return _seq_num;
  }

private:
  sequence_t _seq_num;
};

class FudpHeader : public ::ns3::Header
{
  constexpr static u32 SEQ_MASK = ::std::numeric_limits<u8>::max ();

  constexpr static u32 OPT_MASK = ~SEQ_MASK;

public:
  constexpr static u32 PROTOCOL_ID = 0x12345678;

  enum class Bit : u32 {
    NACK = 1_u32 << 31, // use enclosed sequence number in the next message
    RESET = 1_u32 << 30, // request reset sequence number to 0
  };

  template <Bit BIT>
  void On ()
  {
    _bits |= static_cast<u32> (BIT);
  }

  template <Bit BIT>
  void Off ()
  {
    _bits = _bits & ~static_cast<u32> (BIT);
  }

  template <Bit BIT>
  void Toggle ()
  {
    _bits ^= static_cast<u32> (BIT);
  }

  template <Bit BIT>
  bool Has () const
  {
    return (_bits & static_cast<u32> (BIT)) != 0;
  }

  u32 GetSerializedSize () const override
  {
    return 3 * sizeof (u32);
  }

  void Serialize (::ns3::Buffer::Iterator buf) const override
  {
    buf.WriteU32 (PROTOCOL_ID);
    buf.WriteHtonU32 (_bits);
    buf.WriteHtonU32 (_nack_seq);
  }

  u32 Deserialize (::ns3::Buffer::Iterator buf) override
  {
    NS_ASSERT (PROTOCOL_ID == buf.ReadU32 ());
    _bits = buf.ReadNtohU32 ();
    _nack_seq = buf.ReadNtohU32 ();
    return GetSerializedSize ();
  }

  void Print (::std::ostream &os) const override
  {
    os << " Sequence Number: " << GetSequence () << " NACK=" << Has<Bit::NACK> () << " Reset=" << Has<Bit::RESET> ();
  }

  sequence_t GetSequence () const
  {
    return _bits & SEQ_MASK;
  }

  template <typename FudpSeqMgmtRule>
  FudpSequence<FudpSeqMgmtRule> GetSequence () const
  {
    return GetSequence ();
  }

  void SetSequence (sequence_t seq)
  {
    _bits = (_bits & ~SEQ_MASK) | seq;
  }

  template <typename FudpSeqMgmtRule>
  void SetSequence (FudpSequence<FudpSeqMgmtRule> fudp_seq)
  {
    SetSequence (*fudp_seq);
  }

  nack_seq_t GetNackSequence () const
  {
    return _nack_seq;
  }

  void SetNackSequence (nack_seq_t nack_seq)
  {
    _nack_seq = nack_seq;
  }

  ::ns3::TypeId GetInstanceTypeId () const override
  {
    return GetTypeId ();
  }

  static ::ns3::TypeId GetTypeId ()
  {
    static ::ns3::TypeId tid = ::ns3::TypeId ("FudpHeader").SetParent<::ns3::Header> ().AddConstructor<FudpHeader> ();
    return tid;
  }

private:
  u32 _bits{0};
  nack_seq_t _nack_seq{0};
};

#endif /* FUDP_HEADER_H */
