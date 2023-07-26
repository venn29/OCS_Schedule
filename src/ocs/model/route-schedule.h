//
// Created by schedule on 5/25/23.
//for a circulate scheduler,we simply circulating between such routing policies:
//    1, 1-2,2-3,3-4......n-1-n,n~1 interval is 1
//    2, 1-3,2-4,3-5......n~2 interval is 2
//    ...
//    n-1,  1~n, 2~1,3~2....... interval is n-1
//    every set conclude n policies and there are totally n-1 sets
//

#ifndef NS3_ROUTE_SCHEDULE_H
#define NS3_ROUTE_SCHEDULE_H


#include "ns3/ipv4-l3-protocol.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ipv4-ocs-routing.h"
#include "MultiChannelPointToPoint.h"
#include "Ipv4EpsRouting.h"

namespace ns3
{

class RouteSchedule : public Object
{
  private:
    /* data */
    Time time_intervalNs;
    Time ReConfigurableNs;
    Time next_schedule_time;
    //the number of queue waiting
    uint32_t WaitingQueueSize;
    //one OCS for now
    Ptr<Node> OCS;
    //current routing policies set index
    uint32_t set_index;
    //number of sets
    uint32_t set_numbers;
    //number of policies
    uint32_t ocs_policy_numbers;

    Ptr<Ipv4OcsRouting> ocsrouting;

    //devices to OCS
    std::map<unsigned int,Ptr<MultiChannelPointToPointDevice>> devices_map;
    //routing to OCS
    std::map<unsigned int,Ptr<Ipv4EpsRouting>> eps_routing_map;
    //map this device to remote addr
    std::map<unsigned int,Ipv4InterfaceAddress>  addr_map;
    //map to the interface to the ocs node at each TOR
    std::map<unsigned int, uint32_t> iterface_map;

    Time finishtime;

//    Ptr<BufferRecorder> br;
    //
    std::ofstream* bufferoutfilestream;

    void DoBufferRecord();

  public:
    static TypeId GetTypeId();
    RouteSchedule();
    RouteSchedule(Time time_inter,Time time_config, Time start_time,uint32_t queuenumber,Ptr<Node> Ocs,Ptr<Ipv4OcsRouting> routing,Time finishtime);
    ~RouteSchedule() override;

    //may be changed
    const uint32_t EPSIndex = 0;


    //initialize routing policies, base on OCS and its devices
    void Initialize();
    //
    void SetRoutings( );

    bool SetNight();

    //
    bool DeleteRoutings();
    //
    bool SetRoutingsEPS();
    //
    bool DeleteRoutingsEPS();
    //
    bool ModifyRoutingsEPS();
    //
    bool StartSchedule();
    //
    void SetBufferOutFile(std::string FilePath){
        this->bufferoutfilestream = new std::ofstream();
        this->bufferoutfilestream->open(FilePath,std::ios::out | std::ios::trunc);
    }


//    void SetBufferRecorder(Ptr<BufferRecorder> pbr ){this->br = pbr;}
};


}



#endif // NS3_ROUTE_SCHEDULE_H
