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
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1446));
    Config::SetDefault("ns3::TcpSocket::DelAckCount",UintegerValue(1));
    Config::SetDefault("ns3::RttEstimator::InitialEstimation",TimeValue(MicroSeconds(100)));
    Config::SetDefault("ns3::TcpSocketBase::MinRto",TimeValue(MicroSeconds(650)));
    Config::SetDefault("ns3::TcpSocketBase::ClockGranularity",TimeValue(MicroSeconds(1)));
    Config::SetDefault("ns3::TcpSocket::DataRetries",UintegerValue(100));
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",BooleanValue(true));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd",UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TcpNewReno::GetTypeId()));

    FatTreeHelper* ft = new FatTreeHelper(4);
    ft->Create();
    uint32_t  queuenumber = 4;
    NodeContainer OCS;
    OCS.Create(1);
    Ptr<Node> ocsnode = OCS.Get(0);
    QueueSize queueSize =  QueueSize("1000kB");
    MultiDeviceHelper OCSLinks =  MultiDeviceHelper(queuenumber,ocsnode,queueSize);
    ft->SetOcsMulti(OCSLinks,ocsnode);


    Ptr<AppPlanner> apl = new AppPlanner();
    apl->LongFlowPlan(ft->GetNodeInEdge(3),ft->GetNodeInEdge(0),2,10001,10240*1024, Seconds(0.000001));
    AsciiTraceHelper ascii;
    PointToPointHelper p2ph;
    p2ph.EnablePcap("host",ft->GetNodeInEdge(0));
    p2ph.EnablePcap("tor",ft->GetAllEdges());
    p2ph.EnablePcap("ocs",OCS);
    Simulator::Stop(Seconds(1));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}