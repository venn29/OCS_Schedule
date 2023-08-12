//
// Created by venn on 23-8-9.
//

#include "FatTreeHelper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"

/* Address Resolution Macros */

/* Link between root and aggrgation. root+1.pod+1.aggr+1.(1|2) */
#define ROOTAGGR(root, pod, aggr)		\
  root + 1 << "." << pod + 1 << "." << aggr + 1 << ".0"

/* Link between aggrgation and edge. pod+1+100.aggr+1.edge+1.(1|2) */
#define AGGREDGE(pod, aggr, edge)			\
  pod + 101 << "." << aggr + 1 << "." << edge + 1 << ".0"

/* Link between edge and end node. pod+1+200.edge+1.node+1.(1|2) */
#define EDGENODE(pod, edge, node)		\
  pod + 201 << "." << edge + 1 << "." << node + 1 << ".0"


using namespace ns3;

FatTreeHelper::FatTreeHelper()
    : podnum(4)
{
    SetSWnum();
}

FatTreeHelper::FatTreeHelper(int pn)
    : podnum(pn)
{
    SetSWnum();
}

void
FatTreeHelper::SetSWnum()
{
    this->podnum2 = this->podnum/2;
    agginpodnum = podnum2;
    edgeinpodnum = podnum2;
    nodeinedgenum = podnum2; //temporary

    nodeinpodnum = nodeinedgenum * edgeinpodnum;

    rootswnum = podnum2 * podnum2;
    aggrswnum = agginpodnum * podnum;
    edgeswnum = edgeinpodnum * podnum;
    nodenum = nodeinpodnum * podnum;

}

void
FatTreeHelper::Create()
{
    //Create Node
    this->rootsw.Create(this->rootswnum);
    this->aggrsw.Create(this->aggrswnum);
    this->edgesw.Create(this->edgeswnum);
    for(int i=0;i<this->edgeswnum;i++)
    {
        this->nodes.push_back(new NodeContainer(this->nodeinedgenum));
    }

    InternetStackHelper stack;
    stack.Install(this->rootsw);
    stack.Install(this->aggrsw);
    stack.Install(this->edgesw);
    for(int i=0;i<this->edgeswnum;i++)
    {
        stack.Install(*this->nodes[i]);
    }

    Ipv4AddressHelper address;
    //set up links
    //root and aggr
    for(int pod = 0;pod < this->podnum;pod++)
    {
        for(int root = 0;root<this->rootswnum;root++)
        {
//            int linkn = this->podnum *  pod + root;     //index of link
            int aggr = (int) (root/this->podnum2);      //witch aggsw the root connect to (in pod)
            int aggrn = this->agginpodnum*pod + aggr;    //witch aggsw the root connect to (in all aggrsw)
            NodeContainer nc;
            NetDeviceContainer ndc;
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate",StringValue("10Gbps"));
            p2p.SetChannelAttribute ("Delay", StringValue ("4us"));
            p2p.DisableFlowControl();

            nc = NodeContainer(this->rootsw.Get(root),this->aggrsw.Get(aggrn));
            ndc = p2p.Install(nc);

            //allocate addr
            //to-do:
            std::stringstream  addrbase;
            addrbase << ROOTAGGR(root,pod,aggr);
            address.SetBase(addrbase.str().c_str(),"255.255.255.0");
            Ipv4InterfaceContainer rootIface = address.Assign(ndc);
        }
    }

    //aggr and edge
    for (int pod = 0; pod < this->podnum; pod++)
    {
        for (int aggr = 0; aggr < this->agginpodnum; aggr++)
        {
            for (int edge = 0; edge < edgeinpodnum; edge++)
            {
//                int linkn = ((this->edgeinpodnum * this->agginpodnum * pod) + this->edgeinpodnum * aggr + edge);
                int aggrn = this->agginpodnum * pod + aggr;
                int edgen = edgeinpodnum * pod + edge;
                NodeContainer nc;
                NetDeviceContainer ndc;
                PointToPointHelper p2p;
                p2p.SetDeviceAttribute("DataRate",StringValue("10Gbps"));
                p2p.SetChannelAttribute ("Delay", StringValue ("4us"));


                nc = NodeContainer(this->aggrsw.Get(aggrn),this->edgesw.Get(edgen));
                ndc = p2p.Install(nc);
                this->aggtordevs.push_back(&ndc);
                //allocate addr
                std::stringstream  addrbase;
                addrbase << AGGREDGE(pod,aggr,edge);
                address.SetBase(addrbase.str().c_str(),"255.255.255.0");
                Ipv4InterfaceContainer rootIface = address.Assign(ndc);

                this->SetPfifoSizeQueueDisc(ndc.Get(1),nc.Get(1)->GetObject<TrafficControlLayer>());
            }
        }
    }

    //edge and host
    for (int pod = 0; pod < this->podnum; pod++)
    {
        for (int edge = 0; edge < this->edgeinpodnum; edge++)
        {
            for (int node = 0; node < this->nodeinedgenum; node++)
            {
//                int linkn = this->nodeinpodnum*pod + nodeinedgenum*edge + node;
                int edgen = edgeinpodnum * pod + edge;
//                int noden = nodeinpodnum*pod + nodeinedgenum*edge + node;

                NodeContainer nc;
                NetDeviceContainer ndc;
                PointToPointHelper p2p;
                p2p.SetDeviceAttribute("DataRate",StringValue("10Gbps"));
                p2p.SetChannelAttribute ("Delay", StringValue ("4us"));
                p2p.DisableFlowControl();

                nc = NodeContainer(this->edgesw.Get(edgen),this->nodes[edgen]->Get(node));
                ndc = p2p.Install(nc);
                //allocate addr
                //to do:
                std::stringstream  addrbase;
                addrbase << EDGENODE(pod,edge,node);
                address.SetBase(addrbase.str().c_str(),"255.255.255.0");
                Ipv4InterfaceContainer rootIface = address.Assign(ndc);
            }
        }
    }



    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}



Ptr<Node>
FatTreeHelper::GetRoot(int index)
{
    return this->rootsw.Get(index);
}

Ptr<Node>
FatTreeHelper::GetAggr(int index)
{
    return this->aggrsw.Get(index);
}

Ptr<Node>
FatTreeHelper::GetAggr(int pod, int indexpod)
{
    int aggrn = pod*this->agginpodnum + indexpod;
    return this->aggrsw.Get(aggrn);
}

Ptr<Node>
FatTreeHelper::GetEdge(int index)
{
    return this->rootsw.Get(index);
}


Ptr<Node>
FatTreeHelper::GetEdge(int pod, int indexpod)
{
    int edgen = pod*this->edgeinpodnum + indexpod;
    return this->edgesw.Get(edgen);
}

Ptr<Node>
FatTreeHelper::GetNode(int edgeindex, int index)
{
    return this->nodes[edgeindex]->Get(index);
}

Ptr<Node>
FatTreeHelper::GetNode(int pod,int edgeinpod, int index)
{
    int edgen = pod*this->edgeinpodnum+edgeinpod;
    return this->nodes[edgen]->Get(index);
}

NodeContainer
FatTreeHelper::GetNodeInEdge(int edgeindex)
{
    return *this->nodes[edgeindex];

}
