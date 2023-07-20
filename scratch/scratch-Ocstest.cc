
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-ocs-routing.h"
#include "ns3/route-schedule.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScratchOCSSimulator");

int
main(int argc, char* argv[])
{
    Time::SetResolution(Time::NS);
    NodeContainer TORs;
    TORs.Create(3);



    NodeContainer OCS;
    OCS.Create(1);


    //    PointToPointHelper SwLinks;
    //    SwLinks.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    //    SwLinks.SetDeviceAttribute("Delay", StringValue("200ns"));
    //
    PointToPointHelper OCSLinks;
    OCSLinks.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    OCSLinks.SetChannelAttribute("Delay", StringValue("2ms"));

    //
    NetDeviceContainer T0O,T1O,T2O;
    T0O = OCSLinks.Install(TORs.Get(0),OCS.Get(0));
    T1O = OCSLinks.Install(TORs.Get(1),OCS.Get(0));
    T2O = OCSLinks.Install(TORs.Get(2),OCS.Get(0));

    InternetStackHelper stack;
    stack.Install(TORs);

    stack.Install(OCS);
    //

    //    std::cout<<OCS.Get(0)->GetNDevices()<<std::endl;
    //    for(uint32_t i=0;i<OCS.Get(0)->GetNDevices();i++){
    //        auto dv = OCS.Get(0)->GetDevice(i);
    //        auto channel = dv->GetChannel();
    //        std::cout<<dv<<std::endl;
    //        std::cout<<channel<<std::endl;
    //    }

    //assign address
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer T0OIface,T1OIface,T2OIface;
    address.SetBase("10.0.0.0","255.255.255.0");
    T0OIface = address.Assign(T0O);

    address.SetBase("10.0.1.0","255.255.255.0");
    T1OIface = address.Assign(T1O);

    address.SetBase("10.0.2.0","255.255.255.0");
    T2OIface = address.Assign(T2O);




    uint32_t MaxPacketSize = 1400;
    Time interPacketInterval = Seconds(0.3);
    uint32_t maxPacketCount = 320;
    //s-c 0 : s0, c1
    //s-c 1: s0 , c2
    //s-c 2: s1 c0
    //s-c 3: s1 c2
    //s-c 4: s2 c0
    //s-c 5: s2 c1
    uint16_t sport0 = 4000;
    uint16_t sport1 = 5000;

    Address Addr0 = Address(T0OIface.GetAddress(0));
    //    Address Addr1 = Address(T1OIface.GetAddress(0));
    //    Address Addr2 = Address(T2OIface.GetAddress(0));
    //s-c 0, s-c 1
    UdpServerHelper server0(sport0);
    UdpServerHelper server1(sport1);
    UdpClientHelper client0(Addr0,sport0);
    UdpClientHelper client1(Addr0,sport1);

    ApplicationContainer s0 = server0.Install(TORs.Get(0));
    s0.Start(Seconds(0.0));
    s0.Stop(Seconds(10));
    client0.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client0.SetAttribute("Interval", TimeValue(interPacketInterval));
    client0.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    ApplicationContainer c0 = client0.Install(TORs.Get(1));
    c0.Start(Seconds(0.0));
    c0.Stop(Seconds(10.0));

    ApplicationContainer s1 = server1.Install(TORs.Get(0));
    s1.Start(Seconds(0.0));
    s1.Stop(Seconds(10));
    client1.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client1.SetAttribute("Interval", TimeValue(interPacketInterval));
    client1.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    ApplicationContainer c1 = client1.Install(TORs.Get(2));
    c1.Start(Seconds(0.0));
    c1.Stop(Seconds(10));
    //
    //    //s-c 2, s-c 3
    //    UdpServerHelper server2(sport0);
    //    UdpServerHelper server3(sport1);
    //    UdpClientHelper client2(Addr1,sport0);
    //    UdpClientHelper client3(Addr1,sport1);
    //
    //    ApplicationContainer s2 = server2.Install(TORs.Get(1));
    //    s2.Start(Seconds(1.0));
    //    s2.Stop(Seconds(10));
    //    client2.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    //    client2.SetAttribute("Interval", TimeValue(interPacketInterval));
    //    client2.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    //    ApplicationContainer c2 = client2.Install(TORs.Get(0));
    //    c2.Start(Seconds(2.0));
    //    c2.Stop(Seconds(10.0));
    //
    //    ApplicationContainer s3 = server3.Install(TORs.Get(1));
    //    s3.Start(Seconds(1.0));
    //    s3.Stop(Seconds(10));
    //    client3.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    //    client3.SetAttribute("Interval", TimeValue(interPacketInterval));
    //    client3.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    //    ApplicationContainer c3 = client3.Install(TORs.Get(2));
    //    c3.Start(Seconds(2.0));
    //    c3.Stop(Seconds(10));
    //
    //    //s-c 4, s-c 5
    //    UdpServerHelper server4(sport0);
    //    UdpServerHelper server5(sport1);
    //    UdpClientHelper client4(Addr2,sport0);
    //    UdpClientHelper client5(Addr2,sport1);
    //
    //    ApplicationContainer s4 = server4.Install(TORs.Get(2));
    //    s4.Start(Seconds(1.0));
    //    s4.Stop(Seconds(10));
    //    client4.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    //    client4.SetAttribute("Interval", TimeValue(interPacketInterval));
    //    client4.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    //    ApplicationContainer c4 = client4.Install(TORs.Get(0));
    //    c4.Start(Seconds(2.0));
    //    c4.Stop(Seconds(10.0));
    //
    //    ApplicationContainer s5 = server5.Install(TORs.Get(2));
    //    s5.Start(Seconds(1.0));
    //    s5.Stop(Seconds(10));
    //    client5.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    //    client5.SetAttribute("Interval", TimeValue(interPacketInterval));
    //    client5.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
    //    ApplicationContainer c5 = client5.Install(TORs.Get(1));
    //    c5.Start(Seconds(2.0));
    //    c5.Stop(Seconds(10));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

//    Ptr<Ipv4OcsRouting> OcsRoutingP = new Ipv4OcsRouting();
//    Ptr<RouteSchedule> RouteScheduler = new RouteSchedule(Seconds(0.9), Seconds(0.1),MicroSeconds(0),1,OCS.Get(0),OcsRoutingP, Seconds(10));
//    Ptr<Ipv4L3Protocol> OcsIpv4 = OCS.Get(0)->GetObject<Ipv4L3Protocol>();
//    OcsIpv4->SetRoutingProtocol(OcsRoutingP);
//    RouteScheduler->Initialize();

    AsciiTraceHelper ascii;
    OCSLinks.EnablePcapAll("OCStest");
    OCSLinks.EnableAsciiAll(ascii.CreateFileStream("sw.tr"));
    Simulator::Stop(Seconds(10));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
