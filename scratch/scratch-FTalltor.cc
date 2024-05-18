//
// Created by schedule on 8/23/23.
//
//
// Created by schedule on 8/18/23.
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
#include "ns3/route-schedule.h"
#include "ns3/Ipv4EpsRouting.h"
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
NS_LOG_COMPONENT_DEFINE("ScratchFattreeWarmup");

int main(int argc,char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc,argv);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1458));
    Config::SetDefault("ns3::TcpSocket::DelAckCount",UintegerValue(1));
    Config::SetDefault("ns3::RttEstimator::InitialEstimation",TimeValue(MicroSeconds(100)));
//    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(2000)));
    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(12840)));
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity",TimeValue(MicroSeconds(1)));
    Config::SetDefault("ns3::TcpSocket::DataRetries",UintegerValue(100));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd",UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault("ns3::TcpSocketBase::Timestamp",BooleanValue(false));
    ns3::RngSeedManager::SetSeed(1530);
    ns3::RngSeedManager::SetRun(7);
    //    LogComponentEnable("TcpSocketBase",LOG_LOGIC);

    FatTreeHelper* ft = new FatTreeHelper(10);
//    FatTreeHelper* ft = new FatTreeHelper(10);
    ft->Create(false);
    uint32_t  queuenumber = 49;
    NodeContainer OCS;
    OCS.Create(1);
    Ptr<Node> ocsnode = OCS.Get(0);
//    QueueSize queueSize =  QueueSize("250kB");
    QueueSize queueSize =  QueueSize("40kB");
    MultiDeviceHelper OCSLinks =  MultiDeviceHelper(queuenumber,ocsnode,queueSize);
    ft->SetOcsMulti(OCSLinks,ocsnode,"nobypass");

    Ptr<AppPlanner> apl = new AppPlanner();
    double starttime = 0.001300;
    double addperu = 0.000260;
    int port = 10001;
    int portadd = 100;
    for(int i = 6;i<6+49;i++){
        apl->LongFlowPlan(ft->GetNodeInEdge((i)%49),ft->GetNodeInEdge(0),80,port,102400*1024, Seconds(starttime));
        starttime += addperu;
        port += portadd;
    }

//    Ptr<AppPlanner> aplmice[71];
//    for(int i=0;i<49;i++){
//        Ptr<AppPlanner> aplt = new AppPlanner;
//        aplmice[i] = aplt;
//        aplmice[i]->AddClientSet(ft->GetNodeInEdge(0));
//        aplmice[i]->AddServerSet(ft->GetNodeInEdge(i+1));
//        aplmice[i]->CreatePlanFromTrace("/home/venn/ns-allinone-3.38/ns-3.38/utils/FlowGenerate/tracedir/FlowTrace_data_"+std::to_string(i+1)+".csv");
//        std::cout<<"created "<<i+1<<std::endl;
//    }
//    apl->CreatePlanFromTrace("/home/venn/ns-allinone-3.38/ns-3.38/FlowTrace.csv");
    AsciiTraceHelper ascii;
    PointToPointHelper p2ph;
    p2ph.EnablePcap("HO0",ft->GetNodeInEdge(0));
    //    p2ph.EnablePcap("tor",ft->GetAllEdges());
    //    p2ph.EnablePcap("ocs",OCS);

    p2ph.EnableAscii(ascii.CreateFileStream("ocs.tr"),OCS);

    Simulator::Stop(Seconds(0.1));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}