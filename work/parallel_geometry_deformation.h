#pragma once
#include "model_skinning_geometry.h"
#include <thread>
#include <exception>
// Inputs must remain immutable until return. Results are private until every
// worker has joined; the first failing vertex in index order is rethrown.
inline std::vector<GeometryDeformedVertexCandidate> deformGeometryBatchCandidate(const VvdGeometryCandidate&geometry,const std::vector<GeometryMatrixCandidate>&palette,size_t workers){
 if(workers>64)throw std::runtime_error("geometry worker budget");
 if(!geometry.tangents.empty()&&geometry.tangents.size()!=geometry.vertices.size())throw std::runtime_error("geometry tangent count");
 std::vector<GeometryDeformedVertexCandidate>result(geometry.vertices.size());
 if(workers<=1){for(size_t n=0;n<result.size();++n)result[n]=skinGeometryVertexCandidate(geometry.vertices[n],geometry.tangents.empty()?nullptr:&geometry.tangents[n],palette);return result;}
 std::vector<std::exception_ptr>errors(result.size());std::vector<std::thread>threads;workers=std::min(workers,result.size());threads.reserve(workers);
 auto run=[&](size_t worker){for(size_t n=worker;n<result.size();n+=workers){try{result[n]=skinGeometryVertexCandidate(geometry.vertices[n],geometry.tangents.empty()?nullptr:&geometry.tangents[n],palette);}catch(...){errors[n]=std::current_exception();}}};
 try{for(size_t w=0;w<workers;++w)threads.emplace_back(run,w);}catch(...){for(auto&t:threads)t.join();throw;}
 for(auto&t:threads)t.join();for(auto&e:errors)if(e)std::rethrow_exception(e);return result;
}
