//
// Created by schedule on 6/8/23.
//

#ifndef NS3_MULTIDEVICEHELPER_H
#define NS3_MULTIDEVICEHELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/trace-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/data-rate.h"

#include <string>

namespace ns3
{
class MultiChannelPointToPointDevice;
class NetDevice;
class Node;
class PointToPointNetDevice;

class MultiDeviceHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
    MultiDeviceHelper();

    MultiDeviceHelper(uint32_t qn,Ptr<Node> onode,QueueSize qsize);

    ~MultiDeviceHelper() override
    {}

    template <typename... Ts>
    void SetQueue(std::string type, Ts&&... args);

    NetDeviceContainer Install(Ptr<Node> tor, Ptr<Node> ocs);

    NetDeviceContainer Install(Ptr<Node> tor);

    void SetChannelAttribute(std::string name, const AttributeValue& value);

    void SetDeviceAttribute(std::string name, const AttributeValue& value);

    void SetMultiDeviceRate(std::string delay);

    void SetOcsNode(Ptr<Node> ond);

    void SetEnableFlowControl(bool efc){
        this->m_enableFlowControl = efc;
    }

    Ptr<Node> GetOcsNode();
    uint32_t  GetQueueNumber(){return  this->queue_number;}
  private:
    void EnablePcapInternal(std::string prefix,
                           Ptr<NetDevice> nd,
                           bool promiscuous,
                           bool explicitFilename) override;

    void EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                             std::string prefix,
                             Ptr<NetDevice> nd,
                             bool explicitFilename) override;

    uint32_t queue_number;
    ObjectFactory m_queueFactory;   //!< Queue Factory
    ObjectFactory m_channelFactory; //!< Channel Factory
    ObjectFactory m_P2PdeviceFactory;  //!< Device Factory
    ObjectFactory m_MultideviceFactory;
    Ptr<Node> ocs_node;

    bool m_enableFlowControl;

    DataRate bps_factory;




};
template <typename... Ts>
void
MultiDeviceHelper::SetQueue(std::string type, Ts&&... args)
{
    QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

    m_queueFactory.SetTypeId(type);
    m_queueFactory.Set(std::forward<Ts>(args)...);
}
}



#endif // NS3_MULTIDEVICEHELPER_H
