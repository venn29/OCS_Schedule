//
// Created by schedule on 6/5/23.
//

#include "MultiChannelPointToPoint.h"
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
NS_LOG_COMPONENT_DEFINE("MultiChannelPointToPointDevice");

NS_OBJECT_ENSURE_REGISTERED(MultiChannelPointToPointDevice);

TypeId
MultiChannelPointToPointDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MultiChannelPointToPointDevice")
            .SetParent<PointToPointNetDevice>()
            .SetGroupName("Ocs")
            .AddConstructor<MultiChannelPointToPointDevice>();
    return tid;
}

MultiChannelPointToPointDevice::MultiChannelPointToPointDevice()
    : multi_queues(std::vector<Ptr<Queue<Packet>>> ()),
      working(false),
      working_queue_idx(0),
      queue_number(8)
{
    NS_LOG_FUNCTION(this);
}


MultiChannelPointToPointDevice::~MultiChannelPointToPointDevice()
{
    NS_LOG_FUNCTION(this);
}

void
MultiChannelPointToPointDevice::DoDispose()
{
    multi_queues.clear();
    queue_number = 0;
}

bool
MultiChannelPointToPointDevice::Send(Ptr<ns3::Packet> packet, const ns3::Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    NS_LOG_LOGIC("p=" << packet << ", dest=" << &dest);
    NS_LOG_LOGIC("UID is " << packet->GetUid());

//    Time t = Simulator::Now();
//    std::cout<<t<<"\n";
    if (IsLinkUp() == false)
    {
        NS_LOG_ERROR(this<< "Link down.");
        return false;
    }

    uint16_t queue_tgt_idx = GetTargetQueue(packet);
    Ptr<Queue<Packet>> queue_tgt = this->multi_queues[queue_tgt_idx];
    AddHeader(packet, protocolNumber);
    if(queue_tgt->Enqueue(packet)){
        //changed them in p2p device to protected
        if(this->working &&PointToPointNetDevice::GetMachineState() == PointToPointNetDevice::READY){
            Ptr<Queue<Packet>> working_queue = this->multi_queues[this->working_queue_idx];
            Ptr<Packet> p = working_queue->Dequeue();
            if (!p)
            {
                NS_LOG_LOGIC("No pending packets in device queue ");
                return false;
            }
            bool ret = TransmitStart(p);
            return ret;
        }
        return true;
    }
//    else{
////        std::cout<<"queue max at "<<ns3::Simulator::Now().GetSeconds()<<" at queue"<<queue_tgt_idx<<"when working queue is"<<working_queue_idx<<std::endl;
//    }
    return false;
}

bool
MultiChannelPointToPointDevice::TransmitStart(Ptr<ns3::Packet> p)
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
    Simulator::Schedule(txCompleteTime,&MultiChannelPointToPointDevice::TransmitComplete,this);
    Ptr<PointToPointChannel> p2pch = DynamicCast<PointToPointChannel>(PointToPointNetDevice::GetChannel());
    bool result = p2pch->TransmitStart(p,this,txTime);
    return result;
}

void
MultiChannelPointToPointDevice::TransmitComplete()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(PointToPointNetDevice::GetMachineState() == BUSY, "Must be BUSY if transmitting\"");
    PointToPointNetDevice::SetMachineState(PointToPointNetDevice::READY);
    NS_ASSERT_MSG(PointToPointNetDevice::GetCurrentPacket(), "PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");
    PointToPointNetDevice::SetCurrentPacket(nullptr);
    //only work at day
    if(!this->working)
        return;
    auto working_queue = this->multi_queues[this->working_queue_idx];
    Ptr<Packet> p = working_queue->Dequeue();
    if (!p)
    {
        NS_LOG_LOGIC("No pending packets in device queue after tx complete");
        return;
    }
    TransmitStart(p);
}

void
MultiChannelPointToPointDevice::BeginWork()
{
    this->working = true;
    PointToPointNetDevice::SetMachineState(PointToPointNetDevice::READY);
    PointToPointNetDevice::SetCurrentPacket(nullptr);
    auto working_queue = this->multi_queues[this->working_queue_idx];
    Ptr<Packet> p = working_queue->Dequeue();
    if (!p)
    {
        NS_LOG_LOGIC("No pending packets in device queue after tx complete");
        return;
    }
    TransmitStart(p);
}

void
MultiChannelPointToPointDevice::IntoNight(){
    this->working = false;
    if(this->queue_number == 1){
        auto working_queue = this->multi_queues[0];
        int cnt = 0;
        Ptr<Packet> p ;
        do{
            p = working_queue->Remove();
            cnt++;
        } while (p != nullptr);
//        std::cout<<"reserved pkt num "<<cnt<<std::endl;
        return;
    }
    if(queue_number>4)
        return;
    auto working_queue = this->multi_queues[(this->working_queue_idx-1)%4];
    Ptr<Packet> p ;
    do{
       p = working_queue->Remove();
    } while (p != nullptr);
}

void
MultiChannelPointToPointDevice::RecordQueueBuffer(std::ofstream* ostream)
{
    Time now = Simulator::Now();
    uint32_t cnt = 0;
    for(int i=this->working_queue_idx;cnt<queue_number;i=(i+1)%queue_number)
    {
        uint32_t  size = this->multi_queues[i]->GetNPackets();
        *ostream<<now.GetSeconds()<<"\t"<<this->GetNode()->GetId()<<"\t"<<this->GetIfIndex()<<"\t"<<i<<"\t"<<size<<"\n";
        cnt++;
    }
//    for(Ptr<Queue<Packet>> q:this->multi_queues)
//    {
////        uint32_t size = q->GetCurrentSize().GetValue();
//        uint32_t  size = q->GetNPackets();
////        std::cout<<size;
//        *ostream<<now.GetSeconds()<<"\t"<<this->GetNode()->GetId()<<"\t"<<this->GetIfIndex()<<"\t"<<size<<"\n";
//    }
}

uint16_t
MultiChannelPointToPointDevice::GetTargetQueue(Ptr<Packet> p)
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
MultiChannelPointToPointDevice::AddHeader(Ptr<Packet> p, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << p << protocolNumber);
    PppHeader ppp;
    ppp.SetProtocol(EtherToPpp(protocolNumber));
    p->AddHeader(ppp);
}

uint16_t
MultiChannelPointToPointDevice::EtherToPpp(uint16_t proto)
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