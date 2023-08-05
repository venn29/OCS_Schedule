//
// Created by venn on 23-8-2.
//

#include "SingleRouteSchedule.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3{
NS_LOG_COMPONENT_DEFINE("SingleRouteSchedule");
NS_OBJECT_ENSURE_REGISTERED(SingleRouteSchedule);

TypeId
SingleRouteSchedule::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SingleRouteSchedule")
                            .SetParent<Object>()
                            .SetGroupName("Ocs")
                            .AddConstructor<SingleRouteSchedule>();
    return tid;
}

SingleRouteSchedule::SingleRouteSchedule()
    :time_intervalNs(20),
      ReConfigurableNs(180),
      next_schedule_time(0),
      WaitingQueueSize(1),
      OCS(nullptr),
      ocsrouting(nullptr),
      finishtime(Seconds(10))
{
    NS_LOG_FUNCTION(this);
}

SingleRouteSchedule::SingleRouteSchedule(Time time_inter, Time time_config, Time start_time,uint32_t queuenumber,Ptr<Node> Ocs,Ptr<Ipv4OcsRouting> routing,Time finishtime)
{
    this->time_intervalNs = time_inter;
    this->ReConfigurableNs = time_config;
    this->next_schedule_time = start_time;
    this->WaitingQueueSize = queuenumber;
    this->OCS = Ocs;
    this->ocsrouting = routing;
    this->finishtime = finishtime;
    NS_LOG_FUNCTION(this);
}

SingleRouteSchedule::~SingleRouteSchedule()
{
    NS_LOG_FUNCTION(this);
}

void
SingleRouteSchedule::Initialize()
{
    this->set_index = 0;
    this->ocs_policy_numbers = this->OCS->GetNDevices()-1;
    this->set_numbers = ocs_policy_numbers-1;
    auto ocs_ipv4l3 = this->OCS->GetObject<Ipv4L3Protocol>();
    //init maps
    for(uint32_t i=0;i<this->ocs_policy_numbers;i++)
    {
        Ptr<NetDevice> dv = this->OCS->GetDevice(i);
        auto channel = dv->GetChannel();
        if(!channel)
            continue;
        Ptr<NetDevice> TORdv;
        for(std::size_t j=0;j<channel->GetNDevices();j++){
            TORdv = channel->GetDevice(j);
            if( TORdv != dv)
                break;
        }
        //        std::cout<<TORdv<<std::endl;
        Ptr<Node> TorNode = TORdv->GetNode();
        //        std::cout<<TorNode<<std::endl;
        auto ipv4l3 = TorNode->GetObject<Ipv4L3Protocol>();
        auto iif = ipv4l3->GetInterfaceForDevice(TORdv);
        this->iterface_map.insert(std::make_pair(i,iif));
        auto ocs_iif = ocs_ipv4l3->GetInterfaceForDevice(dv);
        //        std::cout<<iif<<std::endl;
        for (uint32_t j = 0; j < ipv4l3->GetNAddresses(iif); j++)
        {
            Ipv4InterfaceAddress iaddr = ipv4l3->GetAddress(iif, j);

            auto addr = iaddr.GetLocal();
            auto mask = iaddr.GetMask();
            this->ocsrouting->AddNetworkRouteTo(addr,mask,ocs_iif);
            // \to do: may occur Repetitive keys, we do not care now
            this->addr_map.insert(std::make_pair(i,iaddr));
        }
        //TOR multiple
        Ptr<MultiChannelPointToPointDevice> multi_tor = DynamicCast<MultiChannelPointToPointDevice>(TORdv);
        if(multi_tor){
            this->devices_map.insert(std::make_pair(i,multi_tor));
        }
        else{
            NS_LOG_ERROR("device "<<TORdv<<"is not a multichannel device");
        }
        //TOR Routing
        int16_t prio;
        Ptr<Ipv4RoutingProtocol> routing = ipv4l3->GetRoutingProtocol();
        Ptr<Ipv4ListRouting> list_routing = DynamicCast<Ipv4ListRouting>(routing);
        Ptr<Ipv4SingleEPSRouting> eps_routing;
        if(list_routing){
            Ptr<Ipv4RoutingProtocol> routing_2 = list_routing->GetRoutingProtocol(this->EPSIndex,prio);
            eps_routing = DynamicCast<Ipv4SingleEPSRouting>(routing_2);
        }
        else{
            eps_routing = DynamicCast<Ipv4SingleEPSRouting>(routing);
        }
        if (!eps_routing){
            NS_LOG_ERROR("the routing of"<<this->EPSIndex<<"of "<<TorNode<<"is not eps routing");
            return ;
        }
        else{
            this->eps_routing_map.insert(std::make_pair(i,eps_routing));
            eps_routing->SetDevicetoOcs(multi_tor);
        }
    }
    //load policies
    uint32_t  queuenumber = 1;
    uint32_t addr_cnt;
    for(uint32_t i=0;i<this->ocs_policy_numbers;i++)
    {
        auto interface = this->iterface_map.find(i)->second;
        Ptr<Ipv4SingleEPSRouting> epi = this->eps_routing_map.find(i)->second;
        addr_cnt = 0;
        for(uint32_t j = (i+1)%ocs_policy_numbers; j != i; j=(j+1)%ocs_policy_numbers)
        {
            Ipv4InterfaceAddress jaddr = this->addr_map.find(j)->second;
            epi->AddNetworkRouteTo(jaddr.GetLocal(),jaddr.GetMask(),interface);
            if(addr_cnt < queuenumber)
            {
                epi->SetDestAddr(jaddr.GetLocal());
                addr_cnt++;
            }
        }
    }
    //init ocs
    this->ocsrouting->ClearDeviceMatch();
    auto interval = this->set_index+1;
    auto devices_number = this->OCS->GetNDevices()-1;
    uint32_t match_count = 0;
    for(unsigned int i=0;i<devices_number;i++){
        bool inserted = this->ocsrouting->AddDeviceMatch(this->OCS->GetDevice(i),this->OCS->GetDevice((i+interval)%devices_number));
        if(inserted)
            match_count++;
    }
    if(match_count > 0)
        this->set_index = (this->set_index+1)%this->set_numbers;
    this->SetRoutings();
}

void
SingleRouteSchedule::SetRoutings()
{
    //    std::cout<<"Reset Route at "<<Simulator::Now().GetSeconds()<<std::endl;
    if(Simulator::Now() >= this->finishtime)
        return ;
    //    this->ocsrouting->ClearDeviceMatch();
    this->ocsrouting->SetNight(false);
    //restart of devices on TOR
    auto devices_number = this->OCS->GetNDevices()-1;
    for(unsigned int i=0;i<devices_number;i++){
        Ptr<MultiChannelPointToPointDevice> dvi = this->devices_map.find(i)->second;
        dvi->BeginWork();
    }

    //schedule configure event
    Simulator::Schedule(
        this->time_intervalNs,
        &SingleRouteSchedule::SetNight,
        this
    );
    //    this->DoBufferRecord();
    return ;
}

bool
SingleRouteSchedule::SetNight()
{
    this->ocsrouting->ClearDeviceMatch();
    this->ocsrouting->SetNight(true);
    auto interval = this->set_index+1;
    //why minus 1? avoid the 0.0.0.0 device
    auto devices_number = this->OCS->GetNDevices()-1;
    uint32_t match_count = 0;


    for(unsigned int i=0;i<devices_number;i++){
        bool inserted = this->ocsrouting->AddDeviceMatch(this->OCS->GetDevice(i),this->OCS->GetDevice((i+interval)%devices_number));
        if(inserted)
            match_count++;
        //config tor devices, from i to j
        //this function is an error, address i should not be the current one, need to be modified
        Ptr<Ipv4SingleEPSRouting> epsrti = this->eps_routing_map.find(i)->second;
        Ptr<MultiChannelPointToPointDevice> dvi = this->devices_map.find(i)->second;
        //last addr in queue
        uint32_t last_addr_idx;
        uint32_t queue_number = 1;
        if(interval+queue_number-1 >=devices_number)
            last_addr_idx = i+interval+queue_number;
        else
            last_addr_idx = i+interval+queue_number-1;
        Ipv4Address addressi = this->addr_map.find((last_addr_idx)%devices_number)->second.GetLocal();
        epsrti->BeginConfig(addressi);
        dvi->IntoNight();
    }
    if(match_count > 0)
        this->set_index = (this->set_index+1)%this->set_numbers;

    Simulator::Schedule(
        this->ReConfigurableNs,
        &SingleRouteSchedule::SetRoutings,
        this
    );
    this->DoBufferRecord();
    return !this->ocsrouting->DeviceMapEmpty();
}

void
SingleRouteSchedule::DoBufferRecord()
{
    for(auto md : this->devices_map)
    {
        md.second->RecordQueueBuffer(this->bufferoutfilestream);
    }
}

}