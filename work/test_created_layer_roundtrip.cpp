#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "create_runtime_float_layer.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");
 if(argc!=3)throw std::runtime_error("usage: input output");
 SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;
 auto incoming=r.references.incoming;auto external=r.references.externalUnresolved;
 std::array<char,16> id{};id.fill(char(0x27));auto index=r.graph.elements.size();
 createRuntimeFloatLayer(r,m,id,4);
 if(incoming!=r.references.incoming||external!=r.references.externalUnresolved)throw std::runtime_error("creation changed reference ledger");
 saveGraph(r.graph,argv[2]);SessionRuntime copy(loadGraph(argv[2]));
 auto&g=r.graph;auto&h=copy.graph;size_t checks=0;
 if(g.header!=h.header||g.strings!=h.strings||g.elements.size()!=h.elements.size())throw std::runtime_error("graph metadata mismatch");
 for(size_t i=0;i<g.elements.size();++i){const auto&a=g.elements[i];const auto&b=h.elements[i];
  if(a.type!=b.type||a.name!=b.name||a.id!=b.id||a.attributes.size()!=b.attributes.size())throw std::runtime_error("element mismatch");
  for(size_t j=0;j<a.attributes.size();++j){const auto&x=a.attributes[j];const auto&y=b.attributes[j];
   if(x.name!=y.name||x.type!=y.type||x.values.size()!=y.values.size())throw std::runtime_error("attribute mismatch");
   for(size_t k=0;k<x.values.size();++k){const auto&v=x.values[k];const auto&w=y.values[k];
    if(v.wire!=w.wire||v.text!=w.text||v.reference!=w.reference||v.stringIndex!=w.stringIndex)throw std::runtime_error("value mismatch");++checks;
   }
  }
 }
 auto handle=copy.elements.graphHandles.at(index);auto element=copy.elements.resolve(copy.graph,handle);
 if(!element||element->id!=id||copy.graph.strings.at(element->type)!="DmeFloatLogLayer")throw std::runtime_error("reloaded layer missing");
 for(const char*name:{"times","curvetypes","values","compressed","name"}){
  auto attribute=copy.attributes.find(handle,name);
  if(!copy.attributes.resolve(copy.graph,copy.elements,attribute)||!copy.attributes.identity(copy.graph,copy.elements,attribute))throw std::runtime_error("rebuilt attribute handle invalid");
 }
 if(incoming!=copy.references.incoming||external!=copy.references.externalUnresolved)throw std::runtime_error("reloaded reference ledger mismatch");
 std::cout<<"elements="<<h.elements.size()<<" value_checks="<<checks<<" rebuilt_layer_attributes=5 ledger_equal=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
