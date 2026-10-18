#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_float_expression_playback.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8);if(argc!=3)throw std::runtime_error("usage: INPUT CLI_OUTPUT (fixture11 chain)");
 auto graph=loadGraph(argv[1]);auto original=graph;
 constexpr uint32_t p=3740,e=3743,c=4234,root=2237;
 auto paths=channelTimePaths(graph,root);if(paths.at(p).size()!=1)throw std::runtime_error("ambiguous fixture path");
 ChannelBinding play(graph,p),copy(graph,c);SessionAttributeChanges changes;
 const auto tick=channelLocalTime(graph,paths.at(p).at(0),0,false);
 if(!executeSessionFloatPlay(graph,play,tick,&changes)||!executeSessionExpression(graph,e,&changes)||!executeSessionFloatCopy(graph,copy,&changes))throw std::runtime_error("direct evaluation failed");
 const auto actual=loadGraph(argv[2]);if(!graphExactEqualCandidate(graph,actual))throw std::runtime_error("CLI graph differs from direct evaluation");
 // Restore every reported write to prove there are no unreported differences.
 auto restored=actual;for(const auto&item:changes.attributes){auto*destination=runtimeAttribute(restored,item.first,item.second);auto*source=runtimeAttribute(original,item.first,item.second);if(!destination||!source)throw std::runtime_error("missing changed attribute");*destination=*source;}
 if(!graphExactEqualCandidate(original,restored))throw std::runtime_error("unreported CLI graph change");
 std::cout<<"CLI playback full_graph_exact=passed reported_writes="<<changes.attributes.size()<<" other_payload_preservation=passed\n";
 }catch(const std::exception&ex){std::cerr<<ex.what()<<'\n';return 1;}}
