/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
#ifndef TRACE_TAG_H
#define TRACE_TAG_H

#include "ns3/tag.h"

namespace ns3
{
  class PacketTraceTag : public Tag
  {
  public:
    static TypeId GetTypeId()
    {
      static TypeId tid = TypeId("PacketTraceTag")
        .SetParent<Tag>()
        .AddConstructor<PacketTraceTag>();
      return tid;
    }

    PacketTraceTag():
      m_nodeId{0}, m_packetId{0}
    {
    }

    PacketTraceTag(uint16_t node_id, uint64_t id):
      m_nodeId{node_id}, m_packetId{id}
    {
    }

    TypeId GetInstanceTypeId() const override
    {
      return GetTypeId();
    }

    uint32_t GetSerializedSize() const override
    {
      return sizeof(uint16_t) + sizeof(uint64_t); // Adjust the size as per your tag data
    }

    void Serialize(TagBuffer buffer) const override
    {
      buffer.WriteU16(m_nodeId); // Serialize your tag data
      buffer.WriteU64(m_packetId);
    }

    void Deserialize(TagBuffer buffer) override
    {
      m_nodeId = buffer.ReadU16(); // Deserialize your tag data
      m_packetId = buffer.ReadU64();
    }

    void Print(std::ostream& os) const override
    {
      os << "PacketTraceTag Node Id: " << m_nodeId
         << " Packet Id: " << m_packetId;
    }

    uint64_t GetId() const
    {
      return m_packetId;
    }

    uint16_t GetNodeId() const
    {
      return m_nodeId;
    }

  private:
    uint16_t m_nodeId{0};
    uint64_t m_packetId{0};
  };
}    

#endif /* TRACE_TAG_H */
