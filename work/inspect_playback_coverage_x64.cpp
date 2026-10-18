#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_plan.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;auto graph=loadGraph(argv[1]);auto roots=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1);auto paths=channelTimePaths(graph,roots.at(0));auto discovery=discoverSessionExpressionChainsCandidate(graph,roots.at(0));auto plan=planSessionExpressionChainsCandidate(graph,discovery.chains);std::set<uint32_t>covered;for(const auto&operation:plan.operations)covered.insert(operation.id);
 std::map<std::string,size_t>groups;size_t omitted=0;for(const auto&entry:paths){if(covered.count(entry.first))continue;++omitted;try{ChannelBinding b(graph,entry.first);std::string owner=b.destination.present()?graph.strings.at(graph.elements.at(b.destination.element).type):"missing";std::string key="mode="+std::to_string(b.mode)+" destination_type="+std::to_string(b.destination.scalarType)+" owner="+owner+" time_paths="+std::to_string(entry.second.size());++groups[key];}catch(const std::exception&e){++groups[std::string("invalid=")+e.what()];}}
 std::cout<<"active_clip="<<roots.at(0)<<" hierarchy_channels="<<paths.size()<<" covered_channels="<<paths.size()-omitted<<" omitted_channels="<<omitted<<'\n';for(const auto&group:groups)std::cout<<"count="<<group.second<<' '<<group.first<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
