//
// Created by schedule on 6/1/23.
//

#ifndef NS3_METADATAHEADER_H
#define NS3_METADATAHEADER_H
#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-header.h"
namespace ns3
{
class MetadataHeader : public Header
{
  public:
    MetadataHeader();
    ~MetadataHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint16_t GetDestQueue(){
        return this->m_dest_queue;
    }

    uint32_t GetLeftFlowSize(){
        return this->m_left_flow_size;
    }

    void SetDestQueue(uint16_t dest_queue){
        this->m_dest_queue = dest_queue;
    }

    void SetLeftFlowSize(uint32_t flow_size){
        this->m_left_flow_size = flow_size;
    }

  private:
    uint32_t m_left_flow_size;
    uint16_t m_dest_queue;
};
}

#endif // NS3_METADATAHEADER_H
