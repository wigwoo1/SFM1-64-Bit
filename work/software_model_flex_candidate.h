#pragma once
#include "model_flex_deformation_candidate.h"
#include "software_flex_vertex_weight_candidate.h"
#include "software_flex_normalize_candidate.h"
// Explicit native-array labels and tangent policy. No temporal-history inference.
inline void softwareModelFlexIntoCandidate(const VvdGeometryCandidate&baseline,const std::vector<MdlMeshFlexCandidate>&flexes,const std::vector<float>&a8,const std::vector<float>&ac,float scale,const std::function<bool(uint32_t,uint32_t)>&selected,bool tangents,VvdGeometryCandidate&output){
 if(&baseline==&output||!selected||a8.size()!=ac.size()||!std::isfinite(scale)||scale<=0)throw std::runtime_error("invalid software flex inputs");for(float v:a8)if(!std::isfinite(v))throw std::runtime_error("nonfinite A8 flex");for(float v:ac)if(!std::isfinite(v))throw std::runtime_error("nonfinite AC flex");if(tangents&&baseline.tangents.size()!=baseline.vertices.size())throw std::runtime_error("software flex tangent addressing");
 output=baseline;std::vector<bool>allocated(output.vertices.size());size_t count=0;
 for(const auto&flex:flexes){if(!selected(flex.body,flex.model))continue;if(flex.descriptor>=a8.size()||flex.pair>=a8.size())throw std::runtime_error("software flex descriptor addressing");float p=flexRampCandidate(flex.targets,a8[flex.descriptor]),old=flexRampCandidate(flex.targets,ac[flex.descriptor]),pair=flex.pair?flexRampCandidate(flex.targets,a8[flex.pair]):p,oldPair=flex.pair?flexRampCandidate(flex.targets,ac[flex.pair]):old;
  bool inactive=true;for(float v:{p,old,pair,oldPair})inactive=inactive&&(double(v)>-.001&&double(v)<.001);if(inactive)continue;
  for(const auto&delta:flex.vertices){uint64_t index=uint64_t(flex.firstVertex)+delta.index;if(index>=output.vertices.size())throw std::runtime_error("software flex vertex addressing");size_t i=size_t(index);if(!allocated[i]){if(count==65536)throw std::runtime_error("software flex slot capacity");allocated[i]=true;++count;}float weight=softwareFlexVertexWeightCandidate(p,old,pair,oldPair,delta.speed,delta.side);
   for(size_t component=0;component<6;++component){size_t offset=component<3?16+component*4:28+(component-3)*4;float decoded=component<3?delta.positionDelta[component]:delta.normalDelta[component-3];float change=float(quantizeFlexDeltaCandidate(decoded,scale))*scale;float value;std::memcpy(&value,output.vertices[i].data()+offset,4);value=value+change*weight;if(!std::isfinite(value))throw std::runtime_error("software flex accumulation overflow");std::memcpy(output.vertices[i].data()+offset,&value,4);
    if(tangents&&component>=3){std::memcpy(&value,output.tangents[i].data()+(component-3)*4,4);value=value+change*weight;if(!std::isfinite(value))throw std::runtime_error("software flex tangent overflow");std::memcpy(output.tangents[i].data()+(component-3)*4,&value,4);}
   }
  }
 }
 for(size_t i=0;i<allocated.size();++i)if(allocated[i]){std::array<float,3>normal;std::memcpy(normal.data(),output.vertices[i].data()+28,12);softwareFlexNormalizeCandidate(normal);std::memcpy(output.vertices[i].data()+28,normal.data(),12);if(tangents){std::memcpy(normal.data(),output.tangents[i].data(),12);softwareFlexNormalizeCandidate(normal);std::memcpy(output.tangents[i].data(),normal.data(),12);}}
}
