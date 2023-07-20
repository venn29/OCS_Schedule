//
// Created by schedule on 6/8/23.
//

#ifndef NS3_OCSUTILS_H
#define NS3_OCSUTILS_H

#include "ns3/tag.h"
#include "ns3/queue-item.h"
namespace ns3
{
class QueueItem;
class OcsTag : public Tag
{
  public:
    OcsTag();
    OcsTag(uint32_t leftsize);
    void SetLeftSize(uint32_t size);
    uint32_t GetLeftSize();
    void SetQueueIdx(uint16_t qidx);
    uint16_t GetQueueIdx();
    //override
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

  private:
    uint32_t leftsize;
    uint16_t queueidx;
};

uint8_t SelectQueueByLeftSize(Ptr<QueueItem> item);


}
#endif // NS3_OCSUTILS_H
