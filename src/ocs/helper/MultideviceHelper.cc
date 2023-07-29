//
// Created by schedule on 6/8/23.
//

#include "MultideviceHelper.h"
#include "ns3/MultiChannelPointToPoint.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/simulator.h"
#include "ns3/queue-size.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/OcsUtils.h"
#include "ns3/trace-helper.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MultideviceHelper");

MultiDeviceHelper::MultiDeviceHelper()
{
    QueueSize qs(QueueSizeUnit::BYTES, 18750);
    m_MultideviceFactory.SetTypeId("ns3::MultiChannelPointToPointDevice");
    m_P2PdeviceFactory.SetTypeId("ns3::PointToPointNetDevice");
    m_channelFactory.SetTypeId("ns3::PointToPointChannel");
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_queueFactory.Set("MaxSize", QueueSizeValue(qs));
    queue_number = 8;
    ocs_node = nullptr;
    m_enableFlowControl = true;
}

MultiDeviceHelper::MultiDeviceHelper(uint32_t qn, Ptr<ns3::Node> onode, ns3::QueueSize qsize)
{
    m_MultideviceFactory.SetTypeId("ns3::MultiChannelPointToPointDevice");
    m_P2PdeviceFactory.SetTypeId("ns3::PointToPointNetDevice");
    m_channelFactory.SetTypeId("ns3::PointToPointChannel");
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_queueFactory.Set("MaxSize", QueueSizeValue(qsize));
    queue_number = qn;
    ocs_node = onode;
    m_enableFlowControl = true;
}

void
MultiDeviceHelper::SetDeviceAttribute(std::string name, const AttributeValue& value)
{
    m_P2PdeviceFactory.Set(name, value);
}

void
MultiDeviceHelper::SetMultiDeviceRate(std::string delay)
{
    this->bps_factory = DataRate(delay);
}

void
MultiDeviceHelper::SetChannelAttribute(std::string name, const AttributeValue& value)
{
    m_channelFactory.Set(name, value);
}

void
MultiDeviceHelper::SetOcsNode(Ptr<Node> ond)
{
    ocs_node = ond;
}

Ptr<Node>
MultiDeviceHelper::GetOcsNode()
{
    return ocs_node;
}

NetDeviceContainer
MultiDeviceHelper::Install(Ptr<ns3::Node> tor)
{
    return Install(tor,this->ocs_node);
}

NetDeviceContainer
MultiDeviceHelper::Install(Ptr<ns3::Node> tor, Ptr<ns3::Node> ocs)
{
    NetDeviceContainer container;
    //device on ocs
    Ptr<PointToPointNetDevice> devA = m_P2PdeviceFactory.Create<PointToPointNetDevice>();
    devA->SetAddress(Mac48Address::Allocate());
    ocs->AddDevice(devA);
    Ptr<Queue<Packet>> queueA = m_queueFactory.Create<Queue<Packet>>();
    devA->SetQueue(queueA);
    //device on tor
    Ptr<MultiChannelPointToPointDevice> devB = m_MultideviceFactory.Create<MultiChannelPointToPointDevice>();
    devB->SetDataRate(this->bps_factory);
    devB->SetAddress(Mac48Address::Allocate());
    tor->AddDevice(devB);
    devB->SetQueueNumber(this->queue_number);
    for(uint32_t i=0;i<devB->GetQueueNumber();i++){
        Ptr<Queue<Packet>> queuei = m_queueFactory.Create<Queue<Packet>>();
        devB->AddQueue(queuei);
    }
    if(m_enableFlowControl)
    {
        Ptr<NetDeviceQueueInterface> ndqiA = CreateObject<NetDeviceQueueInterface>();
        ndqiA->GetTxQueue(0)->ConnectQueueTraces(queueA);

        Ptr<NetDeviceQueueInterface> ndqiB = CreateObjectWithAttributes<NetDeviceQueueInterface>("NTxQueues",UintegerValue(queue_number));
        for(uint32_t i=0;i<queue_number;i++)
        {
            ndqiB->GetTxQueue(i)->ConnectQueueTraces(devB->GetQueue(i));
        }
        ndqiB->SetSelectQueueCallback(&SelectQueueByLeftSize);
    }
    Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel>();
    devA->Attach(channel);
    devB->Attach(channel);
    container.Add(devA);
    container.Add(devB);
    return container;
}

// private

void
MultiDeviceHelper::EnablePcapInternal(std::string prefix,
                                      Ptr<NetDevice> nd,
                                      bool promiscuous,
                                      bool explicitFilename)
{
    //
    // All of the Pcap enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system.  We can only deal with devices of type PointToPointNetDevice.
    //
    Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("PointToPointHelper::EnablePcapInternal(): Device "
                    << device << " not of type ns3::PointToPointNetDevice");
        return;
    }

    PcapHelper pcapHelper;

    std::string filename;
    if (explicitFilename)
    {
        filename = prefix;
    }
    else
    {
        filename = pcapHelper.GetFilenameFromDevice(prefix, device);
    }

    Ptr<PcapFileWrapper> file = pcapHelper.CreateFile(filename, std::ios::out, PcapHelper::DLT_PPP);
    pcapHelper.HookDefaultSink<PointToPointNetDevice>(device, "PromiscSniffer", file);
}

// need to be modified
void
MultiDeviceHelper::EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                                        std::string prefix,
                                        Ptr<NetDevice> nd,
                                        bool explicitFilename)
{
    //
    // All of the ascii enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system.  We can only deal with devices of type PointToPointNetDevice.
    //
    Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("PointToPointHelper::EnableAsciiInternal(): Device "
                    << device << " not of type ns3::PointToPointNetDevice");
        return;
    }

    //
    // Our default trace sinks are going to use packet printing, so we have to
    // make sure that is turned on.
    //
    Packet::EnablePrinting();

    //
    // If we are not provided an OutputStreamWrapper, we are expected to create
    // one using the usual trace filename conventions and do a Hook*WithoutContext
    // since there will be one file per context and therefore the context would
    // be redundant.
    //
    if (!stream)
    {
        //
        // Set up an output stream object to deal with private ofstream copy
        // constructor and lifetime issues.  Let the helper decide the actual
        // name of the file given the prefix.
        //
        AsciiTraceHelper asciiTraceHelper;

        std::string filename;
        if (explicitFilename)
        {
            filename = prefix;
        }
        else
        {
            filename = asciiTraceHelper.GetFilenameFromDevice(prefix, device);
        }

        Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream(filename);

        //
        // The MacRx trace source provides our "r" event.
        //
        asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<PointToPointNetDevice>(device,
                                                                                     "MacRx",
                                                                                     theStream);

        //
        // The "+", '-', and 'd' events are driven by trace sources actually in the
        // transmit queue.
        //

        Ptr<MultiChannelPointToPointDevice> mcdevice =
            DynamicCast<MultiChannelPointToPointDevice>(device);
        if (mcdevice)
        {
            uint32_t queuenumber = mcdevice->GetQueueNumber();
            for (uint32_t i = 0; i < queuenumber; i++)
            {
                Ptr<Queue<Packet>> queuei = mcdevice->GetQueue(i);
                asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(queuei,
                                                                                     "Enqueue",
                                                                                     theStream);
                asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(queuei,
                                                                                  "Drop",
                                                                                  theStream);
                asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(queuei,
                                                                                     "Dequeue",
                                                                                     theStream);
            }
        }
        else
        {
            Ptr<Queue<Packet>> queue = device->GetQueue();
            asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(queue,
                                                                                 "Enqueue",
                                                                                 theStream);
            asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(queue,
                                                                              "Drop",
                                                                              theStream);
            asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(queue,
                                                                                 "Dequeue",
                                                                                 theStream);
        }
        // PhyRxDrop trace source for "d" event
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<PointToPointNetDevice>(device,
                                                                                  "PhyRxDrop",
                                                                                  theStream);

        return;
    }
    uint32_t nodeid = nd->GetNode()->GetId();
    uint32_t deviceid = nd->GetIfIndex();
    std::ostringstream oss;

    oss << "/NodeList/" << nd->GetNode()->GetId() << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/MacRx";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

    // \todo: bound every queue to asicii, but not now
//
//    oss.str("");
//    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
//        << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
//    Config::Connect(oss.str(),
//                    MakeBoundCallback(&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));
//
//    oss.str("");
//    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
//        << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
//    Config::Connect(oss.str(),
//                    MakeBoundCallback(&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));
//
//    oss.str("");
//    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
//        << "/$ns3::PointToPointNetDevice/TxQueue/Drop";
//    Config::Connect(oss.str(),
//                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/PhyRxDrop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}
}