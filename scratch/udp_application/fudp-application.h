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

#ifndef FUDP_APPLICATION_H
#define FUDP_APPLICATION_H

#include <memory>

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/type-id.h"

#include "types.h"

using FudpFeature = u32;
constexpr FudpFeature FUDP_FEATURE_ZIGZAG = 1;
constexpr FudpFeature FUDP_FEATURE_HEALTH_PROBE = 2;
constexpr FudpFeature FUDP_FEATURE_NACK_SEQUENCE = 1u << 3;

constexpr bool ContainsZigzag (FudpFeature features)
{
  return (features & FUDP_FEATURE_ZIGZAG) != 0;
}

constexpr bool ContainsHealthProbe (FudpFeature features)
{
  return (features & FUDP_FEATURE_HEALTH_PROBE) != 0;
}

constexpr bool ContainsNackSequence (FudpFeature features)
{
  return (features & FUDP_FEATURE_NACK_SEQUENCE) != 0;
}

class FudpApplication;

class FudpApplicationImpl
{
public:
  FudpApplicationImpl () = default;

  virtual ~FudpApplicationImpl (){};

  FudpApplication &GetContainer ();

  virtual void StartApplication () = 0;

  virtual void StopApplication() = 0;

private:
  void SetContainer (FudpApplication *);

  FudpApplication *_container;

  friend class FudpApplication;
};

class FudpApplication : public ::ns3::Application
{
public:
  FudpApplication () = default;

  ~FudpApplication () = default;

  template <typename T>
  T &GetImpl ();

  void SetImpl (::std::shared_ptr<FudpApplicationImpl>);

  void StartApplication () override final;

  void StopApplication() override final;

  ::ns3::TypeId GetInstanceTypeId () const override;

  static ::ns3::TypeId GetTypeId ();

private:
  ::std::shared_ptr<FudpApplicationImpl> _impl;
};

template <typename T>
T &FudpApplication::GetImpl ()
{
  return *reinterpret_cast<T *> (_impl.get());
}

#endif
