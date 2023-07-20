//
// Created by venn on 23-7-2.
//

#ifndef NS3_BUFFERRECORDER_H
#define NS3_BUFFERRECORDER_H

#include "ns3/nstime.h"
#include "ns3/MultiChannelPointToPoint.h"
#include "ns3/output-stream-wrapper.h"
#include <fstream>
#include <iostream>

namespace ns3
{
class BufferRecorder : public Object
{
  public:
    static TypeId GetTypeId();
    BufferRecorder();
    ~BufferRecorder();
    BufferRecorder(Time intervaltime, std::ofstream* stream, uint32_t queuenumber);
    void StartRecord();
    void DoRecord();
    void AddDevice(Ptr<MultiChannelPointToPointDevice> d);
  private:
    Time recordInterval;
    std::ofstream* outfilestream;
    uint32_t queuenumber;
    std::list<Ptr<MultiChannelPointToPointDevice>> devices;
};//class bufferrecorder
}//namespace ns3

#endif // NS3_BUFFERRECORDER_H
