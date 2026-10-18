#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_plan.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;auto graph=loadGraph(argv[1]);auto roots=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1);auto paths=channelTimePaths(graph,roots.at(0));auto discovery=discoverSessionExpressionChainsCandidate(graph,roots.at(0));auto plan=planSessionExpressionChainsCandidate(graph,discovery.chains);std::set<uint32_t>covered;for(const auto&operation:plan.operations)covered.insert(operation.id);
 auto additional=discoverSessionAdditionalPlayPathsCandidate(graph,roots.at(0),discovery.chains);for(const auto&item:additional)plan.operations.push_back(channelOperation(item.first,ChannelBinding(graph,item.first)));
 std::map<AttributeIdentity,std::vector<uint32_t>>writers;for(const auto&operation:plan.operations)for(const auto&output:operation.outputs)writers[output].push_back(operation.id);size_t conflicts=0;for(const auto&item:writers)if(item.second.size()>1){++conflicts;std::cout<<"conflict_element="<<item.first.first<<" attribute="<<item.first.second<<" writers=";for(auto id:item.second)std::cout<<id<<",";std::cout<<'\n';}
 auto nodes=attributeDependencies(plan.operations);std::vector<uint32_t>scheduleRoots;for(uint32_t i=0;i<nodes.size();++i)scheduleRoots.push_back(i);auto schedule=traverseSchedule(nodes,scheduleRoots);std::cout<<"additional_channels="<<additional.size()<<" operations="<<plan.operations.size()<<" writer_conflicts="<<conflicts<<" cycle="<<schedule.cycle<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
