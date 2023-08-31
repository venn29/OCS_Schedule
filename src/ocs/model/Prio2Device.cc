//
// Created by schedule on 8/25/23.
//

#include "Prio2Device.h"
#include "OcsUtils.h"
#include "ns3/ppp-header.h"
#include "ns3/error-model.h"
#include "ns3/llc-snap-header.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/pointer.h"

#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/point-to-point-channel.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("Prio2Device");
NS_OBJECT_ENSURE_REGISTERED(Prio2Device);

TypeId
Prio2Device::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Prio2Device")
            .SetParent<PointToPointNetDevice>()
            .SetGroupName("Ocs")
            .AddConstructor<Prio2Device>();
    return tid;
}

Prio2Device::Prio2Device()
    :prio_queues(std::vector<Ptr<Queue<Packet>>> (2)),
      queue_number(2)
{
    NS_LOG_FUNCTION(this);
}

Prio2Device::~Prio2Device()
{
    NS_LOG_FUNCTION(this);
}

void
Prio2Device::DoDispose()
{
    prio_queues.clear();
    queue_number = 0;
}

bool
Prio2Device::Send(Ptr<ns3::Packet> packet, const ns3::Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    NS_LOG_LOGIC("p=" << packet << ", dest=" << &dest);
    NS_LOG_LOGIC("UID is " << packet->GetUid());

    if (IsLinkUp() == false)
    {
        NS_LOG_ERROR(this<< "Link down.");
        return false;
    }

    uint16_t queue_tgt_idx = GetTargetQueue(packet);
    Ptr<Queue<Packet>> queue_tgt = this->prio_queues[queue_tgt_idx];
    AddHeader(packet, protocolNumber);
    if(queue_tgt->Enqueue(packet))
    {
        if(PointToPointNetDevice::GetMachineState() == PointToPointNetDevice::READY)
        {
            for(auto queue:prio_queues)
            {
                if(Ptr<Packet> p = queue->Dequeue())
                {
                    bool ret = TransmitStart((p));
                    return ret;
                }
            }
        }
        return true;
    }
    return false;
}

bool
Prio2Device::TransmitStart(Ptr<ns3::Packet> p)
{
    NS_LOG_FUNCTION(this << p);
    NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
    NS_ASSERT_MSG(PointToPointNetDevice::GetMachineState() == READY, "Must be READY to transmit");

    PointToPointNetDevice::SetMachineState(PointToPointNetDevice::BUSY);
    PointToPointNetDevice::SetCurrentPacket(p);
    DataRate bps = PointToPointNetDevice::GetDataRate();
    Time txTime = bps.CalculateBytesTxTime(p->GetSize());
    Time txCompleteTime = txTime + PointToPointNetDevice::GetInterframeGap();

    NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.As(Time::S));
    Simulator::Schedule(txCompleteTime,&Prio2Device::TransmitComplete,this);
    Ptr<PointToPointChannel> p2pch = DynamicCast<PointToPointChannel>(PointToPointNetDevice::GetChannel());
    bool result = p2pch->TransmitStart(p,this,txTime);
    return result;
}

void
Prio2Device::TransmitComplete()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(PointToPointNetDevice::GetMachineState() == BUSY, "Must be BUSY if transmitting\"");
    PointToPointNetDevice::SetMachineState(PointToPointNetDevice::READY);
    NS_ASSERT_MSG(PointToPointNetDevice::GetCurrentPacket(), "PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");
    PointToPointNetDevice::SetCurrentPacket(nullptr);
    for(auto queue:prio_queues)
    {
        if(Ptr<Packet> p = queue->Dequeue())
        {
            TransmitStart((p));
            return ;
        }
    }
}

uint16_t
Prio2Device::GetTargetQueue(Ptr<ns3::Packet> p)
{
    if(this->queue_number <= 1)
        return 0;
    OcsTag metadata;
    bool found = p->PeekPacketTag(metadata);
    if(!found){
        NS_LOG_ERROR("no queue index found in multichannel p2p device");
        return -1;
    }
    else{
        return metadata.GetQueueIdx();
    }
}

void
Prio2Device::AddHeader(Ptr<Packet> p, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << p << protocolNumber);
    PppHeader ppp;
    ppp.SetProtocol(EtherToPpp(protocolNumber));
    p->AddHeader(ppp);
}

uint16_t
Prio2Device::EtherToPpp(uint16_t proto)
{
    NS_LOG_FUNCTION_NOARGS();
    switch (proto)
    {
    case 0x0800:
        return 0x0021; // IPv4
    case 0x86DD:
        return 0x0057; // IPv6
    default:
        NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
    }
    return 0;
}

}