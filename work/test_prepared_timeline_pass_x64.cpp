#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_expression_chain_pass.h"
#include "graph_exact_equal.h"
#ifdef TEST_BORROWED_TIMELINE
#include "reference_owned_dmx_projection.h"
#endif
#include <iostream>
#ifdef TEST_PARALLEL_EXPRESSIONS
#define PREPARED_EXPRESSION_WORKERS 4
#else
#define PREPARED_EXPRESSION_WORKERS 0
#endif
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("fixture required");auto base=loadGraph(argv[1]);auto roots=hierarchyRefs(base,base.elements.at(0),"activeClip",1);auto discovery=discoverSessionExpressionChainsCandidate(base,roots.at(0));auto typed=discoverSessionAdditionalPlayPathsCandidate(base,roots.at(0),discovery.chains);auto colors=discoverSessionColorCopyChannelsCandidate(base,roots.at(0));std::set<uint32_t>packSet;for(auto id:colors){ChannelBinding b(base,id);packSet.insert(b.source.element);}std::vector<uint32_t>packs(packSet.begin(),packSet.end());
#ifdef TEST_BORROWED_TIMELINE
 RecoveredOwnedDmxReferenceProjectionCandidate<Graph> projection(base,[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external borrowed graph");});
 SessionRuntime retained(projection.graph,BorrowedSessionGraphCandidate{}),reference(base);
 if(retained.ownedGraph||&retained.graph!=&projection.graph)throw std::runtime_error("borrowed graph copied");
#else
 SessionRuntime retained(base),reference(base);
#endif
 PreparedSessionExpressionChainPassCandidate prepared(retained,discovery.chains,typed,colors,packs);auto elementHandles=retained.elements.graphHandles;auto attributeCount=retained.attributes.table.liveCount;double freshMilliseconds=0,preparedMilliseconds=0;unsigned frames=0;
 auto step=[&](int32_t tick,size_t expressionWorkers=PREPARED_EXPRESSION_WORKERS){auto start=std::chrono::steady_clock::now();auto actual=prepared.step(tick,nullptr,PREPARED_EXPRESSION_WORKERS,0,expressionWorkers);preparedMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();start=std::chrono::steady_clock::now();auto expected=executeSessionExpressionChainPassCandidate(reference,discovery.chains,tick,typed,colors,packs);freshMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(actual.executed!=expected.executed||actual.changes.attributes!=expected.changes.attributes||!graphExactEqualCandidate(retained.graph,reference.graph))throw std::runtime_error("retained frame/journal/order mismatch");if(retained.elements.graphHandles!=elementHandles||retained.attributes.table.liveCount!=attributeCount)throw std::runtime_error("handles rebuilt");++frames;};
 for(int32_t tick:{-10000,0,10000,10000,-10000})step(tick);
 if(PREPARED_EXPRESSION_WORKERS)for(size_t workers:{1u,0u,16u,4u})step(0,workers);
 ChannelBinding first(base,discovery.chains.at(0).play);for(auto*r:{&retained,&reference}){auto*a=runtimeAttribute(r->graph,first.log,"defaultvalue");if(!a||a->type!=3)throw std::runtime_error("fixture default missing");setFloat(*a,0,0.25f);}step(0);
 if(base.elements.size()>25923){ChannelBinding conflict(retained.graph,18239);auto*a=runtimeAttribute(retained.graph,conflict.log,"defaultvalue");setFloat(*a,0,1.0f);auto before=retained.graph;bool rejected=false;try{prepared.step(0);}catch(const std::runtime_error&e){rejected=std::string(e.what()).find("writer equivalence changed")!=std::string::npos;}if(!rejected||!graphExactEqualCandidate(before,retained.graph))throw std::runtime_error("stale equivalence guard");}
 if(PREPARED_EXPRESSION_WORKERS&&!(prepared.expressionSampledJobs()+prepared.privateChainJobs()))throw std::runtime_error("parallel expression path not exercised");
 std::cout<<"sampled_expressions="<<prepared.expressionSampledJobs()<<" private_chain_jobs="<<prepared.privateChainJobs()<<" max_expression_batch="<<prepared.maxExpressionBatch()<<"\n";
 auto before=retained.graph;prepared.invalidate();bool rejected=false;try{prepared.step(0);}catch(const std::runtime_error&){rejected=true;}if(!rejected||!graphExactEqualCandidate(before,retained.graph))throw std::runtime_error("invalidated plan mutated graph");

#ifdef TEST_BORROWED_TIMELINE
 auto protectedGraph=projection.graph;unsigned rejectedEdits=0;
 try{retained.setReference(retained.elements.graphHandles.at(0),"activeClip",0,0xffffffffu);}catch(const std::runtime_error&){++rejectedEdits;}
 try{retained.setReferenceRange(retained.elements.graphHandles.at(0),"activeClip",0,{0xffffffffu},false,[](int,uint32_t,size_t,size_t){});}catch(const std::runtime_error&){++rejectedEdits;}
 try{retained.removeAttribute(retained.elements.graphHandles.at(0),"activeClip");}catch(const std::runtime_error&){++rejectedEdits;}
 if(rejectedEdits!=3||!graphExactEqualCandidate(protectedGraph,projection.graph))throw std::runtime_error("borrowed owner mutation bypass");
 auto exported=projection.exportReferences([](uint32_t,const auto&)->std::string{throw std::runtime_error("external export");});if(!graphExactEqualCandidate(exported,projection.graph))throw std::runtime_error("borrowed owned export");projection.scene.cleanup({},[](auto&,bool){},[](auto&){});if(projection.scene.size()||projection.scene.references.nodes.allocationCount())throw std::runtime_error("borrowed owned teardown");
 std::cout<<"borrowed_graph_identity_owner_edit_guards_export_teardown=passed\n";
#endif
 std::cout<<"prepared timeline frames="<<frames<<" full_graph_journal_order_handles_value_edit_invalidation=passed preparation_ms="<<prepared.preparationMilliseconds()<<" retained_mean_ms="<<preparedMilliseconds/frames<<" rebuild_pass_mean_ms="<<freshMilliseconds/frames<<" runtime_startup_excluded=1 expression_workers="<<PREPARED_EXPRESSION_WORKERS<<"\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
