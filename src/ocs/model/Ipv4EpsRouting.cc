//
// Created by schedule on 6/1/23.
//





#include "Ipv4EpsRouting.h"
#include "MultiChannelPointToPoint.h"
#include "OcsUtils.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-route.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/queue-disc.h"
#include "ns3/queue.h"


#include <iomanip>
#include <algorithm>
#include <utility>



namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4EpsRouting");

NS_OBJECT_ENSURE_REGISTERED(Ipv4EpsRouting);

//override methods
TypeId
Ipv4EpsRouting::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv4EpsRouting")
                            .SetParent<Ipv4RoutingProtocol>()
                            .SetGroupName("Ocs")
                            .AddConstructor<Ipv4EpsRouting>();
    return tid;
}
Ipv4EpsRouting::Ipv4EpsRouting(int queue_number,uint32_t mice_threshold, double queue_space_threshold,double bypasspro)
    : m_ipv4(nullptr),
      to_ocs_device(nullptr),
      ocs_dest_addr(std::vector<Ipv4Address>()),
      queue_number(queue_number),
      working_queue_index(0),
      mice_thresh(mice_threshold),
      queue_space_thresh(queue_space_threshold),
      enqueued_flows(std::vector<std::map<size_t ,Ptr<Flow>>> (queue_number)),
      atnight(false),
      bypassrandom(CreateObject<UniformRandomVariable>()),
      bypassprobility(bypasspro)
{
    NS_LOG_FUNCTION(this);
}


Ipv4EpsRouting::Ipv4EpsRouting()
    : m_ipv4(nullptr),
      to_ocs_device(nullptr),
      ocs_dest_addr(std::vector<Ipv4Address>()),
      queue_number(2),
      working_queue_index(0),
      mice_thresh(10),
      queue_space_thresh(0.5),
      enqueued_flows(std::vector<std::map<size_t,Ptr<Flow>>> (2)),
      atnight(false),
      bypassrandom(CreateObject<UniformRandomVariable>()),
      bypassprobility(0.3)
{
    NS_LOG_FUNCTION(this);
}

Ipv4EpsRouting::~Ipv4EpsRouting()
{
    NS_LOG_FUNCTION(this);
}

//won't be used at any switch
Ptr<Ipv4Route>
Ipv4EpsRouting::RouteOutput(Ptr<Packet> p,
                           const Ipv4Header& header,
                           Ptr<NetDevice> oif,
                           Socket::SocketErrno& sockerr) {
    return nullptr;
}


//the packet is out of ip header
bool
Ipv4EpsRouting::RouteInput(Ptr<const Packet> p,
           const Ipv4Header& ipHeader,
           Ptr<const NetDevice> idev,
           UnicastForwardCallback ucb,
           MulticastForwardCallback mcb,
           LocalDeliverCallback lcb,
           ErrorCallback ecb)
{

    if(this->m_bypass == pktonly)
        return false;
    NS_LOG_FUNCTION(this << p << ipHeader << ipHeader.GetSource() << ipHeader.GetDestination()
                         << idev << &ucb << &mcb << &lcb << &ecb);

//    std::cout <<Simulator::Now().GetSeconds() << p << ipHeader << ipHeader.GetSource() << ipHeader.GetDestination()
//                         << idev << &ucb << &mcb << &lcb << &ecb<<"\n";
//    Time t = Simulator::Now();
//    std::cout<<t<<"\n";
    NS_ASSERT(m_ipv4);
    // Check if input device supports IP
    NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
    uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

    //    // Multicast recognition; handle local delivery here
    //
    if (ipHeader.GetDestination().IsMulticast())
    {
        NS_LOG_LOGIC("Multicast destination");
        NS_LOG_LOGIC("Multicast route not found");
        return false; // Let other routing protocols try to handle this
    }
//    Time t = Simulator::Now();
//    std::cout<<t<<"\n";

    if (m_ipv4->IsDestinationAddress(ipHeader.GetDestination(), iif))
    {
        if (!lcb.IsNull())
        {
            NS_LOG_LOGIC("Local delivery to " << ipHeader.GetDestination());
            lcb(p, ipHeader, iif);
            return true;
        }
        else
        {
            // The local delivery callback is null.  This may be a multicast
            // or broadcast packet, so return false so that another
            // multicast routing protocol can handle it.  It should be possible
            // to extend this to explicitly check whether it is a unicast
            // packet, and invoke the error callback if so
            return false;
        }
    }
    if (m_ipv4->IsForwarding(iif) == false)
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
        return true;
    }
    //to do: handle ack packets, just return false

    //first get tcp header and size(if any)
    //second, if ack or syn , return false(just for now. Then we can deliver them to ocs, only when it's connected)
    //third, if common packet with small size , return false, send to EPS
    //fourth, if common packets with big size but not connected

    //read tcp header
    Ptr<Packet> packet = p->Copy();
    auto protocal = ipHeader.GetProtocol();
    TcpHeader tcpHeader;
    UdpHeader udpHeader;
    uint16_t dstport;
    uint16_t srcport;
    //tcp
    if(protocal == 6)
    {
        packet->PeekHeader(tcpHeader);
        int syn = tcpHeader.GetFlags() & TcpHeader::SYN;
        // we can transfer then through ocs, when they can, too. But it needs metadata
        if (syn)
        {
            return false;
        }
        dstport = tcpHeader.GetDestinationPort();
        srcport = tcpHeader.GetSourcePort();
    }
    else if (protocal == 17)
    {
        packet->PeekHeader(udpHeader);
        dstport = udpHeader.GetDestinationPort();
        srcport = udpHeader.GetSourcePort();
    }
    else
        return false;

//    std::cout<<dstport<<std::endl;
    //we do not have any entryst for the downlink acks
    Ptr<Ipv4Route> rtentry = LookupEps(ipHeader.GetDestination());
    //down traffic
    if(!rtentry)
        return false;


    OcsTag metaData;
    bool tag_found = packet->RemovePacketTag(metaData);
    uint32_t q_idx = GetQueueIdx(rtentry->GetDestination());
    uint32_t size = metaData.GetLeftSize();

    // Get flow size and get the q_idx, no reason ,just speed up the program, it can be split anytime
    //    auto size = GetLeftFlowSize(packet,q_idx);

    //ack packets
//    if(!tag_found)
//    {
//        //other acks
//        if(q_idx != working_queue_index)
//            return false;
//        //if tcp
//        if(protocal == 6)
//        {
//            auto acknumber = tcpHeader.GetAckNumber().GetValue();
//            if(acknumber > 1)
//            {
//                metaData.SetQueueIdx(working_queue_index);
//                packet->AddPacketTag(metaData);
//                ucb(rtentry,packet,ipHeader);
//                return true;
//            }
//        }
//    }

    if(!tag_found)
    {
//        std::cout<<"notag"<<std::endl;
        return false;
    }

    if(size < this->mice_thresh)
    {
        std::cout<<"small"<<std::endl;
        return false;
    }
    //    std::cout<<"big"<<std::endl;
    Ptr<Flow> pflow = this->Getflow(Flow::HashPacket(ipHeader.GetSource(),ipHeader.GetDestination(),protocal,srcport,dstport));
    if(!pflow)
        pflow = this->AddUpFlow(ipHeader.GetSource(),ipHeader.GetDestination(),srcport,dstport,protocal);



    //big flow packets of other destinations, drop
    if(q_idx<0 || q_idx >= queue_number){
        if(pflow->GetLostFlag())
            return false;
        if(ns3::Simulator::Now().GetSeconds() - pflow->GetDroptime() >=0.000520 &&pflow->GetDroptime()!=0)
        {
            pflow->SetLostFlag(true);
//            std::cout<<"flow"<<pflow->Getdstport()<<",now "<<ns3::Simulator::Now().GetSeconds()<<", drop time "<<pflow->GetDroptime()<<std::endl;
            return false;
        }
        NS_LOG_LOGIC("Out of queue number");
        pflow->DropPacket();
        ecb(p,ipHeader,Socket::SOCKET_ERRNO_LAST);
        //drop
        return true;
    }
    //bypass strategy
    if(q_idx != this->working_queue_index)
    {
        pflow->SetLostFlag(false);
//        bool flow_enqueued = SearchFlowEnqueued(ipHeader.GetSource(),ipHeader.GetDestination(),srcport,dstport,protocal,q_idx);
        bool flow_enqueued = pflow->GetEnqueueStatus();
        double queue_space = GetQueueSpace(q_idx);
        NS_LOG_LOGIC(queue_space<<","<<this->queue_space_thresh<<"\n");
//        std::cout<<"space: "<<queue_space<<"thresh: "<<this->queue_space_thresh<<std::endl;
        if(queue_space > this->queue_space_thresh && !flow_enqueued)
        {
            if (this->m_bypass == randomize)
            {
                double randomv = GetBypassRandom();
                if (randomv < this->bypassprobility)
                    return false;
            }
            else if (m_bypass == cwndbased)
            {
                //going bypass, we do not take udp into consideration
                if( pflow->GetSent() < pflow->GetWarmupThresh() && protocal == 6)
                {
                	pflow->ReceiveSequence();
                	return false;
                }
                    
            }
        }
        else
            return false;
        if(!flow_enqueued){
            pflow->SetEnqueued();
        }
    }
    //enqueue, we did not consider what if the queue is full
    //todo

    pflow->ReceiveSequence();
    pflow->SetLostFlag(false);
    pflow->ResetDropTime();
    metaData.SetQueueIdx(q_idx);
    packet->AddPacketTag(metaData);
    ucb(rtentry,packet,ipHeader);

    return true;
}

//own methods
Ptr<Ipv4Route>
Ipv4EpsRouting::LookupEps(ns3::Ipv4Address dest, Ptr<ns3::NetDevice> oif)
{
    NS_LOG_FUNCTION(this << dest << " " << oif);
    Ptr<Ipv4Route> rtentry = nullptr;
    uint16_t longest_mask = 0;
    uint32_t shortest_metric = 0xffffffff;
    if (dest.IsLocalMulticast())
    {
        NS_ASSERT_MSG(
            oif,
            "Try to send on link-local multicast address, and no interface index is given!");

        rtentry = Create<Ipv4Route>();
        rtentry->SetDestination(dest);
        rtentry->SetGateway(Ipv4Address::GetZero());
        rtentry->SetOutputDevice(oif);
        rtentry->SetSource(m_ipv4->GetAddress(m_ipv4->GetInterfaceForDevice(oif), 0).GetLocal());
        return rtentry;
    }
    for (NetworkRoutesI i = m_networkRoutes.begin(); i != m_networkRoutes.end(); i++)
    {
        Ipv4RoutingTableEntry* j = i->first;
        uint32_t metric = i->second;
        Ipv4Mask mask = (j)->GetDestNetworkMask();
        uint16_t masklen = mask.GetPrefixLength();
        Ipv4Address entry = (j)->GetDestNetwork();
        NS_LOG_LOGIC("Searching for route to " << dest << ", checking against route to " << entry
                                               << "/" << masklen);
        if (mask.IsMatch(dest, entry))
        {
            NS_LOG_LOGIC("Found global network route " << j << ", mask length " << masklen
                                                       << ", metric " << metric);
            if (oif)
            {
                if (oif != m_ipv4->GetNetDevice(j->GetInterface()))
                {
                    NS_LOG_LOGIC("Not on requested interface, skipping");
                    continue;
                }
            }
            if (masklen < longest_mask) // Not interested if got shorter mask
            {
                NS_LOG_LOGIC("Previous match longer, skipping");
                continue;
            }
            if (masklen > longest_mask) // Reset metric if longer masklen
            {
                shortest_metric = 0xffffffff;
            }
            longest_mask = masklen;
            if (metric > shortest_metric)
            {
                NS_LOG_LOGIC("Equal mask length, but previous metric shorter, skipping");
                continue;
            }
            shortest_metric = metric;
            Ipv4RoutingTableEntry* route = (j);
            uint32_t interfaceIdx = route->GetInterface();
            rtentry = Create<Ipv4Route>();
            rtentry->SetDestination(route->GetDest());
            rtentry->SetSource(m_ipv4->SourceAddressSelection(interfaceIdx, route->GetDest()));
            rtentry->SetGateway(route->GetGateway());
            rtentry->SetOutputDevice(m_ipv4->GetNetDevice(interfaceIdx));
            if (masklen == 32)
            {
                break;
            }
        }
    }
    if (rtentry)
    {
        NS_LOG_LOGIC("Matching route via " << rtentry->GetGateway() << " at the end");
    }
    else
    {
        NS_LOG_LOGIC("No matching route to " << dest << " found");
    }
    return rtentry;

}


void
Ipv4EpsRouting::AddEnqueuedFlow(uint32_t q_idx,Ipv4Address srcIp,Ipv4Address destIp, uint16_t src_port, uint16_t dst_port, uint8_t protocol  )
{
    auto f = new Flow(srcIp,destIp,protocol,src_port,dst_port,this->ssthresh);
    auto hash = f->HashFLow();
    auto m = this->enqueued_flows[q_idx];
    m.insert(std::make_pair(hash,f));
}

Ptr<Flow>
Ipv4EpsRouting::AddUpFlow(Ipv4Address srcIp,
                          Ipv4Address destIp,
                          uint16_t src_port,
                          uint16_t dst_port,
                          uint8_t protocol)
{
    auto f = new Flow(srcIp,destIp,protocol,src_port,dst_port,this->ssthresh);
    auto hash = f->HashFLow();
    this->all_upflows.insert(std::make_pair(hash,f));
    return f;
}

//should return which queue to invoke
uint32_t
Ipv4EpsRouting::BeginConfig(Ipv4Address new_addr)
{
//    this->atnight = true;
    //update TOR addr
    this->ocs_dest_addr[this->working_queue_index] = new_addr;
    this->working_queue_index = (this->working_queue_index + 1) % this->queue_number;
    //clear dropped flows for new working queue
    this->enqueued_flows[this->working_queue_index].clear();
    return this->working_queue_index;
}

uint32_t
Ipv4EpsRouting::BeginConfig()
{
    this->working_queue_index = (this->working_queue_index + 1) % this->queue_number;
    this->enqueued_flows[this->working_queue_index].clear();
    return this->working_queue_index;
}

void
Ipv4EpsRouting::BeginWork()
{
//    this->atnight = false;
}
//
//uint32_t
//Ipv4EpsRouting::GetLeftFlowSize(Ptr< ns3::Packet> p,uint16_t q_idx)
//{
//    uint32_t left_size=0;
//    Ipv4Header ip;
//    p->RemoveHeader(ip);
//    auto proto = ip.GetProtocol();
//    if(proto == 6){
//        TcpHeader tcp;
//        p->RemoveHeader(tcp);
//        MetadataHeader meta;
//        p->RemoveHeader(meta);
//        meta->SetDestQueue(q_idx);
//        p->AddHeader(meta);
//        p->AddHeader(tcp);
//    }
//    //udp
//    else if(proto == 17)
//    {
//        UdpHeader udp;
//        p->RemoveHeader(udp);
//        MetadataHeader meta;
//        p->RemoveHeader(meta);
//        meta->SetDestQueue(q_idx);
//        p->AddHeader(meta);
//        p->AddHeader(udp);
//    }
//    p->AddHeader(ip);
//    return left_size;
//}

//node->device->m_queues->queue->Size
double
Ipv4EpsRouting::GetQueueSpace(uint32_t q_idx)
{
    Ptr<MultiChannelPointToPointDevice> md = DynamicCast<MultiChannelPointToPointDevice>(this->to_ocs_device);
    if(!md)
        return -1;
    Ptr<Queue<Packet>> target_queue =  md->GetQueue(q_idx);
    uint32_t totalsize = target_queue->GetMaxSize().GetValue();
    uint32_t currentsize = target_queue->GetCurrentSize().GetValue();
    return (totalsize-currentsize)*1.0/(totalsize*1.0);
}

uint32_t
Ipv4EpsRouting::GetQueueIdx(Ipv4Address dest)
{
    //we'd better use the rtentry and match it
    //to be modified
    for(uint16_t i=0;i<this->ocs_dest_addr.size();i++){
        if(dest == this->ocs_dest_addr[i])
            return i;
    }
    return -1;
}

bool
Ipv4EpsRouting::SearchFlowEnqueued(Ipv4Address srcIp,Ipv4Address destIp, uint16_t src_port, uint16_t dst_port, uint8_t protocol,uint32_t q_idx )
{
    size_t hash = Flow::HashPacket(srcIp,destIp,protocol,src_port,dst_port);
    auto flow_map = this->enqueued_flows[q_idx];
    auto it = flow_map.find(hash);
    if(it==flow_map.end())
        return false;
    else{
        if(it->second->EqualFLow(srcIp,destIp,protocol,src_port,dst_port))
            return true;
        else
            return false;
    }
}


void
Ipv4EpsRouting::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    // If interface address and network mask have been set, add a route
    // to the network of the interface (like e.g. ifconfig does on a
    // Linux box)
    for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++)
    {
        if (m_ipv4->GetAddress(i, j).GetLocal() != Ipv4Address() &&
            m_ipv4->GetAddress(i, j).GetMask() != Ipv4Mask() &&
            m_ipv4->GetAddress(i, j).GetMask() != Ipv4Mask::GetOnes())
        {
            AddNetworkRouteTo(
                m_ipv4->GetAddress(i, j).GetLocal().CombineMask(m_ipv4->GetAddress(i, j).GetMask()),
                m_ipv4->GetAddress(i, j).GetMask(),
                i);
        }
    }
}

void
Ipv4EpsRouting::NotifyInterfaceDown(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    // Remove all static routes that are going through this interface
    for (NetworkRoutesI it = m_networkRoutes.begin(); it != m_networkRoutes.end();)
    {
        if (it->first->GetInterface() == i)
        {
            delete it->first;
            it = m_networkRoutes.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
Ipv4EpsRouting::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << " " << address.GetLocal());
    if (!m_ipv4->IsUp(interface))
    {
        return;
    }

    Ipv4Address networkAddress = address.GetLocal().CombineMask(address.GetMask());
    Ipv4Mask networkMask = address.GetMask();
    if (address.GetLocal() != Ipv4Address() && address.GetMask() != Ipv4Mask())
    {
        AddNetworkRouteTo(networkAddress, networkMask, interface);
    }
}

void
Ipv4EpsRouting::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << " " << address.GetLocal());
    if (!m_ipv4->IsUp(interface))
    {
        return;
    }
    Ipv4Address networkAddress = address.GetLocal().CombineMask(address.GetMask());
    Ipv4Mask networkMask = address.GetMask();
    // Remove all static routes that are going through this interface
    // which reference this network
    for (NetworkRoutesI it = m_networkRoutes.begin(); it != m_networkRoutes.end();)
    {
        if (it->first->GetInterface() == interface && it->first->IsNetwork() &&
            it->first->GetDestNetwork() == networkAddress &&
            it->first->GetDestNetworkMask() == networkMask)
        {
            delete it->first;
            it = m_networkRoutes.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
Ipv4EpsRouting::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_LOG_FUNCTION(this << ipv4);
    NS_ASSERT(!m_ipv4 && ipv4);
    m_ipv4 = ipv4;
    // we do not want this routing protocol to connect hosts or other switches which is not TOR
//    for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++)
//    {
//        if (m_ipv4->IsUp(i))
//        {
//            NotifyInterfaceUp(i);
//        }
//        else
//        {
//            NotifyInterfaceDown(i);
//        }
//    }
}

// Formatted like output of "route -n" command
void
Ipv4EpsRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this << stream);
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "Node: " << m_ipv4->GetObject<Node>()->GetId() << ", Time: " << Now().As(unit)
        << ", Local time: " << m_ipv4->GetObject<Node>()->GetLocalTime().As(unit)
        << ", Ipv4OcsRouting table" << std::endl;

    if (GetNRoutes() > 0)
    {
        *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface"
            << std::endl;
        for (uint32_t j = 0; j < GetNRoutes(); j++)
        {
            std::ostringstream dest;
            std::ostringstream gw;
            std::ostringstream mask;
            std::ostringstream flags;
            Ipv4RoutingTableEntry route = GetRoute(j);
            dest << route.GetDest();
            *os << std::setw(16) << dest.str();
            gw << route.GetGateway();
            *os << std::setw(16) << gw.str();
            mask << route.GetDestNetworkMask();
            *os << std::setw(16) << mask.str();
            flags << "U";
            if (route.IsHost())
            {
                flags << "HS";
            }
            else if (route.IsGateway())
            {
                flags << "GS";
            }
            *os << std::setw(6) << flags.str();
            *os << std::setw(7) << GetMetric(j);
            // Ref ct not implemented
            *os << "-"
                << "      ";
            // Use not implemented
            *os << "-"
                << "   ";
            if (!Names::FindName(m_ipv4->GetNetDevice(route.GetInterface())).empty())
            {
                *os << Names::FindName(m_ipv4->GetNetDevice(route.GetInterface()));
            }
            else
            {
                *os << route.GetInterface();
            }
            *os << std::endl;
        }
    }
    *os << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

void
Ipv4EpsRouting::AddNetworkRouteTo(Ipv4Address network,
                                  Ipv4Mask networkMask,
                                  Ipv4Address nextHop,
                                  uint32_t interface,
                                  uint32_t metric)
{
    NS_LOG_FUNCTION(this << network << " " << networkMask << " " << nextHop << " "
                         << interface << " " << metric);

    Ipv4RoutingTableEntry route =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, nextHop, interface);

    if (!LookupRoute(route, metric))
    {
        Ipv4RoutingTableEntry* routePtr = new Ipv4RoutingTableEntry(route);
        m_networkRoutes.emplace_back(routePtr, metric);
    }
}

void
Ipv4EpsRouting::SetDefaultRoute(Ipv4Address nextHop, uint32_t interface, uint32_t metric)
{
    NS_LOG_FUNCTION(this << nextHop << " " << interface << " " << metric);
    AddNetworkRouteTo(Ipv4Address("0.0.0.0"), Ipv4Mask::GetZero(), nextHop, interface, metric);
}

void
Ipv4EpsRouting::AddNetworkRouteTo(Ipv4Address network,
                                  Ipv4Mask networkMask,
                                  uint32_t interface,
                                  uint32_t metric)
{
    NS_LOG_FUNCTION(this << network << " " << networkMask << " " << interface << " " << metric);

    Ipv4RoutingTableEntry route =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, interface);
    if (!LookupRoute(route, metric))
    {
        Ipv4RoutingTableEntry* routePtr = new Ipv4RoutingTableEntry(route);

        m_networkRoutes.emplace_back(routePtr, metric);
    }
}

bool
Ipv4EpsRouting::LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric)
{
    for (NetworkRoutesI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        Ipv4RoutingTableEntry* rtentry = j->first;

        if (rtentry->GetDest() == route.GetDest() &&
            rtentry->GetDestNetworkMask() == route.GetDestNetworkMask() &&
            rtentry->GetGateway() == route.GetGateway() &&
            rtentry->GetInterface() == route.GetInterface() && j->second == metric)
        {
            return true;
        }
    }
    return false;
}

uint32_t
Ipv4EpsRouting::GetNRoutes() const
{
    NS_LOG_FUNCTION(this);
    return m_networkRoutes.size();
}

Ipv4RoutingTableEntry
Ipv4EpsRouting::GetRoute(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (NetworkRoutesCI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            return j->first;
        }
        tmp++;
    }
    NS_ASSERT(false);
    // quiet compiler.
    return nullptr;
}

uint32_t
Ipv4EpsRouting::GetMetric(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (NetworkRoutesCI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            return j->second;
        }
        tmp++;
    }
    NS_ASSERT(false);
    // quiet compiler.
    return 0;
}

void
Ipv4EpsRouting::RemoveRoute(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (NetworkRoutesI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            delete j->first;
            m_networkRoutes.erase(j);
            return;
        }
        tmp++;
    }
    NS_ASSERT(false);
}

void
Ipv4EpsRouting::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (NetworkRoutesI j = m_networkRoutes.begin(); j != m_networkRoutes.end();
         j = m_networkRoutes.erase(j))
    {
        delete (j->first);
    }
    m_ipv4 = nullptr;
    Ipv4RoutingProtocol::DoDispose();
}


}
