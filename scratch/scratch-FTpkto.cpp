//
// Created by venn on 23-9-19.
//

//packet only


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
    //    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(11960)));
//    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(17680)));
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity",TimeValue(MicroSeconds(1)));
//    Config::SetDefault("ns3::TcpSocket::DataRetries",UintegerValue(100));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd",UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpNewReno::GetTypeId()));
    Config::SetDefault("ns3::TcpSocketBase::Timestamp",BooleanValue(false));
    ns3::RngSeedManager::SetSeed(1530);
    ns3::RngSeedManager::SetRun(7);
    //    LogComponentEnable("TcpSocketBase",LOG_LOGIC);

    //    FatTreeHelper* ft = new FatTreeHelper(10);
    FatTreeHelper* ft = new FatTreeHelper(12);
    ft->Create(true);
    uint32_t  queuenumber = 4;
    NodeContainer OCS;
    OCS.Create(1);
    Ptr<Node> ocsnode = OCS.Get(0);
    QueueSize queueSize =  QueueSize("400kB");
    MultiDeviceHelper OCSLinks =  MultiDeviceHelper(queuenumber,ocsnode,queueSize);
    ft->SetOcsMulti(OCSLinks,ocsnode,"warmup");


    Ptr<AppPlanner> apl = new AppPlanner();
    apl->LongFlowPlan(ft->GetNodeInEdge(6),ft->GetNodeInEdge(0),64,10001,102400*1024, Seconds(0.000520));
    apl->AddClientSet(ft->GetNodeInEdge(0));
    apl->AddServerSet(ft->GetNodeInEdge(6));
    //    apl->CreatePlanUniform(2500);
    apl->CreatePlanFromTrace("/home/venn/ns-allinone-3.38/ns-3.38/FlowTrace2W.csv");
    AsciiTraceHelper ascii;
    PointToPointHelper p2ph;
    p2ph.EnablePcap("HO0",ft->GetNodeInEdge(0));
    //    p2ph.EnablePcap("tor",ft->GetAllEdges());
    //    p2ph.EnablePcap("ocs",OCS);

    p2ph.EnableAscii(ascii.CreateFileStream("ocs.tr"),OCS);

    Simulator::Stop(Seconds(1));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}