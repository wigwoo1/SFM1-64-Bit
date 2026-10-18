#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_runtime_handles.h"
int main(int argc,char**argv){try{if(argc!=2)return 1;auto g=loadGraph(argv[1]);SessionRuntimeHandles runtime(g);unsigned refs=0,external=0;
 for(uint32_t i=0;i<g.elements.size();++i){if(runtime.resolve(g,runtime.graphHandles[i])!=&g.elements[i])throw std::runtime_error("element binding mismatch");
  for(auto&a:g.elements[i].attributes)if(a.type==1||a.type==15)for(auto&v:a.values){if(v.reference==0xfffffffe){++external;continue;}auto resolved=runtime.resolve(g,runtime.fromReference(v));if(resolved!=(v.reference==0xffffffff?nullptr:&g.elements[v.reference]))throw std::runtime_error("reference mismatch");++refs;}}
 auto originalCount=g.elements.size();auto saved=runtime.graphHandles.at(0);auto clone=g.elements.at(0);g.elements.push_back(clone);auto added=runtime.bind(g,uint32_t(originalCount));
 if(runtime.resolve(g,saved)!=&g.elements[0]||runtime.resolve(g,added)!=&g.elements.back())throw std::runtime_error("graph growth invalidated handles");
 if(!runtime.retire(added)||runtime.retire(added)||runtime.resolve(g,added))throw std::runtime_error("retirement mismatch");
 auto replacement=runtime.bind(g,uint32_t(originalCount));if(replacement==added||runtime.resolve(g,added)||runtime.resolve(g,replacement)!=&g.elements.back())throw std::runtime_error("stale handle alias");
 std::cout<<"elements="<<originalCount<<" reference_checks="<<refs<<" external_unresolved="<<external<<" growth_and_retirement_checks=3\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
