#pragma once
#include "session_vector_runtime.h"
#include <thread>
#include <exception>
struct VectorSampleJobCandidate {ChannelBinding binding;int32_t tick;};
struct VectorSampleResultCandidate {bool success=false;std::array<uint32_t,3>bits{};std::exception_ptr error;};
inline VectorSampleResultCandidate sampleVectorJobCandidate(const Graph&graph,const VectorSampleJobCandidate&job){
 VectorSampleResultCandidate result;try{std::array<float,3>value{};result.success=sampleSessionVectorPlayCandidate(graph,job.binding,job.tick,value);if(result.success)std::memcpy(result.bits.data(),value.data(),12);}catch(...){result.error=std::current_exception();}return result;
}
// Caller freezes graph/job storage until return and validates read dependencies.
// Results preserve job order; errors are deferred for scheduler-ordered handling.
inline std::vector<VectorSampleResultCandidate> sampleVectorBatchCandidate(const Graph&graph,const std::vector<VectorSampleJobCandidate>&jobs,size_t workers){
 if(!workers||workers>64)throw std::runtime_error("vector worker count must be 1..64");
 std::vector<VectorSampleResultCandidate>results(jobs.size());if(jobs.empty())return results;
 workers=std::min(workers,jobs.size());if(workers==1){for(size_t i=0;i<jobs.size();++i)results[i]=sampleVectorJobCandidate(graph,jobs[i]);return results;}
 std::vector<std::thread>threads;threads.reserve(workers);
 try{for(size_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{for(size_t i=worker;i<jobs.size();i+=workers)results[i]=sampleVectorJobCandidate(graph,jobs[i]);});}catch(...){for(auto&thread:threads)thread.join();throw;}
 for(auto&thread:threads)thread.join();return results;
}
