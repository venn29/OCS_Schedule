//
// Created by venn on 23-8-1.
//

#ifndef NS3_IPV4SINGLEEPSROUTING_H
#define NS3_IPV4SINGLEEPSROUTING_H

#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

#include <list>
#include <stdint.h>
#include <utility>

namespace ns3
{
class Ipv4RoutingTableEntry;
class Ipv4SingleEPSRouting : public Ipv4RoutingProtocol
{
  public:
    static TypeId GetTypeId();
    Ipv4SingleEPSRouting();
    ~Ipv4SingleEPSRouting() override;

    Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr) override;

    bool RouteInput(Ptr<const Packet> p,
                    const Ipv4Header& header,
                    Ptr<const NetDevice> idev,
                    UnicastForwardCallback ucb,
                    MulticastForwardCallback mcb,
                    LocalDeliverCallback lcb,
                    ErrorCallback ecb) override;

    void NotifyInterfaceUp(uint32_t interface) override;
    void NotifyInterfaceDown(uint32_t interface) override;
    void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void SetIpv4(Ptr<Ipv4> ipv4) override;
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S) const override;

    void AddNetworkRouteTo(Ipv4Address network,
                           Ipv4Mask networkMask,
                           Ipv4Address nextHop,
                           uint32_t interface,
                           uint32_t metric = 0);

    void AddNetworkRouteTo(Ipv4Address network,
                           Ipv4Mask networkMask,
                           uint32_t interface,
                           uint32_t metric = 0);

    uint32_t GetNRoutes() const;
    Ipv4RoutingTableEntry GetRoute(uint32_t i) const;
    uint32_t GetMetric(uint32_t index) const;
    void SetDefaultRoute(Ipv4Address nextHop, uint32_t interface, uint32_t metric = 0);
    bool LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric);
    void RemoveRoute(uint32_t i);

    void BeginConfig(Ipv4Address new_addr);
    void BeginWork();
    void SetDevicetoOcs(Ptr<NetDevice> nd){
        this->to_ocs_device = nd;
    }
    void SetDestAddr(Ipv4Address ip){this->ocs_dest_addr = ip;};


  protected:
    void DoDispose() override;

  private:
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>> NetworkRoutes;
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>>::const_iterator NetworkRoutesCI;
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>>::iterator NetworkRoutesI;
    bool LookupSingleRoute(const Ipv4RoutingTableEntry& route, uint32_t metric);
    Ptr<Ipv4Route> LookupSingleEps(Ipv4Address dest, Ptr<NetDevice> oif = nullptr);

    NetworkRoutes m_networkRoutes;
    Ptr<Ipv4> m_ipv4;
    Ptr<NetDevice> to_ocs_device;
    Ipv4Address ocs_dest_addr;
};
}
#endif // NS3_IPV4SINGLEEPSROUTING_H
