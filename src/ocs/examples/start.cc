#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include "../helper/new-bulk-send-helper.h"
#include "../model/new-bulk-send-application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestExample");

int main(int argc, char* argv[])
{
    bool tracing = false;
    uint32_t maxBytes = 0;

//    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(536));

    CommandLine cmd(__FILE__);
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes", "Total number of bytes for application to send", maxBytes);
    cmd.Parse(argc, argv);

//    LogComponentEnable("PacketTagList", LOG_LEVEL_INFO);
    LogComponentEnable("NewBulkSendApplication", LOG_LEVEL_INFO);
//    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
//    LogComponentEnable("TcpL4Protocol", LOG_LEVEL_INFO);
//    LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("500Kbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);
    
    uint16_t Port = 9;
    NewBulkSendHelper newBulkSendHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(interfaces.GetAddress(1), Port));
    newBulkSendHelper.SetAttribute("MaxBytes", UintegerValue(uint32_t(1024 * 1024))); 
    newBulkSendHelper.SetAttribute("SendSize", UintegerValue(uint32_t(536)));                               
    ApplicationContainer sendApps = newBulkSendHelper.Install(nodes.Get(0));
    sendApps.Start(Seconds(0.));
    sendApps.Stop(Seconds(10.));

    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                        InetSocketAddress(Ipv4Address::GetAny(), Port));
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(10.));

    if (tracing)
    {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll(ascii.CreateFileStream("test.tr"));
        pointToPoint.EnablePcapAll("test", false);
    }

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;
    return 0;
}   