#pragma once
#include "session_expression_runtime.h"
#include <thread>
#include <exception>
struct ScalarSampleJobCandidate {ChannelBinding binding;int32_t tick;};
struct ScalarSampleResultCandidate {bool success=false;uint32_t bits=0;std::exception_ptr error;};
inline ScalarSampleResultCandidate sampleScalarJobCandidate(const Graph&graph,const ScalarSampleJobCandidate&job){
 ScalarSampleResultCandidate result;try{float value=0;result.success=sampleSessionFloatPlayCandidate(graph,job.binding,job.tick,value);if(result.success)std::memcpy(&result.bits,&value,4);}catch(...){result.error=std::current_exception();}return result;
}
// Caller freezes graph/job storage until return and validates read dependencies.
// Results preserve job order; errors are deferred for scheduler-ordered handling.
inline std::vector<ScalarSampleResultCandidate> sampleScalarBatchCandidate(const Graph&graph,const std::vector<ScalarSampleJobCandidate>&jobs,size_t workers){
 if(!workers||workers>64)throw std::runtime_error("scalar worker count must be 1..64");
 std::vector<ScalarSampleResultCandidate>results(jobs.size());if(jobs.empty())return results;
 workers=std::min(workers,jobs.size());if(workers==1){for(size_t i=0;i<jobs.size();++i)results[i]=sampleScalarJobCandidate(graph,jobs[i]);return results;}
 std::vector<std::thread>threads;threads.reserve(workers);
 try{for(size_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{for(size_t i=worker;i<jobs.size();i+=workers)results[i]=sampleScalarJobCandidate(graph,jobs[i]);});}catch(...){for(auto&thread:threads)thread.join();throw;}
 for(auto&thread:threads)thread.join();return results;
}
