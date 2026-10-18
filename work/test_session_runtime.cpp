#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_runtime.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;SessionRuntime runtime(loadGraph(argv[1]));auto&g=runtime.graph;size_t removed=0,released=0,editChecks=0;
 for(uint32_t i=0;i<g.elements.size()&&!editChecks;++i)for(const auto&attribute:g.elements[i].attributes)if(attribute.type==15&&attribute.values.size()>1){
  auto owner=runtime.elements.graphHandles[i];auto name=g.strings.at(attribute.name);auto handle=runtime.attributes.find(owner,name);auto saved=attribute.values[0];auto target=runtime.elements.graphHandles.at(0);
  if(saved.reference==0xfffffffe)continue;
  runtime.setReference(owner,name,0,0xffffffff);
  if(!runtime.setReference(owner,name,0,target)||runtime.references.incoming.at(target).front()!=handle||runtime.setReference(owner,name,0,target))throw std::runtime_error("reference insertion mismatch");++editChecks;
  if(!runtime.setReference(owner,name,0,0xffffffff)||attribute.values[0].reference!=0xffffffff)throw std::runtime_error("reference clear mismatch");++editChecks;
  if(runtime.setReference(owner,name,0,0xfffffffe))throw std::runtime_error("invalid target accepted");++editChecks;
  if(saved.reference!=0xffffffff)runtime.setReference(owner,name,0,runtime.elements.graphHandles.at(saved.reference));break;
 }
 if(editChecks!=3)throw std::runtime_error("reference edit fixture missing");
 for(uint32_t i=0;i<g.elements.size();++i){auto owner=runtime.elements.graphHandles[i];std::vector<Attribute> selected;
  for(const auto&a:g.elements[i].attributes)if(a.type==1||a.type==15)selected.push_back(a);
  for(const auto&a:selected){auto name=g.strings.at(a.name);auto handle=runtime.attributes.find(owner,name);
   if(!runtime.removeAttribute(owner,name)||runtime.removeAttribute(owner,name)||runtime.attributes.resolve(g,runtime.elements,handle))throw std::runtime_error("owned removal lifecycle mismatch");
   for(const auto&v:a.values)if(v.reference<g.elements.size()){if(runtime.references.occurrences(runtime.elements.graphHandles[v.reference],handle))throw std::runtime_error("owned removal retained reference");++released;}++removed;
  }
 }
 if(!runtime.references.incoming.empty()||runtime.references.externalUnresolved||runtime.removeAttribute(0xffffffff,"missing"))throw std::runtime_error("ledger not empty after removal");
 std::cout<<"removed_reference_attributes="<<removed<<" released_occurrences="<<released<<" reference_edit_checks="<<editChecks<<" final_incoming_targets=0\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
