#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "pack_color_dependencies.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;auto graph=loadGraph(argv[1]);auto base=graph;size_t operators=0,checks=0;const char*names[]={"red","green","blue","alpha"};
 for(uint32_t id=0;id<graph.elements.size();++id){if(graph.strings.at(graph.elements[id].type)!="DmePackColorOperator")continue;++operators;auto operation=packColorOperationCandidate(graph,id);if(operation.inputs.size()!=4||operation.outputs!=std::vector<AttributeIdentity>{{id,"color"}})throw std::runtime_error("pack dependency schema");auto saved=graph.elements[id];
  for(unsigned scenario=0;scenario<8;++scenario){std::array<float,4>values{};for(unsigned i=0;i<4;++i){values[i]=scenario==0?wireFloat(runtimeAttribute(graph,id,names[i])->values[0]):scenario==1?0.0f:scenario==2?1.0f:scenario==3?-1.0f:scenario==4?2.0f:float((scenario*31+i*17)%256)/256.0f;setFloat(*runtimeAttribute(graph,id,names[i]),0,values[i]);}
   auto expected=recoveredPackColorCandidate(values);auto*output=runtimeAttribute(graph,id,"color");setWireU32(output->values[0],expected^0xffffffffu);SessionAttributeChanges journal;
   if(!executeSessionPackColorCandidate(graph,id,&journal)||wireInt(output->values[0])!=int32_t(expected)||journal.attributes!=std::vector<std::pair<uint32_t,std::string>>{{id,"color"}})throw std::runtime_error("pack fixture mismatch");
   SessionAttributeChanges repeat;executeSessionPackColorCandidate(graph,id,&repeat);if(!repeat.attributes.empty())throw std::runtime_error("equal pack emitted write");++checks;
  }
  auto*alpha=runtimeAttribute(graph,id,"alpha");alpha->type=2;auto before=graph;bool rejected=false;try{executeSessionPackColorCandidate(graph,id);}catch(const std::runtime_error&){rejected=true;}if(!rejected||!graphExactEqualCandidate(before,graph))throw std::runtime_error("invalid pack schema mutated");graph.elements[id]=saved;
 }
 if(!operators||!graphExactEqualCandidate(base,graph))throw std::runtime_error("pack restoration mismatch");std::cout<<"pack_operators="<<operators<<" checks="<<checks<<" equal_invalid_schema_full_graph_restore=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
