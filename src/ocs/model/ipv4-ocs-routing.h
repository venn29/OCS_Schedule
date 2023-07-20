//
// Created by schedule on 5/26/23.
//

#ifndef NS3_IPV4OCSROUTING_H
#define NS3_IPV4OCSROUTING_H

#include <map>

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
class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Ipv4RoutingTableEntry;
class Ipv4MulticastRoutingTableEntry;
class Node;

class Ipv4OcsRouting : public Ipv4RoutingProtocol
{
  public:
    static TypeId GetTypeId();
    Ipv4OcsRouting();
    ~Ipv4OcsRouting() override;

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

    void SetDefaultRoute(Ipv4Address nextHop, uint32_t interface, uint32_t metric = 0);
    uint32_t GetNRoutes() const;
    Ipv4RoutingTableEntry GetRoute(uint32_t i) const;
    uint32_t GetMetric(uint32_t index) const;
    void RemoveRoute(uint32_t i);

    bool AddDeviceMatch(Ptr<NetDevice> dva,Ptr<NetDevice> dvb);

    void ClearDeviceMatch();

    bool DeviceMapEmpty();

    void SetNight(bool state){this->atnight = state;}
    bool GetNight(){return this->atnight;}

  protected:
    void DoDispose() override;


  private:
    /// Container for the network routes
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>> NetworkRoutes;
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>>::const_iterator NetworkRoutesCI;
    typedef std::list<std::pair<Ipv4RoutingTableEntry*, uint32_t>>::iterator NetworkRoutesI;
    bool LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric);
    Ptr<Ipv4Route> LookupOcs(Ipv4Address dest, Ptr<NetDevice> oif = nullptr);
   

    NetworkRoutes m_networkRoutes;
    Ptr<Ipv4> m_ipv4;

    //own
    //only when input device and output device matches, the forward is allowed
    typedef std::map<uint32_t ,uint32_t> MatchDevicesMap;
    MatchDevicesMap devicesMap;

    bool atnight;


};

} // namespace ns3

#endif // NS3_IPV4OCSROUTING_H
