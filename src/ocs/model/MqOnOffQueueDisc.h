//
// Created by schedule on 6/1/23.
//

#ifndef NS3_ONOFFQUEUEDISC_H
#define NS3_ONOFFQUEUEDISC_H

#include "ns3/queue-disc.h"

namespace ns3
{

class MqOnOffQueueDisc : public QueueDisc
{
  public:
    static TypeId GetTypeId();

    MqOnOffQueueDisc();

    ~MqOnOffQueueDisc();

    void SetWorkingQueueIdx(uint32_t idx);

    uint32_t GetWorkingQueueIdx();

    uint32_t GetQueueSizeN(uint32_t n);

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;

    //find the queue the packet should be enqueued
    uint32_t SearchQueueIdx(Ptr<QueueDiscItem> item);
    //
    uint32_t working_queue_idx;
};

} // namespace ns3

#endif // NS3_ONOFFQUEUEDISC_H
