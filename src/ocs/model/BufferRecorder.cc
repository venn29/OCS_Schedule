//
// Created by venn on 23-7-2.
//

#include "BufferRecorder.h"
#include "ns3/MultiChannelPointToPoint.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
namespace ns3
{

TypeId
BufferRecorder::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BufferRecorder")
                            .SetParent<Object>()
                            .SetGroupName("Ocs")
                            .AddConstructor<BufferRecorder>();
    return tid;
}

BufferRecorder::BufferRecorder():
      recordInterval(Seconds(1.0)),
      outfilestream(nullptr),
      queuenumber(8)
{

}

BufferRecorder::BufferRecorder(ns3::Time intervaltime, std::ofstream* stream, uint32_t queuenumber):
      recordInterval(intervaltime),
      outfilestream(stream),
      queuenumber(queuenumber)
{}

void
BufferRecorder::AddDevice(Ptr<MultiChannelPointToPointDevice> d)
{
    this->devices.push_back(d);
}

void
BufferRecorder::StartRecord()
{
    Simulator::Schedule(this->recordInterval,&BufferRecorder::StartRecord,this);
    DoRecord();
}

void
BufferRecorder::DoRecord()
{
    for(Ptr<MultiChannelPointToPointDevice> md:this->devices)
    {
        md->RecordQueueBuffer(this->outfilestream);
    }
}

}