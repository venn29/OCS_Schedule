//
// Created by schedule on 6/8/23.
//

#include "OcsUtils.h"
#include "ns3/packet.h"

namespace ns3
{
OcsTag::OcsTag()
{

}
OcsTag::OcsTag(uint32_t leftsize_initial)
    :leftsize(leftsize_initial)
{

}

void
OcsTag::SetLeftSize(uint32_t size)
{
    this->leftsize = size;
}

uint32_t
OcsTag::GetLeftSize()
{
    return this->leftsize;
}

void
OcsTag::SetQueueIdx(uint16_t qidx)
{
    this->queueidx = qidx;
}

uint16_t
OcsTag::GetQueueIdx()
{
    return this->queueidx;
}

TypeId
OcsTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::OcsTag")
                            .SetParent<Tag>()
                            .SetGroupName("Ocs")
                            .AddConstructor<OcsTag>();
    return tid;
}

TypeId
OcsTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
OcsTag::GetSerializedSize() const
{
    return sizeof(uint32_t)+sizeof(uint16_t);
}

void
OcsTag::Serialize(TagBuffer i) const
{
    i.WriteU32(leftsize);
    i.WriteU16(queueidx);
}

void
OcsTag::Deserialize(TagBuffer i)
{
    leftsize = i.ReadU32();
    queueidx = i.ReadU16();
}

void
OcsTag::Print(std::ostream& os) const
{
    os << "Left Size = " << leftsize << " Queue Idx = "<<queueidx;
}


uint8_t SelectQueueByLeftSize(Ptr<QueueItem> item){
    Ptr<Packet> p = item->GetPacket();
    OcsTag meta;
    bool found = p->PeekPacketTag(meta);
    if(!found)
        return -1;
    else
    {
        return (uint8_t)meta.GetQueueIdx();
    }
}


}