#pragma once
#include <thread>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <mutex>
#include <condition_variable>
// Single batch at a time. Caller freezes inputs through sample(); destruction
// must not race API calls. Worker exceptions remain per-job deferred results.
template<class Job,class Result,Result(*Sample)(const Graph&,const Job&)>
class StableGraphSamplingWorkerPoolCandidate {
 std::mutex mutex,submission;std::condition_variable wake,done;
 std::vector<std::thread>threads;bool stopping=false;uint64_t generation=0;size_t finished=0;
 const Graph*graph=nullptr;const std::vector<Job>*jobs=nullptr;
 std::vector<Result>*results=nullptr;
 void worker(size_t index,size_t count){uint64_t seen=0;for(;;){std::unique_lock<std::mutex>lock(mutex);wake.wait(lock,[&]{return stopping||generation!=seen;});if(stopping)return;seen=generation;auto*g=graph;auto*j=jobs;auto*r=results;lock.unlock();for(size_t i=index;i<j->size();i+=count)(*r)[i]=Sample(*g,(*j)[i]);lock.lock();if(++finished==count)done.notify_one();}}
 void stop(){ {std::lock_guard<std::mutex>lock(mutex);stopping=true;}wake.notify_all();for(auto&thread:threads)if(thread.joinable())thread.join();}
public:
 explicit StableGraphSamplingWorkerPoolCandidate(size_t count){if(!count||count>64)throw std::runtime_error("sampling worker count must be 1..64");threads.reserve(count);try{for(size_t i=0;i<count;++i)threads.emplace_back([this,i,count]{worker(i,count);});}catch(...){stop();throw;}}
 ~StableGraphSamplingWorkerPoolCandidate(){stop();}
 StableGraphSamplingWorkerPoolCandidate(const StableGraphSamplingWorkerPoolCandidate&)=delete;
 StableGraphSamplingWorkerPoolCandidate&operator=(const StableGraphSamplingWorkerPoolCandidate&)=delete;
 size_t workerCount()const{return threads.size();}
 std::vector<Result>sample(const Graph&value,const std::vector<Job>&batch){
  std::lock_guard<std::mutex>single(submission);std::vector<Result>output(batch.size());if(batch.empty())return output;
  std::unique_lock<std::mutex>lock(mutex);graph=&value;jobs=&batch;results=&output;finished=0;++generation;wake.notify_all();done.wait(lock,[&]{return finished==threads.size();});graph=nullptr;jobs=nullptr;results=nullptr;return output;
 }
};
