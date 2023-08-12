//
// Created by venn on 23-8-5.
//

#ifndef NS3_APPPLANNER_H
#define NS3_APPPLANNER_H

#include "ns3/node.h"
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/nstime.h"
#include "ns3/FlowSizeDistribution.h"
namespace ns3
{
class Time;
class AppPlanner : public Object
{
  public:
     static TypeId GetTypeId();
     AppPlanner();
     AppPlanner(uint32_t flowthresh,uint32_t smallport, uint32_t bigport,uint32_t hostpertor, Time endtime,double lambdaFlowPerSecond);
     ~AppPlanner();
     void AddClientSet(NodeContainer c);
     void AddServerSet(NodeContainer s);
     void RegisterFlow(uint32_t flowSize,Time starttime);
     void CreatePlanPoisson();
     void LongFlowPlan(NodeContainer servernd, NodeContainer clientnd,int flownum, uint16_t port_start,uint32_t maxbytes,Time starttime);
  private:
    std::vector<NodeContainer> clients;
    std::vector<NodeContainer> servers;
    std::vector<unsigned int> sendercountindex;
    std::vector<unsigned int> receivercountindex;
    std::vector<unsigned int> smallflow_portcurrent;
    std::vector<unsigned int> bigflow_portcurrent;
    Ptr<UniformRandomVariable> randomselecthost;
    Ptr<UniformRandomVariable> randompoisson;
    Ptr<FlowSizeDistribution> fsd;

    uint32_t flowthresh;
    uint32_t smallportstart;
    uint32_t bigportstart;
    uint32_t hostpertor;
    Time endtime;
    double lambdaFlowPerSecond; //for poisson planner
};
}

#endif // NS3_APPPLANNER_H
