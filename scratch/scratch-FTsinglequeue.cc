//
// Created by venn on 23-8-12.
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
#include "ns3/FatTreeHelper.h"
#include <fstream>



using namespace ns3;
NS_LOG_COMPONENT_DEFINE("ScratchFattreeTest");

int main(int argc,char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc,argv);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::DelAckCount",UintegerValue(1));
    Config::SetDefault("ns3::RttEstimator::InitialEstimation",TimeValue(MicroSeconds(100)));
    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(1000)));
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity",TimeValue(MicroSeconds(1)));
    Config::SetDefault("ns3::TcpSocket::DataRetries",UintegerValue(100));
//    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd",UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault("ns3::TcpSocketBase::Timestamp",BooleanValue(true));
    ns3::RngSeedManager::SetSeed(1530);

    std::cout<<"start";
    ns3::RngSeedManager::SetRun(7);
    FatTreeHelper* ft = new FatTreeHelper(10);
    ft->Create(false);
    uint32_t  queuenumber = 1;
    NodeContainer OCS;
    OCS.Create(1);
    Ptr<Node> ocsnode = OCS.Get(0);
    QueueSize queueSize =  QueueSize("2000KB");
    MultiDeviceHelper OCSLinks =  MultiDeviceHelper(queuenumber,ocsnode,queueSize);
    ft->SetOcsSingle(OCSLinks,ocsnode);


    Ptr<AppPlanner> apl = new AppPlanner();
    double starttime = 0.000520;
    double addperu = 0.000260;
    int port = 10001;
    int portadd = 100;
    for(int i = 6;i<6+50;i++){
//        if(i == 0)
//            continue;
        apl->LongFlowPlan(ft->GetNodeInEdge((i)%50),ft->GetNodeInEdge(0),80,port,102400*1024, Seconds(starttime));
        starttime += addperu;
        port += portadd;
    }
    //    apl->AddClientSet(ft->GetNodeInEdge(0));
//    apl->AddServerSet(ft->GetNodeInEdge(6));
//    apl->CreatePlanUniform(2500);
//    apl->CreatePlanFromTrace("/home/venn/ns-allinone-3.38/ns-3.38/FlowTrace_data1W.csv");
//    apl->LongFlowPlan(ft->GetNodeInEdge(6),ft->GetNodeInEdge(0),1,10001,10240*1024, Seconds(0.000520));

        Ptr<AppPlanner> aplmice[71];
        for(int i=0;i<49;i++){
            Ptr<AppPlanner> aplt = new AppPlanner;
            aplmice[i] = aplt;
            aplmice[i]->AddClientSet(ft->GetNodeInEdge(0));
            aplmice[i]->AddServerSet(ft->GetNodeInEdge(i+1));
            aplmice[i]->CreatePlanFromTrace("/home/venn/ns-allinone-3.38/ns-3.38/utils/FlowGenerate/tracedir/FlowTrace_data_"+std::to_string(i+1)+".csv");
//            std::cout<<"created "<<i+1<<std::endl;
        }
    AsciiTraceHelper ascii;
    PointToPointHelper p2ph;
    p2ph.EnablePcap("HO0",ft->GetNodeInEdge(0));
//    p2ph.EnablePcap("ocs",OCS);

    p2ph.EnableAscii(ascii.CreateFileStream("ocs.tr"),OCS);
//    std::cout<<"start";

    Simulator::Stop(Seconds(1));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}