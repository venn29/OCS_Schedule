//
// Created by venn on 23-8-1.
//

#include "Ipv4SingleEPSRouting.h"
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
NS_LOG_COMPONENT_DEFINE("Ipv4SingleEPSRouting");
NS_OBJECT_ENSURE_REGISTERED(Ipv4SingleEPSRouting);
TypeId
Ipv4SingleEPSRouting::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv4SingleEPSRouting")
                                .SetParent<Ipv4RoutingProtocol>()
                                .SetGroupName("Ocs")
                                .AddConstructor<Ipv4SingleEPSRouting>();
    return tid;
}

Ipv4SingleEPSRouting::Ipv4SingleEPSRouting()
    :m_ipv4(nullptr),
      to_ocs_device(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Ipv4SingleEPSRouting::~Ipv4SingleEPSRouting()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Ipv4Route>
Ipv4SingleEPSRouting::RouteOutput(Ptr<Packet> p,
                            const Ipv4Header& header,
                            Ptr<NetDevice> oif,
                            Socket::SocketErrno& sockerr) {
    return nullptr;
}

bool
Ipv4SingleEPSRouting::RouteInput(Ptr<const Packet> p,
                                 const Ipv4Header& ipHeader,
                                 Ptr<const NetDevice> idev,
                                 UnicastForwardCallback ucb,
                                 MulticastForwardCallback mcb,
                                 LocalDeliverCallback lcb,
                                 ErrorCallback ecb)
{
    NS_LOG_FUNCTION(this << p << ipHeader << ipHeader.GetSource() << ipHeader.GetDestination()
                         << idev << &ucb << &mcb << &lcb << &ecb);
    NS_ASSERT(m_ipv4);
    NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
    uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);
    if (ipHeader.GetDestination().IsMulticast())
    {
        NS_LOG_LOGIC("Multicast destination");
        NS_LOG_LOGIC("Multicast route not found");
        return false; // Let other routing protocols try to handle this
    }
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
            return false;
        }
    }
    if (m_ipv4->IsForwarding(iif) == false)
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
        return true;
    }
    Ptr<Packet> packet = p->Copy();
    //we do not have any entry for the downlink acks
    Ptr<Ipv4Route> rtentry = LookupSingleEps(ipHeader.GetDestination());
    //down traffic
    if(!rtentry)
        return false;

    bool OcsRoute = (rtentry->GetDestination() == this->ocs_dest_addr);
    if(!OcsRoute)
        return false;
    else
    {
        ucb(rtentry,packet,ipHeader);
        return true;
    }
}

Ptr<Ipv4Route>
Ipv4SingleEPSRouting::LookupSingleEps(ns3::Ipv4Address dest, Ptr<ns3::NetDevice> oif)
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
Ipv4SingleEPSRouting::BeginConfig(Ipv4Address new_addr)
{
    //    this->atnight = true;
    //update TOR addr
    this->ocs_dest_addr = new_addr;
}


void
Ipv4SingleEPSRouting::NotifyInterfaceUp(uint32_t i)
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
Ipv4SingleEPSRouting::NotifyInterfaceDown(uint32_t i)
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
Ipv4SingleEPSRouting::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
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
Ipv4SingleEPSRouting::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
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
Ipv4SingleEPSRouting::SetIpv4(Ptr<Ipv4> ipv4)
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
Ipv4SingleEPSRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
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
Ipv4SingleEPSRouting::AddNetworkRouteTo(Ipv4Address network,
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
Ipv4SingleEPSRouting::SetDefaultRoute(Ipv4Address nextHop, uint32_t interface, uint32_t metric)
{
    NS_LOG_FUNCTION(this << nextHop << " " << interface << " " << metric);
    AddNetworkRouteTo(Ipv4Address("0.0.0.0"), Ipv4Mask::GetZero(), nextHop, interface, metric);
}

void
Ipv4SingleEPSRouting::AddNetworkRouteTo(Ipv4Address network,
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
Ipv4SingleEPSRouting::LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric)
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
Ipv4SingleEPSRouting::GetNRoutes() const
{
    NS_LOG_FUNCTION(this);
    return m_networkRoutes.size();
}

Ipv4RoutingTableEntry
Ipv4SingleEPSRouting::GetRoute(uint32_t index) const
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
Ipv4SingleEPSRouting::GetMetric(uint32_t index) const
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
Ipv4SingleEPSRouting::RemoveRoute(uint32_t index)
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
Ipv4SingleEPSRouting::DoDispose()
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