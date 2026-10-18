#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "schedule_traversal.h"
#include "channel_dependencies.h"
#include "expression_dependencies.h"
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("usage: inspect_expression_dependencies session.dmx");auto graph=loadGraph(argv[1]);auto roots=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1);ClipHierarchy hierarchy(graph,roots.at(0));
 std::vector<AttributeOperation> operations;for(auto id:hierarchy.channels)operations.push_back(channelOperation(id,ChannelBinding(graph,id)));size_t channels=operations.size();
 for(uint32_t id=0;id<graph.elements.size();++id)if(graph.strings[graph.elements[id].type]=="DmeExpressionOperator")operations.push_back(expressionOperation(graph,id));
 auto nodes=attributeDependencies(operations);uint64_t edges=0,expressionEdges=0;
 for(size_t i=0;i<nodes.size();++i)for(const auto&group:nodes[i].edgeGroups)for(auto next:group){++edges;if(i>=channels||next>=channels)++expressionEdges;}
 std::vector<uint32_t> all;for(uint32_t i=0;i<nodes.size();++i)all.push_back(i);auto schedule=traverseSchedule(nodes,all);
 if(schedule.order.size()!=nodes.size())throw std::runtime_error("missing schedule operation");
 if(!schedule.cycle){std::map<uint32_t,size_t> position;for(size_t i=0;i<schedule.order.size();++i)position[schedule.order[i]]=i;for(const auto&node:nodes)for(const auto&group:node.edgeGroups)for(auto next:group)if(position[node.operation]>=position[nodes[next].operation])throw std::runtime_error("dependency ordering failure");}
 std::cout<<"channels="<<channels<<" expressions="<<operations.size()-channels<<" edges="<<edges<<" expression_edges="<<expressionEdges<<" cycle="<<schedule.cycle<<" ordered="<<schedule.order.size()<<" scope=active_channels_all_serialized_expressions\n";
 return 0;}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
