//
// Created by schedule on 6/12/23.
//
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"

#include "ns3/ipv4-ocs-routing.h"
#include "ns3/route-schedule.h"
#include "ns3/Ipv4EpsRouting.h"
#include "ns3/MultiChannelPointToPoint.h"
#include "ns3/OcsUtils.h"
#include "ns3/MultideviceHelper.h"
#include "ns3/new-bulk-send-helper.h"
#include "ns3/new-bulk-send-application.h"

//should never install the tor switches, because of the judgement of addr
using namespace ns3;
NS_LOG_COMPONENT_DEFINE("ScratchTest");


static void EpsRouteHelper(NodeContainer TORs,uint32_t queuenumber){
    for(uint32_t i = 0;i<TORs.GetN();i++){
        Ptr<Node> tori = TORs.Get(i);
        Ptr<Ipv4L3Protocol> epsIpv4 = tori->GetObject<Ipv4L3Protocol>();
        Ptr<Ipv4RoutingProtocol> totalrouting = epsIpv4->GetRoutingProtocol();
        Ptr<Ipv4ListRouting> listrouting = DynamicCast<Ipv4ListRouting>(totalrouting);
        if(!listrouting){
            std::cout<<"error not listrouting, insert failed";
            return;
        }
        Ptr<Ipv4EpsRouting> epsRouting = new Ipv4EpsRouting(queuenumber,50,0.98,0.9);
        listrouting->AddRoutingProtocol(epsRouting,10);
    }
}
int main(int argc,char* argv[])
{
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1440));
    Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(MicroSeconds(2)));
    Config::SetDefault("ns3::RttEstimator::InitialEstimation",TimeValue(MicroSeconds(200)));
    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(700)));
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity",TimeValue(MicroSeconds(1)));
    Config::SetDefault("ns3::TcpSocket::DataRetries",UintegerValue(100));
//    Time::SetResolution(Time::NS);
    //nodes

    NodeContainer OCS;
    OCS.Create(1);
    NodeContainer Cores;
    Cores.Create(1);

    NodeContainer Hosts;
    Hosts.Create(4);

    NodeContainer TORs;
    TORs.Create(4);
    //device helpers
    uint32_t  queuenumber = 2;
    Ptr<Node> ocsnode = OCS.Get(0);
    QueueSize queueSize =  QueueSize("100kB");
    MultiDeviceHelper OCSLinks =  MultiDeviceHelper(queuenumber,ocsnode,queueSize);
    OCSLinks.SetDeviceAttribute("DataRate", StringValue("80Gbps"));
    OCSLinks.SetChannelAttribute("Delay", StringValue("50ns"));
    OCSLinks.SetMultiDeviceRate("80Gbps");
    OCSLinks.SetEnableFlowControl(false);
    NetDeviceContainer T0O,T1O,T2O,T3O;
    T0O = OCSLinks.Install(TORs.Get(0));
    T1O = OCSLinks.Install(TORs.Get(1));
    T2O = OCSLinks.Install(TORs.Get(2));
    T3O = OCSLinks.Install(TORs.Get(3));
    //Switches
    PointToPointHelper SwLinks;
    SwLinks.SetDeviceAttribute("DataRate",StringValue("10Gbps"));
    SwLinks.SetChannelAttribute("Delay",StringValue("50ns"));
    SwLinks.DisableFlowControl();
    NetDeviceContainer S1,S2,S0,S3;
    S0 = SwLinks.Install(TORs.Get(0),Cores.Get(0));
    S1 = SwLinks.Install(TORs.Get(1),Cores.Get(0));
    S2 = SwLinks.Install(TORs.Get(2),Cores.Get(0));
    S3 = SwLinks.Install(TORs.Get(3),Cores.Get(0));
    //Hosts
    PointToPointHelper HoLinks;
    HoLinks.SetDeviceAttribute("DataRate",StringValue("10Gbps"));
    HoLinks.SetChannelAttribute("Delay",StringValue("10ns"));
    HoLinks.DisableFlowControl();
    NetDeviceContainer H1,H2,H0,H3;
    H0 = HoLinks.Install(TORs.Get(0),Hosts.Get(0));
    H1 = HoLinks.Install(TORs.Get(1),Hosts.Get(1));
    H2 = HoLinks.Install(TORs.Get(2),Hosts.Get(2));
    H3 = HoLinks.Install(TORs.Get(3),Hosts.Get(3));

    InternetStackHelper stack;
    stack.Install(TORs);
    stack.Install(Cores);
    stack.Install(Hosts);

    Ipv4AddressHelper address;


    Ipv4InterfaceContainer SOIface,S1Iface,S2Iface,S3Iface;
    address.SetBase("50.0.0.0","255.255.0.0","0.0.0.1");
    SOIface = address.Assign(S0);
    address.SetBase("50.1.0.0","255.255.0.0","0.0.0.1");
    S1Iface = address.Assign(S1);
    address.SetBase("50.2.0.0","255.255.0.0","0.0.0.1");
    S2Iface = address.Assign(S2);
    address.SetBase("50.3.0.0","255.255.0.0","0.0.0.1");
    S3Iface = address.Assign(S3);

    Ipv4InterfaceContainer H0Iface,H1Iface,H2Iface,H3Iface;
    address.SetBase("10.0.0.0","255.255.255.0","0.0.0.1");
    H0Iface = address.Assign(H0);
    address.SetBase("20.0.0.0","255.255.255.0","0.0.0.1");
    H1Iface = address.Assign(H1);
    address.SetBase("30.0.0.0","255.255.255.0","0.0.0.1");
    H2Iface = address.Assign(H2);
    address.SetBase("40.0.0.0","255.255.255.0","0.0.0.1");
    H3Iface = address.Assign(H3);


    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    // ocs only
    stack.Install(OCS);
    Ipv4InterfaceContainer T0OIface,T1OIface,T2OIface,T3OIface;
    address.SetBase("10.0.0.0","255.255.0.0","0.0.1.1");
    T0OIface = address.Assign(T0O);
    address.SetBase("20.0.0.0","255.255.0.0","0.0.1.1");
    T1OIface = address.Assign(T1O);
    address.SetBase("30.0.0.0","255.255.0.0","0.0.1.1");
    T2OIface = address.Assign(T2O);
    address.SetBase("40.0.0.0","255.255.0.0","0.0.1.1");
    T3OIface = address.Assign(T3O);
    //ocs routing
    Ptr<Ipv4OcsRouting> OcsRoutingP = new Ipv4OcsRouting();
    Ptr<RouteSchedule> RouteScheduler = new RouteSchedule(MicroSeconds(180), MicroSeconds(20),MicroSeconds(0),queuenumber,OCS.Get(0),OcsRoutingP, Seconds(10));
    Ptr<Ipv4L3Protocol> OcsIpv4 = OCS.Get(0)->GetObject<Ipv4L3Protocol>();
    OcsIpv4->SetRoutingProtocol(OcsRoutingP);
    //eps routing
    EpsRouteHelper(TORs,queuenumber);
    RouteScheduler->Initialize();

    uint16_t sport0 = 40001;
//    uint16_t sport1 = 50001;

    //1,2~0
    Ipv4Address Addr0 = H0Iface.GetAddress(1);
    NewBulkSendHelper sender0("ns3::TcpSocketFactory",InetSocketAddress(Addr0,sport0));
//    NewBulkSendHelper sender1("ns3::TcpSocketFactory",InetSocketAddress(Addr0,sport1));
    PacketSinkHelper receiver0("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),sport0));
//    PacketSinkHelper receiver1("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),sport1));

    sender0.SetAttribute("MaxBytes", UintegerValue(uint32_t(1024 * 1024)));
    sender0.SetAttribute("SendSize", UintegerValue(uint32_t(1440)));
//    sender1.SetAttribute("MaxBytes", UintegerValue(uint32_t(10240 * 1024)));
//    sender1.SetAttribute("SendSize", UintegerValue(uint32_t(1440)));
    ApplicationContainer sendapp0 = sender0.Install(Hosts.Get(1));
//    ApplicationContainer sendapp1 = sender1.Install(Hosts.Get(2));
    sendapp0.Start(Seconds(0.00001));
    sendapp0.Stop(Seconds(10));
//    sendapp1.Start(Seconds(0.1));
//    sendapp1.Stop(Seconds(10));


    ApplicationContainer receiveapp0 = receiver0.Install(Hosts.Get(0));
//    ApplicationContainer receiveapp1 = receiver1.Install(Hosts.Get(0));
    receiveapp0.Start(Seconds(0));
//    receiveapp1.Start(Seconds(0));
    receiveapp0.Stop(Seconds(10));
//    receiveapp1.Stop(Seconds(10));

    AsciiTraceHelper ascii;


    HoLinks.EnableAscii(ascii.CreateFileStream("host.tr"),Hosts);
    HoLinks.EnableAscii(ascii.CreateFileStream("sw.tr"),Cores);
    HoLinks.EnableAscii(ascii.CreateFileStream("ocs.tr"),OCS);
    OCSLinks.EnableAscii(ascii.CreateFileStream("TOR.tr"),TORs);
    HoLinks.EnablePcap("ho",Hosts, false);
    HoLinks.EnablePcap("ocs",OCS);
    HoLinks.EnablePcap("core",Cores, false);
//    HoLinks.EnablePcap("ocs",OCS, false);
//    HoLinks.EnablePcap("tor",TORs, false);

    LogComponentEnable("Ipv4EpsRouting",LOG_LEVEL_INFO);
    Simulator::Stop(Seconds(10));
    Simulator::Run();
    Simulator::Destroy();
    return 0;

}
