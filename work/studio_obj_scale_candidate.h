#pragma once
#include "scene_gpu_vertex_candidate.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
inline std::vector<SceneGpuVertexCandidate> studioScaleObjVerticesCandidate(const std::vector<SceneGpuVertexCandidate>& source,float factor){
 if(!std::isfinite(factor)||factor<=0)throw std::runtime_error("OBJ scale must be finite and positive");if(source.empty())throw std::runtime_error("OBJ scale requires geometry");
 std::array<double,3> low{},high{};for(unsigned k=0;k<3;++k)low[k]=high[k]=source.front().position[k];for(const auto& v:source){if(!sceneGpuVertexFiniteCandidate(v))throw std::runtime_error("OBJ scale input nonfinite");for(unsigned k=0;k<3;++k){low[k]=std::min(low[k],double(v.position[k]));high[k]=std::max(high[k],double(v.position[k]));}}
 auto result=source;if(factor==1)return result;for(auto& v:result)for(unsigned k=0;k<3;++k){double center=(low[k]+high[k])*.5;double value=center+(double(v.position[k])-center)*factor;v.position[k]=static_cast<float>(value);if(!std::isfinite(v.position[k]))throw std::runtime_error("OBJ scale position overflow");}if(result.size()%3)throw std::runtime_error("OBJ scale triangle coverage");for(size_t i=0;i<result.size();i+=3){std::array<double,3> a{},b{};for(unsigned k=0;k<3;++k){a[k]=double(result[i+1].position[k])-result[i].position[k];b[k]=double(result[i+2].position[k])-result[i].position[k];}if(a[1]*b[2]-a[2]*b[1]==0&&a[2]*b[0]-a[0]*b[2]==0&&a[0]*b[1]-a[1]*b[0]==0)throw std::runtime_error("OBJ scale collapses triangle");}return result;
}

