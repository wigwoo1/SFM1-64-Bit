#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "reference_serial_float_play.h"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("fixture required");auto baseline=loadGraph(argv[1]);auto actual=baseline,reference=baseline;SessionAttributeChanges a,b;size_t operations=0,samples=0;
 for(uint32_t id=0;id<baseline.elements.size();++id){if(baseline.strings.at(baseline.elements[id].type)!="DmeChannel")continue;ChannelBinding binding(baseline,id);if(!binding.destination.present()||binding.destination.scalarType!=3||binding.log==0xffffffff||binding.layers.size()>1)continue;binding.mode=3;
 for(int32_t tick:{-10000,0,10000,10000,-10000}){float value=0;if(sampleSessionFloatPlayCandidate(baseline,binding,tick,value))++samples;auto x=executeSessionFloatPlay(actual,binding,tick,&a),y=executeReferenceSessionFloatPlay(reference,binding,tick,&b);if(x!=y)throw std::runtime_error("dispatch mismatch");++operations;}
 }
 if(!samples||!graphExactEqualCandidate(actual,reference)||a.attributes!=b.attributes)throw std::runtime_error("sample/commit differs from prior serial implementation");auto untouched=loadGraph(argv[1]);if(!graphExactEqualCandidate(baseline,untouched))throw std::runtime_error("sampling mutated graph");std::cout<<"scalar sample/commit operations="<<operations<<" samples="<<samples<<" graph_journal_order_prior_serial_equivalence=passed const_graph_preserved=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
