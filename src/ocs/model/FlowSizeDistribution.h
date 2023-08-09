//
// Created by venn on 23-8-6.
//

#ifndef NS3_FLOWSIZEDISTRIBUTION_H
#define NS3_FLOWSIZEDISTRIBUTION_H

#include "ns3/core-module.h"
namespace ns3
{

class FlowSizeDistribution : public Object
{
  private:
    Ptr<UniformRandomVariable> sizerandom;
  public:
    static TypeId GetTypeId();
    FlowSizeDistribution();
    ~FlowSizeDistribution();
    uint32_t GenerateFlowsizeByte();
};

} // namespace ns3

#endif // NS3_FLOWSIZEDISTRIBUTION_H
