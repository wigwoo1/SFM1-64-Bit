#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_runtime.h"
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=4)throw std::runtime_error("usage: test input edited-output second-output");SessionRuntime runtime(loadGraph(argv[1]));auto&g=runtime.graph;unsigned edits=0;
 for(uint8_t type:{uint8_t(1),uint8_t(15)}){bool found=false;
  for(uint32_t i=0;i<g.elements.size()&&!found;++i)for(const auto&a:g.elements[i].attributes)if(a.type==type&&!a.values.empty()&&a.values[0].reference!=0xfffffffe){auto owner=runtime.elements.graphHandles[i];auto name=g.strings.at(a.name);
   uint32_t replacement=a.values[0].reference==0?1:0;
   if(!runtime.setReference(owner,name,0,runtime.elements.graphHandles.at(replacement)))throw std::runtime_error("reference edit rejected");++edits;found=true;break;
  }if(!found)throw std::runtime_error("missing reference fixture");
 }
 saveGraph(g,argv[2]);SessionRuntime reloaded(loadGraph(argv[2]));auto&copy=reloaded.graph;size_t checks=0;
 if(g.header!=copy.header||g.strings!=copy.strings||g.elements.size()!=copy.elements.size())throw std::runtime_error("graph metadata mismatch");
 for(uint32_t i=0;i<g.elements.size();++i){auto&left=g.elements[i];auto&right=copy.elements[i];if(left.type!=right.type||left.name!=right.name||left.id!=right.id||left.attributes.size()!=right.attributes.size())throw std::runtime_error("element mismatch");
  for(size_t j=0;j<left.attributes.size();++j){auto&a=left.attributes[j];auto&b=right.attributes[j];if(a.name!=b.name||a.type!=b.type||a.values.size()!=b.values.size())throw std::runtime_error("attribute mismatch");
   for(size_t k=0;k<a.values.size();++k){auto&v=a.values[k];auto&w=b.values[k];if(v.reference!=w.reference||v.wire!=w.wire||v.text!=w.text||v.stringIndex!=w.stringIndex)throw std::runtime_error("value roundtrip mismatch");++checks;}
  }
 }
 if(runtime.references.incoming!=reloaded.references.incoming||runtime.references.externalUnresolved!=reloaded.references.externalUnresolved){
  // Restore rebuilds lists in traversal order; compare occurrence multisets.
  auto first=runtime.references.incoming,second=reloaded.references.incoming;
  for(auto&entry:first)std::sort(entry.second.begin(),entry.second.end());for(auto&entry:second)std::sort(entry.second.begin(),entry.second.end());
  if(first!=second||runtime.references.externalUnresolved!=reloaded.references.externalUnresolved)throw std::runtime_error("reloaded reference ledger mismatch");
 }
 saveGraph(copy,argv[3]);auto bytes=[](const char*path){std::ifstream f(path,std::ios::binary);return std::vector<char>(std::istreambuf_iterator<char>(f),{});};if(bytes(argv[2])!=bytes(argv[3]))throw std::runtime_error("resave bytes differ");
 std::cout<<"reference_edits="<<edits<<" value_roundtrip_checks="<<checks<<" ledger_multisets_equal=1 resave_bytes_equal=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
