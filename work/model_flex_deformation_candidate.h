#pragma once
#include "mdl_mesh_flex_candidate.h"
#include "flex_delta_quantization_candidate.h"
#include "flex_ramp_candidate.h"
#include "flex_vertex_weight_candidate.h"
#include "vvd_geometry.h"
#include <functional>
#include <chrono>
struct ModelFlexWorkProfileCandidate {double copy=0,processing=0;};
// Model-space candidate: caller supplies both descriptor arrays and selection.
// Resets caller-owned output from baseline before accumulating. Output may be
// partial on failure; do not publish it. A later call resets it before reuse.
inline void deformModelFlexesIntoCandidate(const VvdGeometryCandidate&baseline,const std::vector<MdlMeshFlexCandidate>&flexes,const std::vector<float>&current,const std::vector<float>&previous,float scale,const std::function<bool(uint32_t,uint32_t)>&selected,VvdGeometryCandidate&result,ModelFlexWorkProfileCandidate*profile=nullptr){
 if(&baseline==&result)throw std::runtime_error("flex output aliases immutable baseline");
 if(!selected||current.size()!=previous.size()||!std::isfinite(scale)||scale<=0)throw std::runtime_error("invalid model flex inputs");for(float v:current)if(!std::isfinite(v))throw std::runtime_error("nonfinite current flex weight");for(float v:previous)if(!std::isfinite(v))throw std::runtime_error("nonfinite previous flex weight");
 using Clock=std::chrono::steady_clock;auto start=profile?Clock::now():Clock::time_point{};result=baseline;auto copied=profile?Clock::now():Clock::time_point{};
 for(const auto&flex:flexes){if(!selected(flex.body,flex.model))continue;if(flex.descriptor>=current.size()||flex.pair>=current.size())throw std::runtime_error("model flex descriptor outside weights");float primary=flexRampCandidate(flex.targets,current[flex.descriptor]),oldPrimary=flexRampCandidate(flex.targets,previous[flex.descriptor]);float paired=flex.pair?flexRampCandidate(flex.targets,current[flex.pair]):primary,oldPaired=flex.pair?flexRampCandidate(flex.targets,previous[flex.pair]):oldPrimary;
  for(const auto&delta:flex.vertices){uint64_t index=uint64_t(flex.firstVertex)+delta.index;if(index>=result.vertices.size())throw std::runtime_error("model flex vertex outside geometry");float weight=flexVertexWeightCandidate(primary,oldPrimary,paired,oldPaired,delta.speed,delta.side);if(weight==0)continue;auto&vertex=result.vertices[size_t(index)];
   for(size_t i=0;i<6;++i){size_t offset=i<3?16+i*4:28+(i-3)*4;float base;std::memcpy(&base,vertex.data()+offset,4);float decoded=i<3?delta.positionDelta[i]:delta.normalDelta[i-3];float change=float(quantizeFlexDeltaCandidate(decoded,scale))*scale;float value=base+change*weight;if(!std::isfinite(value))throw std::runtime_error("model flex accumulation overflow");std::memcpy(vertex.data()+offset,&value,4);}
  }
 }if(profile){profile->copy=std::chrono::duration<double,std::milli>(copied-start).count();profile->processing=std::chrono::duration<double,std::milli>(Clock::now()-copied).count();}
}

// Value-returning compatibility entry point. The Into form reuses caller storage.
inline VvdGeometryCandidate deformModelFlexesCandidate(const VvdGeometryCandidate&baseline,const std::vector<MdlMeshFlexCandidate>&flexes,const std::vector<float>&current,const std::vector<float>&previous,float scale,const std::function<bool(uint32_t,uint32_t)>&selected,ModelFlexWorkProfileCandidate*profile=nullptr){VvdGeometryCandidate result;deformModelFlexesIntoCandidate(baseline,flexes,current,previous,scale,selected,result,profile);return result;}
