//
// Created by venn on 23-8-6.
//

#include "FlowSizeDistribution.h"

namespace ns3
{
TypeId
FlowSizeDistribution::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FlowSizeDistribution")
                        .SetParent<Object>()
                        .SetGroupName("Ocs")
                        .AddConstructor<FlowSizeDistribution>();
    return tid;
}
FlowSizeDistribution::FlowSizeDistribution()
{
    this->sizerandom = CreateObject<UniformRandomVariable>();
}

FlowSizeDistribution::~FlowSizeDistribution()
{

}

uint32_t
FlowSizeDistribution::GenerateFlowsizeByte()
{
//    double outcome = this->sizerandom->GetValue(0,1);
    //test for plenty small flows
    return 49*1000;
}

} // namespace ns3