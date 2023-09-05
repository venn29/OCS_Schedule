//
// Created by venn on 23-8-31.
//

#include "Prio2DeviceHelper.h"
#include "ns3/Prio2Device.h"
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
NS_LOG_COMPONENT_DEFINE("Prio2DeviceHelper");

Prio2DeviceHelper::Prio2DeviceHelper()
{
    QueueSize qs(QueueSizeUnit::BYTES,2000);
    m_deviceFactory.SetTypeId("ns3::PointToPointNetDevice");
    m_prio2deviceFatcory.SetTypeId("ns3::Prio2Device");
    m_channelFactory.SetTypeId("ns3::PointToPointChannel");
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_queueFactory.Set("MaxSize",QueueSizeValue(qs));
    m_enableFlowControl = false;
}

Prio2DeviceHelper::Prio2DeviceHelper(ns3::QueueSize qsize)
{

    m_deviceFactory.SetTypeId("ns3::PointToPointNetDevice");
    m_prio2deviceFatcory.SetTypeId("ns3::Prio2Device");
    m_channelFactory.SetTypeId("ns3::PointToPointChannel");
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_queueFactory.Set("MaxSize",QueueSizeValue(qsize));
    m_enableFlowControl = false;
}

void
Prio2DeviceHelper::SetDeviceAttribute(std::string name, const AttributeValue& value)
{
    m_deviceFactory.Set(name, value);
}

void
Prio2DeviceHelper::SetPrioDeviceRate(std::string delay)
{
    this->bps_factory = DataRate(delay);
}

void
Prio2DeviceHelper::SetChannelAttribute(std::string name, const AttributeValue& value)
{
    m_channelFactory.Set(name, value);
}

NetDeviceContainer
Prio2DeviceHelper::Install(Ptr<ns3::Node> tor, Ptr<ns3::Node> agg)
{
    NetDeviceContainer container;
    //device on agg
    Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice>();
    devA->SetAddress(Mac48Address::Allocate());
    agg->AddDevice(devA);
    Ptr<Queue<Packet>> queueA = m_queueFactory.Create<Queue<Packet>>();
    devA->SetQueue(queueA);
    //device on tor
    Ptr<Prio2Device> devB = m_prio2deviceFatcory.Create<Prio2Device>();
    devB->SetDataRate(this->bps_factory);
    devB->SetAddress(Mac48Address::Allocate());
    tor->AddDevice(devB);
    devB->SetQueueNumber(2);
    for(uint32_t i=0;i<devB->GetQueueNumber();i++){
        Ptr<Queue<Packet>> queuei = m_queueFactory.Create<Queue<Packet>>();
        devB->AddQueue(queuei);
    }
    if(m_enableFlowControl)
    {
        Ptr<NetDeviceQueueInterface> ndqiA = CreateObject<NetDeviceQueueInterface>();
        ndqiA->GetTxQueue(0)->ConnectQueueTraces(queueA);

    }
    Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel>();
    devA->Attach(channel);
    devB->Attach(channel);
    container.Add(devA);
    container.Add(devB);
    return container;
}

void
Prio2DeviceHelper::EnablePcapInternal(std::string prefix,
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



void
Prio2DeviceHelper::EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
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
        Ptr<Queue<Packet>> queue = device->GetQueue();
        asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>>(queue,
                                                                             "Enqueue",
                                                                             theStream);
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>>(queue, "Drop", theStream);
        asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>>(queue,
                                                                             "Dequeue",
                                                                             theStream);

        // PhyRxDrop trace source for "d" event
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<PointToPointNetDevice>(device,
                                                                                  "PhyRxDrop",
                                                                                  theStream);

        return;
    }

    //
    // If we are provided an OutputStreamWrapper, we are expected to use it, and
    // to providd a context.  We are free to come up with our own context if we
    // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
    // compatibility and simplicity, we just use Config::Connect and let it deal
    // with the context.
    //
    // Note that we are going to use the default trace sinks provided by the
    // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
    // but the default trace sinks are actually publicly available static
    // functions that are always there waiting for just such a case.
    //
    uint32_t nodeid = nd->GetNode()->GetId();
    uint32_t deviceid = nd->GetIfIndex();
    std::ostringstream oss;

    oss << "/NodeList/" << nd->GetNode()->GetId() << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/MacRx";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/TxQueue/Drop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::PointToPointNetDevice/PhyRxDrop";
    Config::Connect(oss.str(),
                    MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}


}