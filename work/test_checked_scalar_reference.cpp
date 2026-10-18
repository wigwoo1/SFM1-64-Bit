#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "coordinated_reference_write.h"
#include "channel_runtime_metadata.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;
 for(size_t i=0;i<r.graph.elements.size();++i)if(r.graph.strings.at(r.graph.elements[i].type)=="DmeChannel"){
 auto h=r.elements.graphHandles[i];bindChannelRuntimeMetadata(r,m,h,4);auto a=r.attributes.find(h,"log");auto attr=r.attributes.resolve(r.graph,r.elements,a);auto old=attr->values[0].reference;if(old>=r.graph.elements.size())continue;
 auto target=r.elements.graphHandles.at(old==0?1:0);auto ledger=r.references.incoming;auto identity=r.attributes.identity(r.graph,r.elements,a);auto flags=m.owners.at(h).attributeFlags.at(identity);unsigned calls=0;
 if(coordinatedScalarReferenceWriteChecked(r,m,h,"log",target,false,[&](uint32_t t){++calls;return t==0xffffffff;})||calls!=1||attr->values[0].reference!=old||ledger!=r.references.incoming||m.owners.at(h).attributeFlags.at(identity)!=flags||!m.queues.marked.empty())throw std::runtime_error("schema rejection mutated state");
 if(!coordinatedScalarReferenceWriteChecked(r,m,h,"log",0xffffffff,false,[&](uint32_t){++calls;return false;})||calls!=1||attr->values[0].reference!=0xffffffff)throw std::runtime_error("null target incorrectly checked");
 if(!coordinatedScalarReferenceWriteChecked(r,m,h,"log",target,false,[&](uint32_t t){++calls;return t==target;})||calls!=2||r.references.occurrences(target,a)!=1)throw std::runtime_error("accepted target failed");
 std::cout<<"checked_scalar_reference_checks=3\n";return 0;
 }throw std::runtime_error("no fixture");
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
