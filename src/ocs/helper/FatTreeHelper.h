//
// Created by venn on 23-8-9.
//

#ifndef NS3_FATTREEHELPER_H
#define NS3_FATTREEHELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/queue-size.h"
#include "ns3/MultideviceHelper.h"
#include "ns3/ocs-helper.h"
#include "ns3/SingleRouteSchedule.h"
#include "ns3/route-schedule.h"
#include "ns3/Ipv4EpsRouting.h"
#include "ns3/Ipv4SingleEPSRouting.h"
namespace ns3
{
class FatTreeHelper
{
  public:
    FatTreeHelper();
    FatTreeHelper(int pn);
    void Create();

    void SetPodNUm(int pn) { this->podnum = pn;}
    int GetRootN() {return this->rootswnum;}
    int GetAggrN() {return this->aggrswnum;}
    int GetEdgeN() {return this->edgeswnum;}
    int GetNodeN() {return this->nodenum;}

    Ptr<Node> GetRoot(int index);
    Ptr<Node> GetAggr(int index);
    Ptr<Node> GetAggr(int pod,int indexpod);
    Ptr<Node> GetEdge(int index);
    Ptr<Node> GetEdge(int pod,int indexpod);
    Ptr<Node> GetNode(int edgeindex,int indexpod);
    Ptr<Node> GetNode(int pod,int edgeinpod,int indexpod);

    NodeContainer GetNodeInEdge(int edgeindex);
    NodeContainer GetAllEdges(){return this->edgesw;}

    void SetPrioQueueDisc();

    void SetOcsSingle(MultiDeviceHelper OCSLinkhelper,Ptr<Node> ocssw);

    void SetOcsMulti(MultiDeviceHelper OCSLinkhelper, Ptr<Node> ocssw,std::string routetype);

  private:
    void SetSWnum();
    int podnum;
    int podnum2;
    int agginpodnum;
    int edgeinpodnum;
    int nodeinedgenum;
    int nodeinpodnum;
    int rootswnum;
    int aggrswnum;
    int edgeswnum;
    int nodenum;



    NodeContainer rootsw;
    NodeContainer aggrsw;
    NodeContainer edgesw;
    std::vector<NodeContainer*> nodes;

    //device container
    std::vector<NetDeviceContainer*> rootaggdevs;
    std::vector<NetDeviceContainer*> aggtordevs;
    std::vector<NetDeviceContainer*> tornodedevs;
    std::vector<NetDeviceContainer> torocsdevs;

    void SetPfifoSizeQueueDisc(Ptr<NetDevice> dv,Ptr<TrafficControlLayer> tc)
    {
        Ptr<PointToPointNetDevice> p2pdv = DynamicCast<PointToPointNetDevice>(dv);
        p2pdv->SetQueueSize(QueueSize("3100B"));
        tc->DeleteRootQueueDiscOnDevice(dv);
        TrafficControlHelper tcHelper;
        tcHelper.SetRootQueueDisc("ns3::PFifoFlowSizeQueueDisc");
        tcHelper.Install(dv);
    }

    void EPSRouteInstallSingle(NodeContainer TORs, uint32_t queuenumber);

    void EPSRouteInstall(NodeContainer TORs, uint32_t queuenumber,std::string routetype);

};
}
#endif // NS3_FATTREEHELPER_H
