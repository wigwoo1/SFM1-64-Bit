#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reference_owned_dmx_projection.h"
#include "session_fresh_flex_pass.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("fixture and content root required");auto graph=loadGraph(argv[1]);std::vector<uint32_t>operators;for(uint32_t i=0;i<graph.elements.size()&&operators.size()<2;++i)if(graph.strings.at(graph.elements[i].type)=="DmeGlobalFlexControllerOperator")operators.push_back(i);if(operators.size()!=2)throw std::runtime_error("two fixture operators required");auto find=[&](uint32_t op,const std::string&name)->Attribute&{for(auto&a:graph.elements.at(op).attributes)if(graph.strings.at(a.name)==name)return a;throw std::runtime_error("missing operator attribute");};
 auto first=operators[0],second=operators[1];graph.elements[second].name=graph.elements[first].name;find(second,"gameModel").values=find(first,"gameModel").values;
 uint32_t a=0x3e800000,b=0x3f400000;auto&firstWire=find(first,"flexWeight").values.at(0).wire;auto&secondWire=find(second,"flexWeight").values.at(0).wire;if(firstWire.size()!=4||secondWire.size()!=4)throw std::runtime_error("source payload");std::memcpy(firstWire.data(),&a,4);std::memcpy(secondWire.data(),&b,4);
 RecoveredOwnedDmxReferenceProjectionCandidate<Graph>p(graph,[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external input");});bool rejected=false;try{executeSessionFreshFlexPassCandidate(p,argv[2]);}catch(const std::runtime_error&e){rejected=std::string(e.what()).find("conflicting shared flex writers")!=std::string::npos;}
 if(!rejected||!graphExactEqualCandidate(graph,p.graph)||!p.scene.queues.dirty.handles.empty()||!p.scene.queues.cache.handles.empty())throw std::runtime_error("conflict preflight mutated graph or wrong rejection");p.scene.cleanup({},[](auto&,bool){},[](auto&){});if(p.scene.size()||p.scene.references.nodes.allocationCount())throw std::runtime_error("teardown");std::cout<<"session conflicting flex writers: native metadata discovery, pre-mutation rejection, complete graph preservation, teardown passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
