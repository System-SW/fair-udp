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
 */
#ifndef FAIR_UDP_HEADER_H
#define FAIR_UDP_HEADER_H

#include "ns3/header.h"

namespace ns3
{
  class FairUdpHeader : public Header
  {
  public:
    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const override;
    virtual uint32_t GetSerializedSize() const override;
    virtual void Serialize(Buffer::Iterator start) const override;
    virtual uint32_t Deserialize(Buffer::Iterator start) override;
    virtual void Print(std::ostream& os) const override;

    void SetData(uint32_t data);
    uint32_t GetData() const;

  private:
    uint32_t data_;
  };
}    

#endif /* FAIR_UDP_HEADER_H */
