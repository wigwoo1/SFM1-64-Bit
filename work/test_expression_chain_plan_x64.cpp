#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_plan.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;auto graph=loadGraph(argv[1]);auto roots=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1);auto discovered=discoverSessionExpressionChainsCandidate(graph,roots.at(0));auto plan=planSessionExpressionChainsCandidate(graph,discovered.chains);
 if(plan.schedule.cycle)throw std::runtime_error("combined chain dependency cycle");std::map<uint32_t,size_t>positions;for(size_t i=0;i<plan.schedule.order.size();++i)if(!positions.emplace(plan.schedule.order[i],i).second)throw std::runtime_error("duplicate scheduled operation");if(positions.size()!=plan.operations.size())throw std::runtime_error("incomplete schedule");
 auto duplicated=discovered.chains;duplicated.insert(duplicated.end(),discovered.chains.begin(),discovered.chains.end());auto deduplicated=planSessionExpressionChainsCandidate(graph,duplicated);if(deduplicated.schedule.order!=plan.schedule.order||deduplicated.operations.size()!=plan.operations.size())throw std::runtime_error("duplicate chain changed plan");
 auto nodes=attributeDependencies(plan.operations);size_t edges=0;for(size_t i=0;i<nodes.size();++i)for(const auto&group:nodes[i].edgeGroups)for(auto consumer:group){if(positions.at(nodes[i].operation)>=positions.at(nodes.at(consumer).operation))throw std::runtime_error("dependency order violation");++edges;}
 std::cout<<"chains="<<discovered.chains.size()<<" unique_operations="<<plan.operations.size()<<" dependency_edges="<<edges<<" conflicting_attributes="<<plan.conflictingWriters.size()<<" cycle=0 dependency_order=passed\n";
 for(const auto&item:plan.conflictingWriters)std::cout<<"conflict_element="<<item.first.first<<" attribute="<<item.first.second<<" writers="<<item.second.size()<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
