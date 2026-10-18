#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_expression_runtime.h"
#include "graph_exact_equal.h"
#include "parallel_vector_sample_batch.h"
#include "vector_sampling_worker_pool.h"
#include <exception>
#include <iostream>
using SampleJob=VectorSampleJobCandidate;using SampleResult=VectorSampleResultCandidate;
static std::string errorText(const std::exception_ptr&error){if(!error)return {};try{std::rethrow_exception(error);}catch(const std::exception&e){return e.what();}catch(...){return "nonstandard exception";}}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("fixture required");const auto graph=loadGraph(argv[1]);std::vector<SampleJob>jobs;
 for(uint32_t id=0;id<graph.elements.size();++id){if(graph.strings.at(graph.elements[id].type)!="DmeChannel")continue;ChannelBinding binding(graph,id);if(!binding.destination.present()||binding.destination.scalarType!=10||binding.log==0xffffffff||binding.layers.size()>1)continue;binding.mode=3;for(int32_t tick:{-10000,0,10000,10000,-10000})jobs.push_back({binding,tick});}
 if(jobs.empty())throw std::runtime_error("no jobs");auto rejected=jobs.front();rejected.binding.mode=0;jobs.push_back(rejected);auto invalid=jobs.front();invalid.binding.log=0xfffffffeu;jobs.push_back(invalid);std::vector<SampleResult>expected;for(const auto&job:jobs)expected.push_back(sampleVectorJobCandidate(graph,job));if(expected[expected.size()-2].success||expected[expected.size()-2].error||!expected.back().error)throw std::runtime_error("error fixtures not exercised");
 if(!sampleVectorBatchCandidate(graph,{},8).empty())throw std::runtime_error("empty batch");for(size_t invalidWorkers:{0u,65u}){bool failed=false;try{sampleVectorBatchCandidate(graph,{},invalidWorkers);}catch(const std::runtime_error&){failed=true;}if(!failed)throw std::runtime_error("invalid worker count accepted");}
 for(size_t workers:{1u,2u,4u,8u}){VectorSamplingWorkerPoolCandidate pool(workers);if(pool.workerCount()!=workers||!pool.sample(graph,{}).empty())throw std::runtime_error("pool empty/count");auto actual=pool.sample(graph,jobs);for(unsigned repeat=0;repeat<3;++repeat){auto repeated=pool.sample(graph,jobs);for(size_t i=0;i<jobs.size();++i)if(repeated[i].success!=expected[i].success||repeated[i].bits!=expected[i].bits||bool(repeated[i].error)!=bool(expected[i].error)||errorText(repeated[i].error)!=errorText(expected[i].error))throw std::runtime_error("repeated pool mismatch");}for(size_t i=0;i<jobs.size();++i)if(actual[i].success!=expected[i].success||actual[i].bits!=expected[i].bits||bool(actual[i].error)!=bool(expected[i].error)||errorText(actual[i].error)!=errorText(expected[i].error))throw std::runtime_error("worker/serial result mismatch");std::cout<<"vector_workers="<<workers<<" jobs="<<jobs.size()<<" payload_dispatch_exception_equivalence=passed repeated_pool_batches=4\n";}
 auto reload=loadGraph(argv[1]);if(!graphExactEqualCandidate(graph,reload))throw std::runtime_error("workers mutated input");std::cout<<"stable_const_graph_preserved=passed live_scheduler=not_integrated ordered_commit=not_exercised\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
