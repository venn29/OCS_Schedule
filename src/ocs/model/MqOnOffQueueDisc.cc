//
// Created by schedule on 6/1/23.
//

#include "MqOnOffQueueDisc.h"
#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/socket.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include <algorithm>
#include <iterator>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MqOnOffQueuedDisc");

NS_OBJECT_ENSURE_REGISTERED(MqOnOffQueueDisc);
TypeId
MqOnOffQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MqOnOffQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("Ocs")
            .AddConstructor<MqOnOffQueueDisc>();
    return tid;
}

MqOnOffQueueDisc::MqOnOffQueueDisc():
      QueueDisc(QueueDiscSizePolicy::NO_LIMITS),
      working_queue_idx(0)
{
    NS_LOG_FUNCTION(this);
}

MqOnOffQueueDisc::~MqOnOffQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
MqOnOffQueueDisc::SetWorkingQueueIdx(uint32_t idx)
{
    this->working_queue_idx = idx;
}

uint32_t
MqOnOffQueueDisc::GetWorkingQueueIdx()
{
    return this->working_queue_idx;
}



bool
MqOnOffQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);
    int32_t ret = SearchQueueIdx(item);
    if (ret > GetNQueueDiscClasses() || ret <0)
    {
        NS_LOG_DEBUG("Did not found a queue to enqueue this packet.");
        return false;
    }
    else
    {
        NS_LOG_DEBUG("Packet filters returned " << ret);
    }

    NS_ASSERT_MSG(ret < GetNQueueDiscClasses(), "Selected queue out of range");
    bool retval = GetQueueDiscClass(ret)->GetQueueDisc()->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue disc
    // because QueueDisc::AddQueueDiscClass sets the drop callback

    NS_LOG_LOGIC("Number packets band " << ret << ": "
                                        << GetQueueDiscClass(ret)->GetQueueDisc()->GetNPackets());

    return retval;
}


Ptr<QueueDiscItem>
MqOnOffQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;

    if ((item = GetQueueDiscClass(this->working_queue_idx)->GetQueueDisc()->Dequeue()))
    {
        NS_LOG_LOGIC("Popped from band " << this->working_queue_idx << ": " << item);
        NS_LOG_LOGIC("Number packets band "
                     << this->working_queue_idx << ": " << GetQueueDiscClass(this->working_queue_idx)->GetQueueDisc()->GetNPackets());
        return item;
    }
    NS_LOG_LOGIC("Queue" <<working_queue_idx<< " empty");
    return item;
}


Ptr<const QueueDiscItem>
MqOnOffQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    if ( (item = GetQueueDiscClass(this->working_queue_idx)->GetQueueDisc()->Peek())  )
    {
        NS_LOG_LOGIC("Peek from band " << this->working_queue_idx << ": " << item);
        NS_LOG_LOGIC("Number packets band "
                     << this->working_queue_idx << ": " << GetQueueDiscClass(this->working_queue_idx)->GetQueueDisc()->GetNPackets());
        return item;
    }
    NS_LOG_LOGIC("Queue" <<working_queue_idx<< " empty");
    return item;
}


bool
MqOnOffQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNInternalQueues() > 0)
    {
        NS_LOG_ERROR("MqOnOffQueueDisc cannot have internal queues");
        return false;
    }

    if (GetNQueueDiscClasses() == 0)
    {
        // create 3 fifo queue discs
        ObjectFactory factory;
        factory.SetTypeId("ns3::FifoQueueDisc");
        for (uint8_t i = 0; i < 2; i++)
        {
            Ptr<QueueDisc> qd = factory.Create<QueueDisc>();
            qd->Initialize();
            Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass>();
            c->SetQueueDisc(qd);
            AddQueueDiscClass(c);
        }
    }

    if (GetNQueueDiscClasses() < 2)
    {
        NS_LOG_ERROR("MqOnOffQueueDisc needs at least 2 classes");
        return false;
    }

    return true;
}

uint32_t
MqOnOffQueueDisc::GetQueueSizeN(uint32_t n)
{
    if(n<0 || n > GetNQueueDiscClasses())
    {
        NS_LOG_DEBUG("can not fetch the size of queue " << n);
        return -1;
    }
    return GetQueueDiscClass(n)->GetQueueDisc()->GetCurrentSize().GetValue();
}

void
MqOnOffQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
SearchQueueIdx(Ptr<QueueDiscItem> item)
{
    auto pkt = item->GetPacket()->Copy();
    Ipv4Header ip;
    pkt->RemoveHeader(ip);
    TcpHeader tcp;
    pkt->RemoveHeader(tcp);
    MetadataHeader meta;
    pkt->PeekHeader(meta);
    return meta.GetDestQueue();
}

} // namespace ns3