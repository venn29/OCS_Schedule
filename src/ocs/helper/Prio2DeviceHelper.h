//
// Created by venn on 23-8-31.
//

#ifndef NS3_PRIO2DEVICEHELPER_H
#define NS3_PRIO2DEVICEHELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/trace-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/data-rate.h"

namespace ns3
{
class NetDevice;
class Node;
class PointToPointNetDevice;

class Prio2DeviceHelper :  public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
    Prio2DeviceHelper();
    Prio2DeviceHelper(QueueSize qsize);
    ~Prio2DeviceHelper() override
    {}

    template <typename... Ts>
    void SetQueue(std::string type, Ts&&... args);

    NetDeviceContainer Install(Ptr<Node> tor,Ptr<Node> agg);

    void SetChannelAttribute(std::string name, const AttributeValue& value);

    void SetDeviceAttribute(std::string name, const AttributeValue& value);

    void SetPrioDeviceRate(std::string delay);

    void SetEnableFlowControl(bool efc){
        this->m_enableFlowControl = efc;
    }

  private:
    void EnablePcapInternal(std::string prefix,
                            Ptr<NetDevice> nd,
                            bool promiscuous,
                            bool explicitFilename) override;

    void EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                             std::string prefix,
                             Ptr<NetDevice> nd,
                             bool explicitFilename) override;
    ObjectFactory m_queueFactory;   //!< Queue Factory
    ObjectFactory m_channelFactory; //!< Channel Factory
    ObjectFactory m_deviceFactory;  //!< Device Factory
    ObjectFactory m_prio2deviceFatcory;
    bool m_enableFlowControl;
    DataRate bps_factory;
};
template <typename... Ts>
void
Prio2DeviceHelper::SetQueue(std::string type, Ts&&... args)
{
    QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

    m_queueFactory.SetTypeId(type);
    m_queueFactory.Set(std::forward<Ts>(args)...);
}
}
#endif // NS3_PRIO2DEVICEHELPER_H
