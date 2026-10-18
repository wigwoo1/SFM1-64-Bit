#pragma once
#include "vvd_geometry.h"
#include "jiggle_parameters_candidate.h"
#include <cmath>
#include <string>
using GeometryMatrixCandidate=std::array<float,12>;
struct GeometryBoneCandidate {
 std::string name; int32_t parent=-1,procedure=0;
 bool hasJiggleParameters=false;JiggleParametersCandidate jiggleParameters;
 GeometryMatrixCandidate local{},inverseBind{},global{},skin{};
};
inline GeometryMatrixCandidate geometryComposeCandidate(const GeometryMatrixCandidate&a,const GeometryMatrixCandidate&b){
 GeometryMatrixCandidate out{};
 for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c){float v=c==3?a[r*4+3]:0;for(size_t k=0;k<3;++k)v+=a[r*4+k]*b[k*4+c];out[r*4+c]=v;}
 return out;
}
inline std::vector<GeometryMatrixCandidate> evaluateGeometryPosePaletteCandidate(const std::vector<GeometryBoneCandidate>&bones,const std::vector<GeometryMatrixCandidate>&locals,std::vector<GeometryMatrixCandidate>*outGlobals=nullptr,bool evaluateProcedural=true){
 if(bones.size()!=locals.size())throw std::runtime_error("pose bone count");
 for(size_t n=0;n<bones.size();++n){auto&b=bones[n];if(b.parent< -1||(b.parent>=0&&size_t(b.parent)>=bones.size())||b.parent==int64_t(n))throw std::runtime_error("pose parent");if(b.procedure&&evaluateProcedural)throw std::runtime_error("procedural pose unsupported");for(float v:locals[n])if(!std::isfinite(v))throw std::runtime_error("nonfinite local pose");for(float v:b.inverseBind)if(!std::isfinite(v))throw std::runtime_error("nonfinite inverse bind");}
 std::vector<GeometryMatrixCandidate>globals(bones.size()),palette(bones.size());std::vector<bool>done(bones.size());size_t remaining=bones.size();
 while(remaining){size_t before=remaining;for(size_t n=0;n<bones.size();++n){auto parent=bones[n].parent;if(done[n]||(parent>=0&&!done[size_t(parent)]))continue;globals[n]=parent<0?locals[n]:geometryComposeCandidate(globals[size_t(parent)],locals[n]);palette[n]=geometryComposeCandidate(globals[n],bones[n].inverseBind);for(float v:palette[n])if(!std::isfinite(v))throw std::runtime_error("pose overflow");done[n]=true;--remaining;}if(before==remaining)throw std::runtime_error("pose cycle");}
 if(outGlobals)*outGlobals=std::move(globals);return palette;
}
inline std::vector<GeometryBoneCandidate> readGeometrySkeletonCandidate(const std::vector<uint8_t>&bytes){
 auto range=[&](uint64_t o,uint64_t n){if(o>bytes.size()||n>bytes.size()-o)throw std::runtime_error("MDL skeleton span");};
 auto integer=[&](size_t o){range(o,4);int32_t v;std::memcpy(&v,bytes.data()+o,4);return v;};
 auto scalar=[&](size_t o){range(o,4);float v;std::memcpy(&v,bytes.data()+o,4);if(!std::isfinite(v))throw std::runtime_error("nonfinite bone");return v;};
 range(0,164);if(integer(0)!=0x54534449||integer(4)!=49)throw std::runtime_error("MDL49 skeleton required");
 auto count=integer(156),start=integer(160);if(count<0||start<0)throw std::runtime_error("MDL bone count/offset");range(uint64_t(start),uint64_t(count)*216);
 std::vector<GeometryBoneCandidate>bones(static_cast<size_t>(count));
 for(size_t n=0;n<bones.size();++n){size_t base=size_t(start)+n*216;auto&b=bones[n];int64_t name=int64_t(base)+integer(base);if(name<0)throw std::runtime_error("bone name");range(uint64_t(name),1);size_t end=size_t(name);while(end<bytes.size()&&bytes[end])++end;if(end==bytes.size())throw std::runtime_error("bone name termination");b.name.assign(reinterpret_cast<const char*>(bytes.data()+size_t(name)),end-size_t(name));b.parent=integer(base+4);if(b.parent< -1||b.parent>=count||b.parent==int32_t(n))throw std::runtime_error("bone parent");
  float x=scalar(base+44),y=scalar(base+48),z=scalar(base+52),w=scalar(base+56);if(std::abs(x*x+y*y+z*z+w*w-1)>1e-3f)throw std::runtime_error("bone quaternion");
  b.local={1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w,scalar(base+32),2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w,scalar(base+36),2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y,scalar(base+40)};
  for(size_t i=0;i<12;++i)b.inverseBind[i]=scalar(base+96+i*4);b.procedure=integer(base+164);if(b.procedure==5){auto relative=integer(base+168);if(!relative)throw std::runtime_error("jiggle parameter offset missing");int64_t offset=int64_t(base)+relative;if(offset<0)throw std::runtime_error("jiggle parameter offset");b.jiggleParameters=readJiggleParametersCandidate(bytes,size_t(offset));b.hasJiggleParameters=true;}
 }
 std::vector<bool>done(bones.size());size_t remaining=bones.size();while(remaining){size_t before=remaining;for(size_t n=0;n<bones.size();++n){auto&b=bones[n];if(done[n]||(b.parent>=0&&!done[size_t(b.parent)]))continue;b.global=b.parent<0?b.local:geometryComposeCandidate(bones[size_t(b.parent)].global,b.local);b.skin=geometryComposeCandidate(b.global,b.inverseBind);done[n]=true;--remaining;}if(before==remaining)throw std::runtime_error("bone cycle");}
 return bones;
}
inline std::array<float,3> skinGeometryPositionCandidate(const std::array<uint8_t,48>&vertex,const std::vector<GeometryMatrixCandidate>&palette){
 auto scalar=[&](size_t o){float v;std::memcpy(&v,vertex.data()+o,4);if(!std::isfinite(v))throw std::runtime_error("nonfinite skin input");return v;};
 auto influences=vertex[15];if(!influences||influences>3)throw std::runtime_error("skin influence count");std::array<float,3>p{scalar(16),scalar(20),scalar(24)},out{};
 for(size_t n=0;n<influences;++n){auto bone=vertex[12+n];if(bone>=palette.size())throw std::runtime_error("skin bone ID");float weight=scalar(n*4);if(weight<0||weight>1)throw std::runtime_error("skin weight");auto&m=palette[bone];for(float v:m)if(!std::isfinite(v))throw std::runtime_error("nonfinite skin palette");for(size_t r=0;r<3;++r){float v=m[r*4+3];for(size_t k=0;k<3;++k)v+=m[r*4+k]*p[k];out[r]+=weight*v;}}
 for(float v:out)if(!std::isfinite(v))throw std::runtime_error("skin overflow");return out;
}
// Rigid-bone linear blend candidate: no translation or output normalization.
// Nonuniform scale requires a separately verified normal-transform policy.
inline std::array<float,3> skinGeometryDirectionCandidate(const std::array<uint8_t,48>&vertex,const std::array<float,3>&direction,const std::vector<GeometryMatrixCandidate>&palette){
 for(float v:direction)if(!std::isfinite(v))throw std::runtime_error("nonfinite skin direction");
 auto influences=vertex[15];if(!influences||influences>3)throw std::runtime_error("skin influence count");std::array<float,3>out{};
 for(size_t n=0;n<influences;++n){auto bone=vertex[12+n];if(bone>=palette.size())throw std::runtime_error("skin bone ID");float weight;std::memcpy(&weight,vertex.data()+n*4,4);if(!std::isfinite(weight)||weight<0||weight>1)throw std::runtime_error("skin weight");auto&m=palette[bone];for(float v:m)if(!std::isfinite(v))throw std::runtime_error("nonfinite skin palette");for(size_t r=0;r<3;++r){float v=0;for(size_t k=0;k<3;++k)v+=m[r*4+k]*direction[k];out[r]+=weight*v;}}
 for(float v:out)if(!std::isfinite(v))throw std::runtime_error("skin direction overflow");return out;
}
struct GeometryDeformedVertexCandidate {std::array<float,3>position{},normal{};std::array<float,2>uv{};std::array<float,4>tangent{};bool hasTangent=false;};
inline GeometryDeformedVertexCandidate skinGeometryVertexCandidate(const std::array<uint8_t,48>&vertex,const std::array<uint8_t,16>*tangent,const std::vector<GeometryMatrixCandidate>&palette){
 GeometryDeformedVertexCandidate out;out.position=skinGeometryPositionCandidate(vertex,palette);
 // Position validation has already checked every used bone, weight and matrix.
 // Immutable inputs let normal/tangent reuse those checks, preserving arithmetic.
 auto checkedDirection=[&](const std::array<float,3>&direction){
  for(float v:direction)if(!std::isfinite(v))throw std::runtime_error("nonfinite skin direction");
  std::array<float,3>result{};
  for(size_t n=0;n<vertex[15];++n){float weight;std::memcpy(&weight,vertex.data()+n*4,4);const auto&m=palette[vertex[12+n]];for(size_t r=0;r<3;++r){float v=0;for(size_t k=0;k<3;++k)v+=m[r*4+k]*direction[k];result[r]+=weight*v;}}
  for(float v:result)if(!std::isfinite(v))throw std::runtime_error("skin direction overflow");return result;
 };
 std::array<float,3>normal;std::memcpy(normal.data(),vertex.data()+28,12);out.normal=checkedDirection(normal);std::memcpy(out.uv.data(),vertex.data()+40,8);for(float v:out.uv)if(!std::isfinite(v))throw std::runtime_error("nonfinite UV");
 if(tangent){std::array<float,4>input;std::memcpy(input.data(),tangent->data(),16);for(float v:input)if(!std::isfinite(v))throw std::runtime_error("nonfinite tangent");auto direction=checkedDirection({input[0],input[1],input[2]});out.tangent={direction[0],direction[1],direction[2],input[3]};out.hasTangent=true;}return out;
}
