#pragma once
#include "session_expression_chain_discovery.h"
struct SessionExpressionChainPlanCandidate {
 std::vector<AttributeOperation>operations;ScheduleResult schedule;
 std::map<AttributeIdentity,std::vector<uint32_t>>conflictingWriters;
};
inline SessionExpressionChainPlanCandidate planSessionExpressionChainsCandidate(const Graph&graph,const std::vector<SessionExpressionChainCandidate>&chains){
 SessionExpressionChainPlanCandidate plan;std::map<uint32_t,AttributeOperation>unique;
 for(const auto&chain:chains){unique.emplace(chain.play,channelOperation(chain.play,ChannelBinding(graph,chain.play)));unique.emplace(chain.expression,expressionOperation(graph,chain.expression));unique.emplace(chain.copy,channelOperation(chain.copy,ChannelBinding(graph,chain.copy)));}
 std::map<AttributeIdentity,std::vector<uint32_t>>writers;
 for(auto&item:unique){for(const auto&output:item.second.outputs)writers[output].push_back(item.first);plan.operations.push_back(std::move(item.second));}
 for(auto&item:writers)if(item.second.size()>1)plan.conflictingWriters.emplace(item.first,std::move(item.second));
 auto nodes=attributeDependencies(plan.operations);std::vector<uint32_t>roots;for(uint32_t i=0;i<nodes.size();++i)roots.push_back(i);plan.schedule=traverseSchedule(nodes,roots);return plan;
}
