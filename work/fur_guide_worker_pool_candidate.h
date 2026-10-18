#pragma once
#include "fur_surface_attachment_candidate.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <exception>
// One batch at a time. Inputs remain frozen until sample returns; destruction
// must not race a sample. Output ranges are disjoint and preserve anchor order.
class FurGuideWorkerPoolCandidate{
 std::mutex mutex_,submission_;std::condition_variable wake_,done_;std::vector<std::thread>threads_;bool stopping_=false;uint64_t generation_=0;size_t finished_=0;
 const std::vector<FurSurfaceAnchorCandidate>*anchors_=nullptr;const std::vector<GeometryDeformedVertexCandidate>*vertices_=nullptr;std::vector<FurSurfaceGuideCandidate>*output_=nullptr;std::vector<std::exception_ptr>*errors_=nullptr;
 void worker(size_t index,size_t count){uint64_t seen=0;for(;;){std::unique_lock<std::mutex>lock(mutex_);wake_.wait(lock,[&]{return stopping_||generation_!=seen;});if(stopping_)return;seen=generation_;auto*a=anchors_;auto*v=vertices_;auto*out=output_;auto*errors=errors_;lock.unlock();size_t batch=a->size()/count,remainder=a->size()%count,first=index*batch+std::min(index,remainder),last=first+batch+(index<remainder?1:0);try{for(size_t i=first;i<last;++i)(*out)[i]=sampleFurSurfaceGuideCandidate((*a)[i],*v);}catch(...){(*errors)[index]=std::current_exception();}lock.lock();if(++finished_==count)done_.notify_one();}}
 void stop(){ {std::lock_guard<std::mutex>lock(mutex_);stopping_=true;}wake_.notify_all();for(auto&t:threads_)if(t.joinable())t.join();}
public:
 explicit FurGuideWorkerPoolCandidate(size_t count){if(!count||count>64)throw std::runtime_error("fur workers must be 1..64");threads_.reserve(count);try{for(size_t i=0;i<count;++i)threads_.emplace_back([this,i,count]{worker(i,count);});}catch(...){stop();throw;}}
 ~FurGuideWorkerPoolCandidate(){stop();}FurGuideWorkerPoolCandidate(const FurGuideWorkerPoolCandidate&)=delete;FurGuideWorkerPoolCandidate&operator=(const FurGuideWorkerPoolCandidate&)=delete;
 std::vector<FurSurfaceGuideCandidate>sample(const std::vector<FurSurfaceAnchorCandidate>&anchors,const std::vector<GeometryDeformedVertexCandidate>&vertices){std::lock_guard<std::mutex>single(submission_);std::vector<FurSurfaceGuideCandidate>output(anchors.size());if(anchors.empty())return output;std::vector<std::exception_ptr>errors(threads_.size());{std::unique_lock<std::mutex>lock(mutex_);anchors_=&anchors;vertices_=&vertices;output_=&output;errors_=&errors;finished_=0;++generation_;wake_.notify_all();done_.wait(lock,[&]{return finished_==threads_.size();});anchors_=nullptr;vertices_=nullptr;output_=nullptr;errors_=nullptr;}for(const auto&error:errors)if(error)std::rethrow_exception(error);return output;}
};
