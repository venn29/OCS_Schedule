//
// Created by venn on 23-8-5.
//

#include "AppPlanner.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/new-bulk-send-helper.h"
#include "ns3/new-bulk-send-application.h"
#include <cmath>
#include <vector>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("AppPlanner");
NS_OBJECT_ENSURE_REGISTERED(AppPlanner);

AppPlanner::AppPlanner()
    :randomselecthost(CreateObject<UniformRandomVariable>()),
      randompoisson(CreateObject<UniformRandomVariable>()),
      flowthresh(1024*200),
      smallportstart(20001),
      bigportstart(10001),
      hostpertor(16),
      endtime(Seconds(1)),
      lambdaFlowPerSecond(50000)
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
    senderapph.SetAttribute("SendSize", UintegerValue(uint32_t(1458)));
    //APP Container
    ApplicationContainer sendappcontainer = senderapph.Install(client);
    sendappcontainer.Start(starttime);
    sendappcontainer.Stop(this->endtime);
    ApplicationContainer receiveappcontainer = receivapph.Install(server);
    receiveappcontainer.Start(starttime);
    receiveappcontainer.Stop(this->endtime);
}

void
AppPlanner::RegisterFlow(uint32_t flowsize,Time starttime,uint32_t from, uint32_t dst)
{
    //select client and dest port

    Ptr<Node> client = this->clients[0].Get(from);
    int port;
    if(flowsize <= this->flowthresh)
        port  = smallflow_portcurrent[0]++;
    else
        port = bigflow_portcurrent[0]++;
    //select server
    Ptr<Node> server = this->servers[0].Get(dst);
    Ipv4Address serveripv4 = server->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();

    //APP helper
    NewBulkSendHelper senderapph("ns3::TcpSocketFactory",InetSocketAddress(serveripv4,port));
    PacketSinkHelper receivapph("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),port));
    senderapph.SetAttribute("MaxBytes", UintegerValue(flowsize));
    senderapph.SetAttribute("SendSize", UintegerValue(uint32_t(1458)));
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
        std::cout<<intervalure<<std::endl;
    }
    std::cout<<"create "<<x<<" flows"<<std::endl;
}

void
AppPlanner::CreatePlanUniform(double flownum)
{
    Time intertime = this->endtime / flownum;
    Time timenow = Seconds(0);
    int x = 0;
    while(timenow <= this->endtime)
    {
        uint32_t fsize = this->fsd->GenerateFlowsizeByte();
        this->RegisterFlow(fsize,timenow);
        timenow+=intertime;
        x++;
    }
    std::cout<<"create "<<x<<" flows"<<std::endl;
}

void
AppPlanner::CreatePlanFromTrace(std::string path)
{
    std::ifstream csv_data(path,std::ios::in);
    if(!csv_data.is_open())
    {
        std::cout<<"Error open trace file fail"<<std::endl;
        exit(-1);
    }
    std::string line;
    std::vector<std::string> words;
    std::string word;
    std::istringstream sin;
    int flownum = 0;
    while(getline(csv_data,line))
    {
        words.clear();
        sin.clear();
        sin.str(line);
        while(getline(sin,word,','))
        {
            words.push_back(word);
        }
        int t = atoi(words[0].c_str());
        int dst = atoi(words[1].c_str());
        int size = atoi(words[2].c_str());
        this->RegisterFlow(size,NanoSeconds(t),dst,dst);
        flownum++;
    }


}

void
AppPlanner::LongFlowPlan(NodeContainer servernd, NodeContainer clientnd,int flownum, uint16_t port_start,uint32_t maxbytes,Time starttime)
{
    uint32_t servernodenum = servernd.GetN();
    uint32_t clientnodenum = clientnd.GetN();
    NS_ASSERT_MSG(servernodenum == clientnodenum, "Cluster to cluster servre number does not eqaul to client");
    int flowcreated = 0;
    uint16_t  port = port_start;
    while (flowcreated < flownum)
    {
//        ++port;
        for(uint32_t j = 0; j < servernodenum;j++)
        {
            //node
            Ptr<Node> server = servernd.Get(j);
            Ptr<Node> client = clientnd.Get(j);
            Ipv4Address serveripv4 = server->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
            //app helper
            NewBulkSendHelper senderapph("ns3::TcpSocketFactory",InetSocketAddress(serveripv4,port));
            PacketSinkHelper receivapph("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),port));
            senderapph.SetAttribute("MaxBytes",UintegerValue(maxbytes));
            senderapph.SetAttribute("SendSize", UintegerValue(uint32_t(1458)));
            //app container
            ApplicationContainer sendappc = senderapph.Install(client);
            sendappc.Start(starttime);
            ApplicationContainer receiveappc = receivapph.Install(server);
            receiveappc.Start(starttime);
            ++flowcreated;
            ++port;
            if(flowcreated >= flownum)
                break;
        }
    }
}

}
