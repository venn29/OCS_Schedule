//
// Created by schedule on 6/5/23.
//

#ifndef NS3_MULTICHANNELPOINTTOPOINT_H
#define NS3_MULTICHANNELPOINTTOPOINT_H

#include "ns3/point-to-point-net-device.h"
#include "ns3/queue.h"
#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/data-rate.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/output-stream-wrapper.h"


namespace  ns3
{
class MultiChannelPointToPointDevice : public PointToPointNetDevice
{
  public:
    static TypeId GetTypeId();
    MultiChannelPointToPointDevice();
    ~MultiChannelPointToPointDevice();


    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    void TransmitComplete();
    bool TransmitStart(Ptr<Packet> p);
    void DoDispose() override;

    //schedule interfaces
    void SetQueueNumber(uint32_t qn){
        this->queue_number = qn;
    }

    uint32_t GetQueueNumber(){
        return this->queue_number;
    }

    Ptr<Queue<Packet>> GetQueue (int idx){
        return multi_queues[idx];
    }

    void AddQueue(Ptr<Queue<Packet>> qp){
        this->multi_queues.push_back(qp);
    }


    void SetWorkingQueue(uint32_t qidx){
        this->working_queue_idx = qidx;
    }

    uint32_t GetWorkingQueue(){
        return this->working_queue_idx;
    }

    uint32_t NextWorkingQueue(){
        this->working_queue_idx = (this->working_queue_idx+1) % this->queue_number;
        return this->working_queue_idx;
    }
    void IntoNight(){
        this->working = false;
    }
    void BeginWork();

    void RecordQueueBuffer(std::ofstream* ostream);




  private:
    std::vector<Ptr<Queue<Packet>>> multi_queues;
    //when configuring , this device of TOR node does not work
    bool working;
    uint32_t working_queue_idx;
    uint32_t queue_number;


    uint16_t GetTargetQueue(Ptr<Packet> p);
    //tools
    /**
     * Adds the necessary headers and trailers to a packet of data in order to
     * respect the protocol implemented by the agent.
     * \param p packet
     * \param protocolNumber protocol number
     */
    void AddHeader(Ptr<Packet> p, uint16_t protocolNumber);
    uint16_t EtherToPpp(uint16_t proto);



};
}
#endif // NS3_MULTICHANNELPOINTTOPOINT_H
