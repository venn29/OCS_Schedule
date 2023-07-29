//
// Created by venn on 23-7-26.
//

#ifndef NS3_PFIFOFLOWSIZE_H
#define NS3_PFIFOFLOWSIZE_H

#include "ns3/queue-disc.h"

namespace ns3
{
class PFifoFlowSizeQueueDisc : public QueueDisc
{
  public:
    static TypeId GetTypeId();
    PFifoFlowSizeQueueDisc();
    PFifoFlowSizeQueueDisc(QueueSize qsz);
    ~PFifoFlowSizeQueueDisc() override;

    static constexpr const char* LIMIT_EXCEEDED_DROP =
        "Queue disc limit exceeded"; //!< Packet dropped due to queue disc limit exceeded

  private:
    uint32_t flowsizethresh ;

    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;
};
}
#endif // NS3_PFIFOFLOWSIZE_H
