//
// Created by schedule on 6/1/23.
//

#include "MetadataHeader.h"
#include "ns3/address-utils.h"
#include "ns3/buffer.h"
#include "ns3/log.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MetadataHeader");

NS_OBJECT_ENSURE_REGISTERED(MetadataHeader);

MetadataHeader::MetadataHeader()
    : m_left_flow_size(0),
      m_dest_queue(0)
{
}

MetadataHeader::~MetadataHeader()
{}

TypeId
MetadataHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MetadataHeader")
                            .SetParent<Header>()
                            .SetGroupName("Ocs")
                            .AddConstructor<MetadataHeader>();
    return tid;
}

void
MetadataHeader::Print(std::ostream& os) const
{
    os<<"Left flow size "<< m_left_flow_size;
    os<<"entering queue "<< m_dest_queue;
}

uint32_t
MetadataHeader::GetSerializedSize() const
{
    return 6;
}

void
MetadataHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU32(m_left_flow_size);
    i.WriteHtonU16(m_dest_queue);
}

uint32_t
MetadataHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_left_flow_size = i.ReadNtohU32();
    m_dest_queue = i.ReadNtohU16();
    return 6;
}



}