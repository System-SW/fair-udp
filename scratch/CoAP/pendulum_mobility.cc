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
#include <cmath>
#include <ns3/double.h>
#include "pendulum_mobility.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PendulumMobility");
NS_OBJECT_ENSURE_REGISTERED(PendulumMobility);


static Vector
ScaleVector (const double factor, const Vector& vector)
{
  return Vector{vector.x * factor, vector.y * factor, vector.z * factor};
}


TypeId
PendulumMobility::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::PendulumMobility")
    .SetParent<MobilityModel>()
    .SetGroupName("Mobility")
    .AddConstructor<PendulumMobility>()
    .AddAttribute("Period", "Pendulm menuvering period",
                  TimeValue(Seconds(5)),
                  MakeTimeAccessor(&PendulumMobility::period_),
                  MakeTimeChecker())
    .AddAttribute("ApproachRatio", "How close will you get to your destination?",
                  DoubleValue(0.9),
                  MakeDoubleAccessor(&PendulumMobility::approach_ratio_),
                  MakeDoubleChecker<double>())
    .AddAttribute("Destination", "Pendulm menuvering approach point.",
                  VectorValue(Vector{0, 0, 0}),
                  MakeVectorAccessor(&PendulumMobility::terminal_location_),
                  MakeVectorChecker())
    ;
  return tid;
}


PendulumMobility::PendulumMobility ():
  MobilityModel{}
{
  
}


PendulumMobility::~PendulumMobility ()
{
  
}


Vector
PendulumMobility::DoGetPosition () const
{
  Vector distance_vector = ScaleVector(approach_ratio_, terminal_location_ - initial_position_);
  return initial_position_ + Vector{ CalculatePosition(distance_vector.x),
                                     CalculatePosition(distance_vector.y),
                                     CalculatePosition(distance_vector.z) };
}


void
PendulumMobility::DoSetPosition (const Vector& position)
{
  initial_position_ = position;
  start_time_ = Simulator::Now();
}


Vector
PendulumMobility::DoGetVelocity () const
{
  Vector distance_vector = ScaleVector(approach_ratio_, terminal_location_ - initial_position_);
  return Vector{
    CalculateVelocity(distance_vector.x),
    CalculateVelocity(distance_vector.y),
    CalculateVelocity(distance_vector.z)
  };
}


double
PendulumMobility::GetPeriodCoeff () const
{
  constexpr auto pi_2 = 2 * M_PI;
  return pi_2 / period_.GetSeconds();
}


double
PendulumMobility::CalculatePosition (double ele) const
{
  auto coef = GetPeriodCoeff();
  auto diff = (Simulator::Now() - start_time_).GetSeconds();
  return ele * std::sin(diff * coef);
}


double
PendulumMobility::CalculateVelocity (double ele) const
{
  auto coef = GetPeriodCoeff();
  auto diff = (Simulator::Now() - start_time_).GetSeconds();
  return coef * ele * std::cos(diff * coef);
}
