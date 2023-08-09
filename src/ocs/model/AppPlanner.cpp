//
// Created by venn on 23-8-5.
//

#include "AppPlanner.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/new-bulk-send-helper.h"
#include "ns3/new-bulk-send-application.h"
#include <cmath>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("AppPlanner");
NS_OBJECT_ENSURE_REGISTERED(AppPlanner);

AppPlanner::AppPlanner()
    :randomselecthost(CreateObject<UniformRandomVariable>()),
      randompoisson(CreateObject<UniformRandomVariable>()),
      flowthresh(10*1500),
      smallportstart(20001),
      bigportstart(10001),
      hostpertor(16),
      endtime(Seconds(1)),
      lambdaFlowPerSecond(10000)
{
    NS_LOG_FUNCTION(this);
    this->fsd = new FlowSizeDistribution();
}

AppPlanner::AppPlanner(uint32_t flowthresh, uint32_t smallport, uint32_t bigport, uint32_t hostpertor, ns3::Time endtime, double lambdaFlowPerSecond)
    : randomselecthost(CreateObject<UniformRandomVariable>()),
      randompoisson(CreateObject<UniformRandomVariable>()),
      flowthresh(flowthresh),
      smallportstart(smallport),
      bigportstart(bigport),
      hostpertor(hostpertor),
      endtime(endtime),
      lambdaFlowPerSecond(lambdaFlowPerSecond)
{
        NS_LOG_FUNCTION(this);
}

AppPlanner::~AppPlanner()
{
    NS_LOG_FUNCTION(this);
}

TypeId
AppPlanner::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AppPlanner")
                        .SetParent<Object>()
                        .SetGroupName("Ocs")
                        .AddConstructor<AppPlanner>();
    return tid;
}

void
AppPlanner::AddClientSet(NodeContainer c)
{
    this->clients.push_back(c);
    this->sendercountindex.push_back(0);
    this->smallflow_portcurrent.push_back(smallportstart);
    this->bigflow_portcurrent.push_back(bigportstart);
}

void
AppPlanner::AddServerSet(NodeContainer s)
{
    this->servers.push_back(s);
    this->receivercountindex.push_back(0);
}

void
AppPlanner::RegisterFlow(uint32_t flowsize,Time starttime)
{
    //select client and dest port
    int cindex = this->randomselecthost->GetInteger(0,this->clients.size()-1);
    Ptr<Node> client = this->clients[cindex].Get(sendercountindex[cindex]);
    this->sendercountindex[cindex] = (sendercountindex[cindex] + 1)%hostpertor;
    int port;
    if(flowsize <= this->flowthresh)
        port  = smallflow_portcurrent[cindex]++;
    else
        port = bigflow_portcurrent[cindex]++;
    //select server
    int sindex = this->randomselecthost->GetInteger(0,this->servers.size()-1);
    Ptr<Node> server = this->servers[sindex].Get(receivercountindex[sindex]);
    Ipv4Address serveripv4 = server->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    this->receivercountindex[cindex] = (receivercountindex[cindex] + 1)%hostpertor;

    //APP helper
    NewBulkSendHelper senderapph("ns3::TcpSocketFactory",InetSocketAddress(serveripv4,port));
    PacketSinkHelper receivapph("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),port));
    senderapph.SetAttribute("MaxBytes", UintegerValue(flowsize));
    senderapph.SetAttribute("SendSize", UintegerValue(uint32_t(1446)));
    //APP Container
    ApplicationContainer sendappcontainer = senderapph.Install(client);
    sendappcontainer.Start(starttime);
    sendappcontainer.Stop(this->endtime);
    ApplicationContainer receiveappcontainer = receivapph.Install(server);
    receiveappcontainer.Start(starttime);
    receiveappcontainer.Stop(this->endtime);
}

void
AppPlanner::CreatePlanPoisson()
{
    Time timenow = Seconds(0);
    int x = 0;
    while (timenow <= this->endtime)
    {
        long intervalure = (long)(-std::log(this->randompoisson->GetValue()) / (this->lambdaFlowPerSecond/1e9));
        Time intertime = NanoSeconds(intervalure);
        timenow += intertime;
        uint32_t fsize = this->fsd->GenerateFlowsizeByte();
        this->RegisterFlow(fsize,timenow);
        ++x;
    }
    std::cout<<"create "<<x<<" flows";
}

}
