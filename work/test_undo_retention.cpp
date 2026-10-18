#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "runtime_undo_retention.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));RuntimeUndoRetention l(r);auto first=r.elements.graphHandles.at(0),second=r.elements.graphHandles.at(1);l.records[first]={0xc00abcde,0,1};l.records[second]={0x00012345,0,0xffffffff};
 auto base=l.records[first].packed;
 {RuntimeUndoRetainedTarget a(l);a.assign(first);if(l.records[first].packed!=retainUndoReferenceCount(base)||!(l.records[first].flags&1)||l.collectionFlags!=0x10)throw std::runtime_error("first retention mismatch");a.assign(first);if(l.records[first].packed!=retainUndoReferenceCount(base))throw std::runtime_error("unchanged assignment retained twice");
 RuntimeUndoRetainedTarget b(l);b.assign(first);a.assign(second);if(l.records[first].packed!=retainUndoReferenceCount(base)||(l.records[second].packed>>20)!=1)throw std::runtime_error("replacement retention mismatch");RuntimeUndoRetainedTarget moved(std::move(b));if(b.target!=0xffffffff||moved.target!=first)throw std::runtime_error("move ownership failed");}
 if(l.records[first].packed!=base||l.records[second].packed!=0x12345)throw std::runtime_error("destructor release mismatch");
 for(uint32_t i=0;i<1024;++i){auto p=(i<<20)|0xc00abcde;if(releaseUndoReferenceCount(retainUndoReferenceCount(p))!=p||retainUndoReferenceCount(releaseUndoReferenceCount(p))!=p)throw std::runtime_error("packed wrap mismatch");}
 {RuntimeUndoRetainedTarget stale(l);stale.assign(0xfffffffe);if(stale.target!=0xfffffffe)throw std::runtime_error("unresolved target assignment mismatch");}
 std::cout<<"undo_retention_checks=5 packed_counts=1024\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
