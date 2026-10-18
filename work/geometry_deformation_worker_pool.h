#pragma once
#include "parallel_geometry_deformation.h"
#include <mutex>
#include <condition_variable>
#include "render_camera_candidate.h"
// One batch at a time; immutable inputs remain alive until deform() returns.
// Contiguous balanced ranges avoid interleaved output writes across workers.
class GeometryDeformationWorkerPoolCandidate {
 std::mutex mutex,submission;std::condition_variable wake,done;
 std::vector<std::thread>threads;bool stopping=false;uint64_t generation=0;size_t finished=0;
 const VvdGeometryCandidate*geometry=nullptr;const std::vector<GeometryMatrixCandidate>*palette=nullptr;
 const RenderCameraCandidate*placement=nullptr;
 std::vector<GeometryDeformedVertexCandidate>*output=nullptr;std::vector<std::exception_ptr>*errors=nullptr;
 void worker(size_t index,size_t count){uint64_t seen=0;for(;;){std::unique_lock<std::mutex>lock(mutex);wake.wait(lock,[&]{return stopping||seen!=generation;});if(stopping)return;seen=generation;auto*g=geometry;auto*p=palette;auto*r=output;auto*e=errors;auto*transform=placement;lock.unlock();const size_t batch=r->size()/count,remainder=r->size()%count;const size_t first=index*batch+std::min(index,remainder),last=first+batch+(index<remainder?1:0);for(size_t n=first;n<last;++n){try{auto vertex=skinGeometryVertexCandidate(g->vertices[n],g->tangents.empty()?nullptr:&g->tangents[n],*p);if(transform){auto position=transform->clip(vertex.position);vertex.position={position[0],position[1],position[2]};auto normal=vertex.normal;for(size_t row=0;row<3;++row)vertex.normal[row]=transform->rows[row*4]*normal[0]+transform->rows[row*4+1]*normal[1]+transform->rows[row*4+2]*normal[2];if(vertex.hasTangent){auto tangent=vertex.tangent;for(size_t row=0;row<3;++row)vertex.tangent[row]=transform->rows[row*4]*tangent[0]+transform->rows[row*4+1]*tangent[1]+transform->rows[row*4+2]*tangent[2];}}(*r)[n]=vertex;}catch(...){(*e)[index]=std::current_exception();break;}}lock.lock();if(++finished==count)done.notify_one();}}
 void stop(){ {std::lock_guard<std::mutex>lock(mutex);stopping=true;}wake.notify_all();for(auto&t:threads)if(t.joinable())t.join();}
public:
 explicit GeometryDeformationWorkerPoolCandidate(size_t count){if(!count||count>64)throw std::runtime_error("geometry worker count must be 1..64");threads.reserve(count);try{for(size_t n=0;n<count;++n)threads.emplace_back([this,n,count]{worker(n,count);});}catch(...){stop();throw;}}
 ~GeometryDeformationWorkerPoolCandidate(){stop();}
 GeometryDeformationWorkerPoolCandidate(const GeometryDeformationWorkerPoolCandidate&)=delete;
 GeometryDeformationWorkerPoolCandidate&operator=(const GeometryDeformationWorkerPoolCandidate&)=delete;
 size_t workerCount()const{return threads.size();}
 std::vector<GeometryDeformedVertexCandidate>deform(const VvdGeometryCandidate&value,const std::vector<GeometryMatrixCandidate>&matrices,const RenderCameraCandidate*transform=nullptr){
  if(transform&&(transform->rows[12]!=0||transform->rows[13]!=0||transform->rows[14]!=0||transform->rows[15]!=1))throw std::runtime_error("scene placement affine row");
  std::lock_guard<std::mutex>single(submission);if(!value.tangents.empty()&&value.tangents.size()!=value.vertices.size())throw std::runtime_error("geometry tangent count");
  // Each contiguous range stores its first error. Scanning worker order still
  // reports the lowest failing vertex, without one exception slot per vertex.
  std::vector<GeometryDeformedVertexCandidate>result(value.vertices.size());if(result.empty())return result;std::vector<std::exception_ptr>failures(threads.size());
  {std::unique_lock<std::mutex>lock(mutex);geometry=&value;palette=&matrices;placement=transform;output=&result;errors=&failures;finished=0;++generation;wake.notify_all();done.wait(lock,[&]{return finished==threads.size();});geometry=nullptr;palette=nullptr;placement=nullptr;output=nullptr;errors=nullptr;}
  for(auto&e:failures)if(e)std::rethrow_exception(e);return result;
 }
};
