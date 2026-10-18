#pragma once
#include "private_expression_chain_sample.h"
#include <thread>
#include <exception>
#include "stable_graph_sampling_worker_pool.h"
struct PrivateExpressionChainJobCandidate {ChannelBinding play;uint32_t expression;int32_t tick;bool hasSampledInput=false;float sampledInput=0;};
struct PrivateExpressionChainResultCandidate {PrivateExpressionChainSampleCandidate sample;std::exception_ptr error;};
inline PrivateExpressionChainResultCandidate samplePrivateExpressionChainJobCandidate(const Graph&graph,const PrivateExpressionChainJobCandidate&job){
 PrivateExpressionChainResultCandidate result;
 try{result.sample=job.hasSampledInput?evaluatePrivateExpressionChainInputCandidate(graph,job.play,job.expression,job.sampledInput):samplePrivateExpressionChainCandidate(graph,job.play,job.expression,job.tick);}catch(...){result.error=std::current_exception();}
 return result;
}
using PrivateExpressionChainWorkerPoolCandidate=StableGraphSamplingWorkerPoolCandidate<PrivateExpressionChainJobCandidate,PrivateExpressionChainResultCandidate,samplePrivateExpressionChainJobCandidate>;
// Caller freezes graph/jobs until return and proves input stability separately.
// Exceptions retain job order; this batch performs no commits.
inline std::vector<PrivateExpressionChainResultCandidate> samplePrivateExpressionChainsCandidate(const Graph&graph,const std::vector<PrivateExpressionChainJobCandidate>&jobs,size_t workers){
 if(!workers||workers>64)throw std::runtime_error("private chain worker count must be 1..64");
 std::vector<PrivateExpressionChainResultCandidate>results(jobs.size());if(jobs.empty())return results;
 auto sample=[&](size_t i){results[i]=samplePrivateExpressionChainJobCandidate(graph,jobs[i]);};
 workers=std::min(workers,jobs.size());if(workers==1){for(size_t i=0;i<jobs.size();++i)sample(i);return results;}
 std::vector<std::thread>threads;threads.reserve(workers);
 try{for(size_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{for(size_t i=worker;i<jobs.size();i+=workers)sample(i);});}catch(...){for(auto&thread:threads)thread.join();throw;}
 for(auto&thread:threads)thread.join();return results;
}
