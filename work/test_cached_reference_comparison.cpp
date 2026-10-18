#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_expression_chain_pass.h"
static ChannelEndpoint legacyEndpoint(const Graph&g,const Element&e,const char*side){
 ChannelEndpoint b;std::string prefix=side;auto refs=hierarchyRefs(g,e,(prefix+"Element").c_str(),1);auto*a=namedAttribute(g,e,(prefix+"Attribute").c_str());auto*i=namedAttribute(g,e,(prefix+"Index").c_str());
 if(!a||a->type!=5||a->values.size()!=1||!i||i->type!=2||i->values.size()!=1)throw std::runtime_error("invalid endpoint metadata");b.attribute=a->values[0].text;b.index=wireInt(i->values[0]);if(refs.empty())return b;
 b.element=refs.at(0);auto*target=namedAttribute(g,g.elements.at(b.element),b.attribute.c_str());if(!target)throw std::runtime_error("missing endpoint attribute");bool array=target->type>=15;b.scalarType=array?target->type-14:target->type;if(b.index<0||(!array&&b.index!=0)||static_cast<size_t>(b.index)>=target->values.size())throw std::runtime_error("endpoint index out of range");return b;
}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("fixture required");auto graph=loadGraph(argv[1]);size_t singles=0,arrays=0,endpoints=0;
 for(const auto&e:graph.elements)if(graph.strings.at(e.type)=="DmeChannel")for(const char*side:{"from","to"}){if(!sameRuntimeEndpoint(legacyEndpoint(graph,e,side),resolveEndpoint(graph,e,side)))throw std::runtime_error("endpoint reader mismatch");++endpoints;}
 for(const auto&e:graph.elements)for(const auto&a:e.attributes){const auto&name=graph.strings.at(a.name);
  if(a.type==1){auto refs=hierarchyRefs(graph,e,name.c_str(),1);if(cachedSingleHierarchyReference(graph,e,name.c_str())!=(refs.empty()?0xffffffff:refs.at(0)))throw std::runtime_error("scalar reference mismatch");++singles;}
  if(a.type==15){auto refs=hierarchyRefs(graph,e,name.c_str(),15);if(!cachedHierarchyReferencesEqual(graph,e,name.c_str(),refs))throw std::runtime_error("array reference mismatch");auto mismatch=refs;mismatch.push_back(0xffffffff);if(cachedHierarchyReferencesEqual(graph,e,name.c_str(),mismatch))throw std::runtime_error("array length mismatch accepted");++arrays;}
 }
 auto root=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1).at(0);auto discovery=discoverSessionExpressionChainsCandidate(graph,root);auto id=discovery.chains.at(0).play;auto&e=graph.elements.at(id);auto*scalar=runtimeAttribute(graph,id,"fromElement");const auto saved=*scalar;
 unsigned malformed=0;
 auto check=[&]{bool oldFailed=false,newFailed=false;try{hierarchyRefs(graph,e,"fromElement",1);}catch(const std::runtime_error&){oldFailed=true;}try{cachedSingleHierarchyReference(graph,e,"fromElement");}catch(const std::runtime_error&){newFailed=true;}if(!oldFailed||!newFailed)throw std::runtime_error("malformed scalar validation diverged");++malformed;};
 scalar->type=2;check();*scalar=saved;scalar->values.clear();check();*scalar=saved;scalar->values[0].reference=uint32_t(graph.elements.size());check();*scalar=saved;
 auto log=ChannelBinding(graph,id).log;auto*layers=runtimeAttribute(graph,log,"layers");auto savedLayers=*layers;layers->values.at(0).reference=uint32_t(graph.elements.size());
 bool oldFailed=false,newFailed=false;try{hierarchyRefs(graph,graph.elements.at(log),"layers",15);}catch(const std::runtime_error&){oldFailed=true;}try{cachedHierarchyReferencesEqual(graph,graph.elements.at(log),"layers",{});}catch(const std::runtime_error&){newFailed=true;}if(!oldFailed||!newFailed)throw std::runtime_error("mismatching malformed array validation diverged");*layers=savedLayers;++malformed;
 std::cout<<"scalar_fields="<<singles<<" array_fields="<<arrays<<" endpoints="<<endpoints<<" malformed_checks="<<malformed<<" scope=allocation_free_reference_comparison\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
