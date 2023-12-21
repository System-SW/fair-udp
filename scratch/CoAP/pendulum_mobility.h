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
#ifndef PENDULUM_MOBILITY_H
#define PENDULUM_MOBILITY_H

#include <ns3/mobility-model.h>
#include <ns3/simulator.h>

namespace ns3
{
  class PendulumMobility : public MobilityModel
  {
  public:
    static TypeId GetTypeId (void);

    PendulumMobility ();

    ~PendulumMobility () override;

    Vector DoGetPosition () const override;

    void DoSetPosition (const Vector& position) override;

    Vector DoGetVelocity () const override;

  private:
    double GetPeriodCoeff () const;
    double CalculatePosition (double ele) const;
    double CalculateVelocity (double ele) const;


    Vector initial_position_{0, 0, 0};
    Vector terminal_location_{0, 0, 0};
    Time start_time_{0};
    Time period_{0};
    double approach_ratio_{0.9};
  };

}    


#endif /* PENDULUM_MOBILITY_H */
