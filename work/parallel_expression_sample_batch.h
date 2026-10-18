#pragma once
#include "session_expression_runtime.h"
#include <thread>
#include <exception>
struct ExpressionSampleResultCandidate {
 SessionExpressionSample sample;
 std::exception_ptr error;
};
inline ExpressionSampleResultCandidate sampleExpressionJobCandidate(const Graph&graph,uint32_t id){
 ExpressionSampleResultCandidate result;
 try{result.sample=sampleSessionExpression(graph,id);}catch(...){result.error=std::current_exception();}
 return result;
}
// Graph and job storage remain immutable until all workers join. The scheduler
// must establish current inputs before sampling, then handle results/errors in
// its original serial order. This routine does not commit any session writes.
inline std::vector<ExpressionSampleResultCandidate> sampleExpressionBatchCandidate(const Graph&graph,const std::vector<uint32_t>&jobs,size_t workers){
 if(!workers||workers>64)throw std::runtime_error("expression worker count must be 1..64");
 std::vector<ExpressionSampleResultCandidate> results(jobs.size());
 if(jobs.empty())return results;
 workers=std::min(workers,jobs.size());
 if(workers==1){for(size_t i=0;i<jobs.size();++i)results[i]=sampleExpressionJobCandidate(graph,jobs[i]);return results;}
 std::vector<std::thread> threads;threads.reserve(workers);
 try{for(size_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{for(size_t i=worker;i<jobs.size();i+=workers)results[i]=sampleExpressionJobCandidate(graph,jobs[i]);});}
 catch(...){for(auto&thread:threads)thread.join();throw;}
 for(auto&thread:threads)thread.join();return results;
}
