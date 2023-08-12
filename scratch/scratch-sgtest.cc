//
// Created by venn on 23-8-4.
//
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/queue-size.h"

#include "ns3/ipv4-ocs-routing.h"
#include "ns3/SingleRouteSchedule.h"
#include "ns3/Ipv4SingleEPSRouting.h"
#include "ns3/MultiChannelPointToPoint.h"
#include "ns3/OcsUtils.h"
#include "ns3/MultideviceHelper.h"
#include "ns3/new-bulk-send-helper.h"
#include "ns3/new-bulk-send-application.h"
#include "ns3/PFifoFlowSizeQueueDisc.h"
#include "ns3/AppPlanner.h"
#include <fstream>



using namespace ns3;
NS_LOG_COMPONENT_DEFINE("ScratchTest");

static void EpsRouteHelper(NodeContainer TORs,uint32_t queuenumber){
    for(uint32_t i = 0;i<TORs.GetN();i++){
        Ptr<Node> tori = TORs.Get(i);
        Ptr<Ipv4L3Protocol> epsIpv4 = tori->GetObject<Ipv4L3Protocol>();
        Ptr<Ipv4RoutingProtocol> totalrouting = epsIpv4->GetRoutingProtocol();
        Ptr<Ipv4ListRouting> listrouting = DynamicCast<Ipv4ListRouting>(totalrouting);
        if(!listrouting){
            //            std::cout<<"error not listrouting, insert failed";
            return;
        }
        Ptr<Ipv4SingleEPSRouting> SepsRouting = new Ipv4SingleEPSRouting();
        listrouting->AddRoutingProtocol(SepsRouting,10);
    }
}


static void Cluster2ClusterAppHelper(NodeContainer servers, NodeContainer clients,Ipv4InterfaceContainer ServerIfaces[], int apppernode,uint16_t port_start,uint32_t maxbytes)
{
    uint32_t servernodenum = servers.GetN();
    uint32_t clientnodenum = clients.GetN();
    NS_ASSERT_MSG(servernodenum == clientnodenum, "Cluster to cluster servre number does not eqaul to client");
    for(int i=0 ;i<apppernode;i++)
    {
        uint16_t port = port_start+i;
        for(uint32_t j=0;j<servernodenum;j++)
        {
            Ipv4Address Addrj = ServerIfaces[j].GetAddress(1);
            NewBulkSendHelper senderj("ns3::TcpSocketFactory",InetSocketAddress(Addrj,port));
            PacketSinkHelper receiverj("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),port));
            senderj.SetAttribute("MaxBytes", UintegerValue(maxbytes));
            senderj.SetAttribute("SendSize", UintegerValue(uint32_t(1446)));
            ApplicationContainer sendappj = senderj.Install(clients.Get(j));
            sendappj.Start(Seconds(0.00001));
            sendappj.Stop(Seconds(10));
            ApplicationContainer receiveappj = receiverj.Install(servers.Get(j));
            receiveappj.Start(Seconds(0));
            receiveappj.Stop(Seconds(10));
        }
    }
}


void SetPfifoSizeQueueDisc(Ptr<NetDevice> dv,Ptr<TrafficControlLayer> tc)
{
    Ptr<PointToPointNetDevice> p2pdv = DynamicCast<PointToPointNetDevice>(dv);
    p2pdv->SetQueueSize(QueueSize("3100B"));
    tc->DeleteRootQueueDiscOnDevice(dv);
    TrafficControlHelper tcHelper;
    tcHelper.SetRootQueueDisc("ns3::PFifoFlowSizeQueueDisc");
    tcHelper.Install(dv);
}


int main(int argc,char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc,argv);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1446));
    Config::SetDefault("ns3::TcpSocket::DelAckCount",UintegerValue(1));
    Config::SetDefault("ns3::RttEstimator::InitialEstimation",TimeValue(MicroSeconds(100)));
    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(650)));
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity",TimeValue(MicroSeconds(1)));
    Config::SetDefault("ns3::TcpSocket::DataRetries",UintegerValue(100));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd",UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpNewReno::GetTypeId()));
    //    Time::SetResolution(Time::NS);
    //      nodes

    uint32_t servernumber = 16;

    NodeContainer OCS;
    OCS.Create(1);

    NodeContainer Cores;
    Cores.Create(4);

    NodeContainer TORs;
    TORs.Create(8);

    NodeContainer Aggs;
    Aggs.Create(8);

    NodeContainer Hosts0;
    Hosts0.Create(servernumber);
    NodeContainer Hosts1;
    Hosts1.Create(servernumber);
    NodeContainer Hosts2;
    Hosts2.Create(servernumber);
    NodeContainer Hosts3;
    Hosts3.Create(servernumber);
    NodeContainer Hosts4;
    Hosts4.Create(servernumber);
    NodeContainer Hosts5;
    Hosts5.Create(servernumber);
    NodeContainer Hosts6;
    Hosts6.Create(servernumber);
    NodeContainer Hosts7;
    Hosts7.Create(servernumber);

    //CoreSwitch Links
    PointToPointHelper CoreLinks;
    CoreLinks.SetDeviceAttribute("DataRate",StringValue("40Gbps"));
    CoreLinks.SetChannelAttribute("Delay",StringValue("4us"));
    CoreLinks.DisableFlowControl();

    //aggre Links
    PointToPointHelper AggLinks;

    AggLinks.SetDeviceAttribute("DataRate",StringValue("40Gbps"));
    AggLinks.SetChannelAttribute("Delay",StringValue("4us"));


    //Host Links
    PointToPointHelper HoLinks;
    HoLinks.SetDeviceAttribute("DataRate",StringValue("10Gbps"));
    HoLinks.SetChannelAttribute("Delay",StringValue("1us"));
    HoLinks.DisableFlowControl();

    //    Config::SetDefault("ns3::DropTailQueue::MaxSize",QueueSizeValue(QueueSize("150000B")));
    //core device
    NetDeviceContainer CoreDev0,CoreDev1,CoreDev2,CoreDev3,CoreDev4,CoreDev5,CoreDev6,CoreDev7;
    NetDeviceContainer CoreDev8,CoreDev9,CoreDev10,CoreDev11,CoreDev12,CoreDev13,CoreDev14,CoreDev15;
    CoreDev0 = CoreLinks.Install(Cores.Get(0),Aggs.Get(0));
    CoreDev1 = CoreLinks.Install(Cores.Get(0),Aggs.Get(2));
    CoreDev2 = CoreLinks.Install(Cores.Get(0),Aggs.Get(4));
    CoreDev3 = CoreLinks.Install(Cores.Get(0),Aggs.Get(6));

    CoreDev4 = CoreLinks.Install(Cores.Get(1),Aggs.Get(0));
    CoreDev5 = CoreLinks.Install(Cores.Get(1),Aggs.Get(2));
    CoreDev6 = CoreLinks.Install(Cores.Get(1),Aggs.Get(4));
    CoreDev7 = CoreLinks.Install(Cores.Get(1),Aggs.Get(6));

    CoreDev8 = CoreLinks.Install(Cores.Get(2),Aggs.Get(1));
    CoreDev9 = CoreLinks.Install(Cores.Get(2),Aggs.Get(3));
    CoreDev10 = CoreLinks.Install(Cores.Get(2),Aggs.Get(5));
    CoreDev11 = CoreLinks.Install(Cores.Get(2),Aggs.Get(7));

    CoreDev12 = CoreLinks.Install(Cores.Get(3),Aggs.Get(1));
    CoreDev13 = CoreLinks.Install(Cores.Get(3),Aggs.Get(3));
    CoreDev14 = CoreLinks.Install(Cores.Get(3),Aggs.Get(5));
    CoreDev15 = CoreLinks.Install(Cores.Get(3),Aggs.Get(7));

    //aggregation device
    NetDeviceContainer AggDev0,AggDev1,AggDev2,AggDev3,AggDev4,AggDev5,AggDev6,AggDev7,AggDev8,AggDev9,AggDev10,AggDev11,AggDev12,AggDev13,AggDev14,AggDev15;
    AggDev0 = AggLinks.Install(Aggs.Get(0),TORs.Get(0));
    AggDev1 = AggLinks.Install(Aggs.Get(0),TORs.Get(1));
    AggDev2 = AggLinks.Install(Aggs.Get(1),TORs.Get(0));
    AggDev3 = AggLinks.Install(Aggs.Get(1),TORs.Get(1));

    AggDev4 = AggLinks.Install(Aggs.Get(2),TORs.Get(2));
    AggDev5 = AggLinks.Install(Aggs.Get(2),TORs.Get(3));
    AggDev6 = AggLinks.Install(Aggs.Get(3),TORs.Get(2));
    AggDev7 = AggLinks.Install(Aggs.Get(3),TORs.Get(3));

    AggDev8 = AggLinks.Install(Aggs.Get(4),TORs.Get(4));
    AggDev9 = AggLinks.Install(Aggs.Get(4),TORs.Get(5));
    AggDev10 = AggLinks.Install(Aggs.Get(5),TORs.Get(4));
    AggDev11 = AggLinks.Install(Aggs.Get(5),TORs.Get(5));

    AggDev12 = AggLinks.Install(Aggs.Get(6),TORs.Get(6));
    AggDev13 = AggLinks.Install(Aggs.Get(6),TORs.Get(7));
    AggDev14 = AggLinks.Install(Aggs.Get(7),TORs.Get(6));
    AggDev15 = AggLinks.Install(Aggs.Get(7),TORs.Get(7));

    //TOR device (to servers)
    NetDeviceContainer TorDev0[16],TorDev1[16],TorDev2[16],TorDev3[16],TorDev4[16],TorDev5[16],TorDev6[16],TorDev7[16];
    for(int i=0;i<16;i++)
    {
        TorDev0[i] = HoLinks.Install(TORs.Get(0),Hosts0.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev1[i] = HoLinks.Install(TORs.Get(1),Hosts1.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev2[i] = HoLinks.Install(TORs.Get(2),Hosts2.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev3[i] = HoLinks.Install(TORs.Get(3),Hosts3.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev4[i] = HoLinks.Install(TORs.Get(4),Hosts4.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev5[i] = HoLinks.Install(TORs.Get(5),Hosts5.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev6[i] = HoLinks.Install(TORs.Get(6),Hosts6.Get(i));
    }
    for(int i=0;i<16;i++)
    {
        TorDev7[i] = HoLinks.Install(TORs.Get(7),Hosts7.Get(i));
    }


    InternetStackHelper stack;
    stack.Install(TORs);
    stack.Install(Cores);
    stack.Install(Aggs);
    stack.Install(Hosts0);
    stack.Install(Hosts1);
    stack.Install(Hosts2);
    stack.Install(Hosts3);
    stack.Install(Hosts4);
    stack.Install(Hosts5);
    stack.Install(Hosts6);
    stack.Install(Hosts7);

    Ipv4AddressHelper address;

    // \to-do: assign ip for eps switches
    //TOR-HOST address
    Ipv4InterfaceContainer TorIface0[16],TorIface1[16],TorIface2[16],TorIface3[16],TorIface4[16],TorIface5[16],TorIface6[16],TorIface7[16];
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"10.0.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface0[i] = address.Assign(TorDev0[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"10.1.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface1[i] = address.Assign(TorDev1[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"20.0.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface2[i] = address.Assign(TorDev2[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"20.1.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface3[i] = address.Assign(TorDev3[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"30.0.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface4[i] = address.Assign(TorDev4[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"30.1.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface5[i] = address.Assign(TorDev5[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"40.0.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface6[i] = address.Assign(TorDev6[i]);
    }
    for(int i = 0;i<16;i++)
    {
        char base_addr[100];
        sprintf(base_addr,"40.1.%d.0",i*2+1);
        address.SetBase(base_addr,"255.255.255.0");
        TorIface7[i] = address.Assign(TorDev7[i]);
    }

    //Aggregation-TOR address
    Ipv4InterfaceContainer AggIface0,AggIface1,AggIface2,AggIface3,AggIface4,AggIface5,AggIface6,AggIface7;
    Ipv4InterfaceContainer AggIface8,AggIface9,AggIface10,AggIface11,AggIface12,AggIface13,AggIface14,AggIface15;

    address.SetBase("10.2.0.0","255.255.0.0");
    AggIface0 = address.Assign(AggDev0);
    address.SetBase("10.3.0.0","255.255.0.0");
    AggIface1 = address.Assign(AggDev1);
    address.SetBase("10.4.0.0","255.255.0.0");
    AggIface2 = address.Assign(AggDev2);
    address.SetBase("10.5.0.0","255.255.0.0");
    AggIface3 = address.Assign(AggDev3);

    address.SetBase("20.2.0.0","255.255.0.0");
    AggIface4 = address.Assign(AggDev4);
    address.SetBase("20.3.0.0","255.255.0.0");
    AggIface5 = address.Assign(AggDev5);
    address.SetBase("20.4.0.0","255.255.0.0");
    AggIface6 = address.Assign(AggDev6);
    address.SetBase("20.5.0.0","255.255.0.0");
    AggIface7 = address.Assign(AggDev7);

    address.SetBase("30.2.0.0","255.255.0.0");
    AggIface8 = address.Assign(AggDev8);
    address.SetBase("30.3.0.0","255.255.0.0");
    AggIface9 = address.Assign(AggDev9);
    address.SetBase("30.4.0.0","255.255.0.0");
    AggIface10 = address.Assign(AggDev10);
    address.SetBase("30.5.0.0","255.255.0.0");
    AggIface11 = address.Assign(AggDev11);

    address.SetBase("40.2.0.0","255.255.0.0");
    AggIface12 = address.Assign(AggDev12);
    address.SetBase("40.3.0.0","255.255.0.0");
    AggIface13 = address.Assign(AggDev13);
    address.SetBase("40.4.0.0","255.255.0.0");
    AggIface14 = address.Assign(AggDev14);
    address.SetBase("40.5.0.0","255.255.0.0");
    AggIface15 = address.Assign(AggDev15);

    //set queuedisc
    //0
    Ptr<Node> N0 = TORs.Get(0);
    Ptr<Node> N1 = TORs.Get(1);
    Ptr<TrafficControlLayer> Tc0 = N0->GetObject<TrafficControlLayer>();
    Ptr<TrafficControlLayer> Tc1 = N1->GetObject<TrafficControlLayer>();
    Ptr<NetDevice> d0 = AggDev0.Get(1);
    Ptr<NetDevice> d1 = AggDev1.Get(1);
    Ptr<NetDevice> d2 = AggDev2.Get(1);
    Ptr<NetDevice> d3 = AggDev3.Get(1);
    SetPfifoSizeQueueDisc(d0,Tc0);
    SetPfifoSizeQueueDisc(d1,Tc1);
    SetPfifoSizeQueueDisc(d2,Tc0);
    SetPfifoSizeQueueDisc(d3,Tc1);
    //1
    Ptr<Node> N2 = TORs.Get(2);
    Ptr<Node> N3 = TORs.Get(3);
    Ptr<TrafficControlLayer> Tc2 = N2->GetObject<TrafficControlLayer>();
    Ptr<TrafficControlLayer> Tc3 = N3->GetObject<TrafficControlLayer>();
    Ptr<NetDevice> d4 = AggDev4.Get(1);
    Ptr<NetDevice> d5 = AggDev5.Get(1);
    Ptr<NetDevice> d6 = AggDev6.Get(1);
    Ptr<NetDevice> d7 = AggDev7.Get(1);
    SetPfifoSizeQueueDisc(d4,Tc2);
    SetPfifoSizeQueueDisc(d5,Tc3);
    SetPfifoSizeQueueDisc(d6,Tc2);
    SetPfifoSizeQueueDisc(d7,Tc3);
    //2
    Ptr<Node> N4 = TORs.Get(4);
    Ptr<Node> N5 = TORs.Get(5);
    Ptr<TrafficControlLayer> Tc4 = N4->GetObject<TrafficControlLayer>();
    Ptr<TrafficControlLayer> Tc5 = N5->GetObject<TrafficControlLayer>();
    Ptr<NetDevice> d8 = AggDev8.Get(1);
    Ptr<NetDevice> d9 = AggDev9.Get(1);
    Ptr<NetDevice> d10 = AggDev10.Get(1);
    Ptr<NetDevice> d11 = AggDev11.Get(1);
    SetPfifoSizeQueueDisc(d8,Tc4);
    SetPfifoSizeQueueDisc(d9,Tc5);
    SetPfifoSizeQueueDisc(d10,Tc4);
    SetPfifoSizeQueueDisc(d11,Tc5);
    //3
    Ptr<Node> N6 = TORs.Get(6);
    Ptr<Node> N7 = TORs.Get(7);
    Ptr<TrafficControlLayer> Tc6 = N6->GetObject<TrafficControlLayer>();
    Ptr<TrafficControlLayer> Tc7 = N7->GetObject<TrafficControlLayer>();
    Ptr<NetDevice> d12 = AggDev12.Get(1);
    Ptr<NetDevice> d13 = AggDev13.Get(1);
    Ptr<NetDevice> d14 = AggDev14.Get(1);
    Ptr<NetDevice> d15 = AggDev15.Get(1);
    SetPfifoSizeQueueDisc(d12,Tc6);
    SetPfifoSizeQueueDisc(d13,Tc7);
    SetPfifoSizeQueueDisc(d14,Tc6);
    SetPfifoSizeQueueDisc(d15,Tc7);



    //Core-Agg Interfaces
    Ipv4InterfaceContainer CoreIface0,CoreIface1,CoreIface2,CoreIface3,CoreIface4,CoreIface5,CoreIface6,CoreIface7;
    Ipv4InterfaceContainer CoreIface8,CoreIface9,CoreIface10,CoreIface11,CoreIface12,CoreIface13,CoreIface14,CoreIface15;
    address.SetBase("50.0.0.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev0);
    address.SetBase("50.0.1.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev1);
    address.SetBase("50.0.2.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev2);
    address.SetBase("50.0.3.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev3);

    address.SetBase("50.1.0.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev4);
    address.SetBase("50.1.1.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev5);
    address.SetBase("50.1.2.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev6);
    address.SetBase("50.1.3.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev7);

    address.SetBase("50.2.0.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev8);
    address.SetBase("50.2.1.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev9);
    address.SetBase("50.2.2.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev10);
    address.SetBase("50.2.3.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev11);

    address.SetBase("50.3.0.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev12);
    address.SetBase("50.3.1.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev13);
    address.SetBase("50.3.2.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev14);
    address.SetBase("50.3.3.0","255.255.255.0");
    CoreIface0 = address.Assign(CoreDev15);


    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //OCS helpers
    uint32_t  queuenumber = 1;
    Ptr<Node> ocsnode = OCS.Get(0);
    QueueSize queueSize =  QueueSize("1000kB");
    MultiDeviceHelper OCSLinks =  MultiDeviceHelper(queuenumber,ocsnode,queueSize);
    OCSLinks.SetDeviceAttribute("DataRate", StringValue("80Gbps"));
    OCSLinks.SetChannelAttribute("Delay", StringValue("8us"));
    OCSLinks.SetMultiDeviceRate("80Gbps");
    OCSLinks.SetEnableFlowControl(false);
    NetDeviceContainer T0O,T1O,T2O,T3O,T4O,T5O,T6O,T7O;
    T0O = OCSLinks.Install(TORs.Get(0));
    T1O = OCSLinks.Install(TORs.Get(1));
    T2O = OCSLinks.Install(TORs.Get(2));
    T3O = OCSLinks.Install(TORs.Get(3));
    T4O = OCSLinks.Install(TORs.Get(4));
    T5O = OCSLinks.Install(TORs.Get(5));
    T6O = OCSLinks.Install(TORs.Get(6));
    T7O = OCSLinks.Install(TORs.Get(7));
    // ocs only
    stack.Install(OCS);
    Ipv4InterfaceContainer T0OIface,T1OIface,T2OIface,T3OIface,T4OIface,T5OIface,T6OIface,T7OIface;
    address.SetBase("10.0.0.0","255.255.0.0","0.0.50.1");
    T0OIface = address.Assign(T0O);
    address.SetBase("10.1.0.0","255.255.0.0","0.0.50.1");
    T1OIface = address.Assign(T1O);
    address.SetBase("20.0.0.0","255.255.0.0","0.0.50.1");
    T2OIface = address.Assign(T2O);
    address.SetBase("20.1.0.0","255.255.0.0","0.0.50.1");
    T3OIface = address.Assign(T3O);
    address.SetBase("30.0.0.0","255.255.0.0","0.0.50.1");
    T4OIface = address.Assign(T4O);
    address.SetBase("30.1.0.0","255.255.0.0","0.0.50.1");
    T5OIface = address.Assign(T5O);
    address.SetBase("40.0.0.0","255.255.0.0","0.0.50.1");
    T6OIface = address.Assign(T6O);
    address.SetBase("40.1.0.0","255.255.0.0","0.0.50.1");
    T7OIface = address.Assign(T7O);
    //ocs routing
    Ptr<Ipv4OcsRouting> OcsRoutingP = new Ipv4OcsRouting();
    Ptr<SingleRouteSchedule> RouteScheduler = new SingleRouteSchedule(MicroSeconds(180), MicroSeconds(20),MicroSeconds(0),queuenumber,OCS.Get(0),OcsRoutingP, Seconds(10));
    RouteScheduler->SetBufferOutFile("./BufferRecord.txt");
    Ptr<Ipv4L3Protocol> OcsIpv4 = OCS.Get(0)->GetObject<Ipv4L3Protocol>();
    OcsIpv4->SetRoutingProtocol(OcsRoutingP);
    //eps routing
    EpsRouteHelper(TORs,queuenumber);
    RouteScheduler->Initialize();

    //apps
    //    uint16_t sport0 = 40001;
    //    Ipv4Address Addr0 = TorIface0[0].GetAddress(1);
    //    NewBulkSendHelper sender0("ns3::TcpSocketFactory",InetSocketAddress(Addr0,sport0));
    //    PacketSinkHelper receiver0("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),sport0));
    //    sender0.SetAttribute("MaxBytes", UintegerValue(uint32_t(1024 * 1024)));
    //    sender0.SetAttribute("SendSize", UintegerValue(uint32_t(1440)));
    //    ApplicationContainer sendapp0 = sender0.Install(Hosts1.Get(0));
    //    sendapp0.Start(Seconds(0.00001));
    //    sendapp0.Stop(Seconds(10));
    //    ApplicationContainer receiveapp0 = receiver0.Install(Hosts0.Get(0));
    //    receiveapp0.Start(Seconds(0));
    //    receiveapp0.Stop(Seconds(10));
    Cluster2ClusterAppHelper(Hosts0,Hosts2,TorIface0,1,10001,102400*1024);
    Ptr<AppPlanner> apl = new AppPlanner();
    apl->AddClientSet(Hosts2);
    apl->AddServerSet(Hosts0);
    apl->CreatePlanPoisson();


    AsciiTraceHelper ascii;
    //    HoLinks.EnableAscii(ascii.CreateFileStream("host.tr"),Hosts0);
    //    HoLinks.EnableAscii(ascii.CreateFileStream("host1.tr"),Hosts1);
    //    HoLinks.EnableAscii(ascii.CreateFileStream("sw.tr"),Cores);
    //    HoLinks.EnableAscii(ascii.CreateFileStream("agg.tr"),Aggs);
    //    HoLinks.EnableAscii(ascii.CreateFileStream("ocs.tr"),OCS);
    OCSLinks.EnableAscii(ascii.CreateFileStream("TOR.tr"),TORs);
    //    OCSLinks.EnableAscii(ascii.CreateFileStream("Agg.tr"),Aggs);
    //        HoLinks.EnablePcap("ocs",OCS);
    //        HoLinks.EnablePcap("core",Cores, false);
    HoLinks.EnablePcap("tor",TORs, false);
    //        HoLinks.EnablePcap("agg",Aggs, false);
    //    HoLinks.EnablePcap("HO2",Hosts2, false);
    //        HoLinks.EnablePcap("HO0",Hosts0, false);

    LogComponentEnable("TcpCongestionOps",LOG_LEVEL_INFO);
    //
    //    std::ofstream ofs;
    //    ofs.open("QueueSpace.tr",std::ios_base::out);
    //    Ptr<BufferRecorder> br = new BufferRecorder(MicroSeconds(200),&ofs,queuenumber);

    Simulator::Stop(Seconds(10));
    Simulator::Run();
    Simulator::Destroy();
    return 0;

}
