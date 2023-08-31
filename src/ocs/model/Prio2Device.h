//
// Created by schedule on 8/25/23.
//

#ifndef NS3_PRIO2DEVICE_H
#define NS3_PRIO2DEVICE_H

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

namespace ns3
{
class Prio2Device : public PointToPointNetDevice
{
  public:
    static TypeId GetTypeId();
    Prio2Device();
    ~Prio2Device();

    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    void TransmitComplete();
    bool TransmitStart(Ptr<Packet> p);
    void DoDispose() override;



  private:
    std::vector<Ptr<Queue<Packet>>> prio_queues;
    uint32_t queue_number;
    void AddHeader(Ptr<Packet> p, uint16_t protocolNumber);
    uint16_t EtherToPpp(uint16_t proto);

    uint16_t GetTargetQueue(Ptr<Packet> p);
};
}
#endif // NS3_PRIO2DEVICE_H
