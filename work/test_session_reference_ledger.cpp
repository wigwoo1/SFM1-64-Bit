#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_reference_ledger.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;auto g=loadGraph(argv[1]);SessionRuntimeHandles e(g);SessionAttributeHandles a(g,e);SessionReferenceLedger ledger(g,e,a);size_t checks=0;
 for(uint32_t i=0;i<g.elements.size();++i)for(const auto&attribute:g.elements[i].attributes)if(attribute.type==1||attribute.type==15){
  std::map<uint32_t,size_t> expected;for(const auto&v:attribute.values)if(v.reference<g.elements.size())++expected[e.graphHandles.at(v.reference)];auto handle=a.find(e.graphHandles[i],g.strings.at(attribute.name));
  for(auto item:expected){if(ledger.occurrences(item.first,handle)!=item.second)throw std::runtime_error("incoming count mismatch");++checks;}
 }
 bool removed=false;size_t releaseChecks=0;
 for(uint32_t i=0;i<g.elements.size()&&!removed;++i)for(const auto&attribute:g.elements[i].attributes)if(attribute.type==15&&attribute.values.size()>1){auto name=g.strings.at(attribute.name);auto handle=a.find(e.graphHandles[i],name);auto saved=attribute;
  if(!a.remove(g,e,e.graphHandles[i],name,[&](const Value&v,uint32_t live){if(v.reference>=g.elements.size())return;auto target=e.graphHandles.at(v.reference);auto before=ledger.occurrences(target,live);if(!a.resolve(g,e,live)||!before||!ledger.unlink(target,live)||ledger.occurrences(target,live)!=before-1)throw std::runtime_error("unlink lifecycle mismatch");++releaseChecks;}))throw std::runtime_error("remove failed");
  for(const auto&v:saved.values)if(v.reference<g.elements.size()&&ledger.occurrences(e.graphHandles[v.reference],handle))throw std::runtime_error("removed references retained");removed=true;break;
 }
 if(!removed)throw std::runtime_error("no removal fixture");
 ledger.incoming[0xfffffffe]={4,7,4};if(!ledger.unlink(0xfffffffe,4)||ledger.incoming.at(0xfffffffe)!=std::vector<uint32_t>{7,4}||!ledger.unlink(0xfffffffe,4)||ledger.incoming.at(0xfffffffe)!=std::vector<uint32_t>{7}||ledger.unlink(0xfffffffe,4))throw std::runtime_error("duplicate unlink mismatch");
 std::cout<<"incoming_count_checks="<<checks<<" release_checks="<<releaseChecks<<" duplicate_checks=3 external_unresolved="<<ledger.externalUnresolved<<'\n';return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
