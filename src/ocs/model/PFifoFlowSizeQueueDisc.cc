//
// Created by venn on 23-7-26.
//

#include "PFifoFlowSizeQueueDisc.h"
#include "OcsUtils.h"

#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/socket.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("PFiflFlowSizeQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(PFifoFlowSizeQueueDisc);

TypeId
PFifoFlowSizeQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PFifoFlowSizeQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("Ocs")
            .AddConstructor<PFifoFlowSizeQueueDisc>()
            .AddAttribute("MaxSize",
                          "The maximum number of packets accepted by this queue disc.",
                          QueueSizeValue(QueueSize("10000B")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker());
    return tid;
}

PFifoFlowSizeQueueDisc::PFifoFlowSizeQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::BYTES)
{
    QueueDisc::SetMaxSize(QueueSize("150000B"));
    this->flowsizethresh = 50*1500;
    NS_LOG_FUNCTION(this);
}

PFifoFlowSizeQueueDisc::PFifoFlowSizeQueueDisc(QueueSize qsz)
    : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::BYTES)
{
    QueueDisc::SetMaxSize(qsz);
    NS_LOG_FUNCTION(this);
}


PFifoFlowSizeQueueDisc::~PFifoFlowSizeQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

bool
PFifoFlowSizeQueueDisc::DoEnqueue(Ptr<ns3::QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this<<item);
    if (GetCurrentSize() >= GetMaxSize())
    {
        NS_LOG_LOGIC("Queue disc limit exceeded -- dropping packet");
        DropBeforeEnqueue(item, LIMIT_EXCEEDED_DROP);
        return false;
    }
    OcsTag metaData;
    uint32_t band = 0;
    bool tag_found = item->GetPacket()->RemovePacketTag(metaData);
    if(tag_found)
    {
        uint32_t  size = metaData.GetLeftSize();
        if(size > this->flowsizethresh)
        {
            band = 1;
            std::cout<<"enqueue band2"<<std::endl;
        }
    }
    bool retval = GetInternalQueue(band)->Enqueue(item);
    if(!retval)
    {
        NS_LOG_WARN("Packet enqueue failed. Check the size of the internal queues");
    }
    NS_LOG_LOGIC("Number packets band " << band << ": " << GetInternalQueue(band)->GetNPackets());
    return retval;
}

Ptr<QueueDiscItem>
PFifoFlowSizeQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);
    Ptr<QueueDiscItem> item;
    for (uint32_t i = 0; i < GetNInternalQueues(); i++)
    {
        if ((item = GetInternalQueue(i)->Dequeue()))
        {
            NS_LOG_LOGIC("Popped from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band " << i << ": " << GetInternalQueue(i)->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem>
PFifoFlowSizeQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNInternalQueues(); i++)
    {
        if ((item = GetInternalQueue(i)->Peek()))
        {
            NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band " << i << ": " << GetInternalQueue(i)->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
PFifoFlowSizeQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if(GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("PfifoFastQueueDisc cannot have classes");
        return false;
    }

    if(GetNPacketFilters() != 0)
    {
        NS_LOG_ERROR("PfifoFastQueueDisc needs no packet filter");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // create 2 DropTail queues with GetLimit() packets each
        ObjectFactory factory;
        factory.SetTypeId("ns3::DropTailQueue<QueueDiscItem>");
        factory.Set("MaxSize", QueueSizeValue(QueueSize("75000B")));
        AddInternalQueue(factory.Create<InternalQueue>());
        AddInternalQueue(factory.Create<InternalQueue>());
    }

    if (GetNInternalQueues() != 2)
    {
        NS_LOG_ERROR("PfifoFastQueueDisc needs 2 internal queues");
        return false;
    }
    if (GetInternalQueue(0)->GetMaxSize().GetUnit() != QueueSizeUnit::BYTES ||
        GetInternalQueue(1)->GetMaxSize().GetUnit() != QueueSizeUnit::BYTES )
    {
        NS_LOG_ERROR("PfifoFastQueueDisc needs 2 internal queues operating in packet mode");
        return false;
    }

    for (uint8_t i = 0; i < 2; i++)
    {
        if (GetInternalQueue(i)->GetMaxSize() < GetMaxSize())
        {
            NS_LOG_ERROR(
                "The capacity of some internal queue(s) is less than the queue disc capacity");
            return false;
        }
    }

    return true;
}

void
PFifoFlowSizeQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

}