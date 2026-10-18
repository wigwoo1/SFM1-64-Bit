#pragma once
#include "studio_obj_model_candidate.h"
#include <algorithm>
struct StudioObjTriangleCandidate {size_t polygon;std::array<StudioObjCornerCandidate,3> corners;};
inline std::vector<StudioObjTriangleCandidate> studioTriangulateObjCandidate(const StudioObjModelCandidate& model){
 std::vector<StudioObjTriangleCandidate> result;
 for(size_t faceIndex=0;faceIndex<model.polygons.size();++faceIndex){const auto& face=model.polygons[faceIndex];const auto count=face.corners.size();if(count<3)throw std::runtime_error("OBJ polygon arity");
  std::vector<std::array<double,3>> p;auto origin=model.positions.at(face.corners[0].position);double scale=0;
  for(const auto& c:face.corners){auto value=model.positions.at(c.position);std::array<double,3> q{};for(unsigned k=0;k<3;++k){if(!std::isfinite(value[k]))throw std::runtime_error("OBJ nonfinite polygon");q[k]=double(value[k])-origin[k];scale=std::max(scale,std::abs(q[k]));}p.push_back(q);}
  if(scale==0)throw std::runtime_error("OBJ collapsed polygon");const double epsilon=scale*scale*1e-12;
  std::array<double,3> normal{};for(size_t i=0;i<count;++i){const auto& a=p[i];const auto& b=p[(i+1)%count];normal[0]+=(a[1]-b[1])*(a[2]+b[2]);normal[1]+=(a[2]-b[2])*(a[0]+b[0]);normal[2]+=(a[0]-b[0])*(a[1]+b[1]);}
  const double length=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);if(length<=epsilon)throw std::runtime_error("OBJ degenerate polygon");
  for(const auto& q:p)if(std::abs(q[0]*normal[0]+q[1]*normal[1]+q[2]*normal[2])/length>scale*1e-6)throw std::runtime_error("OBJ nonplanar polygon");
  unsigned drop=0;for(unsigned k=1;k<3;++k)if(std::abs(normal[k])>std::abs(normal[drop]))drop=k;unsigned u=(drop+1)%3,v=(drop+2)%3;
  auto cross=[&](size_t a,size_t b,size_t c){return (p[b][u]-p[a][u])*(p[c][v]-p[a][v])-(p[b][v]-p[a][v])*(p[c][u]-p[a][u]);};
  auto on=[&](size_t a,size_t b,size_t c){return std::abs(cross(a,b,c))<=epsilon&&p[c][u]>=std::min(p[a][u],p[b][u])-scale*1e-12&&p[c][u]<=std::max(p[a][u],p[b][u])+scale*1e-12&&p[c][v]>=std::min(p[a][v],p[b][v])-scale*1e-12&&p[c][v]<=std::max(p[a][v],p[b][v])+scale*1e-12;};
  for(size_t i=0;i<count;++i){size_t next=(i+1)%count;if(std::hypot(p[next][u]-p[i][u],p[next][v]-p[i][v])<=scale*1e-12)throw std::runtime_error("OBJ repeated polygon corner");
   for(size_t j=i+1;j<count;++j){size_t end=(j+1)%count;if(j==next||end==i)continue;double a=cross(i,next,j),b=cross(i,next,end),c=cross(j,end,i),d=cross(j,end,next);bool crossing=((a>epsilon&&b<-epsilon)||(a<-epsilon&&b>epsilon))&&((c>epsilon&&d<-epsilon)||(c<-epsilon&&d>epsilon));if(crossing||on(i,next,j)||on(i,next,end)||on(j,end,i)||on(j,end,next))throw std::runtime_error("OBJ self-intersecting polygon");}
  }
  double area=0;for(size_t i=0;i<count;++i){auto j=(i+1)%count;area+=p[i][u]*p[j][v]-p[j][u]*p[i][v];}if(std::abs(area)<=epsilon)throw std::runtime_error("OBJ zero polygon area");double direction=area>0?1:-1;
  std::vector<size_t> ring;for(size_t i=0;i<count;++i)ring.push_back(i);
  while(ring.size()>3){bool found=false;for(size_t i=0;i<ring.size();++i){auto a=ring[(i+ring.size()-1)%ring.size()],b=ring[i],c=ring[(i+1)%ring.size()];if(direction*cross(a,b,c)<=epsilon)continue;bool occupied=false;
    for(auto other:ring)if(other!=a&&other!=b&&other!=c&&direction*cross(a,b,other)>=-epsilon&&direction*cross(b,c,other)>=-epsilon&&direction*cross(c,a,other)>=-epsilon){occupied=true;break;}
    if(occupied)continue;result.push_back({faceIndex,{face.corners[a],face.corners[b],face.corners[c]}});ring.erase(ring.begin()+i);found=true;break;
   }if(!found)throw std::runtime_error("OBJ polygon cannot be triangulated");
  }
  if(direction*cross(ring[0],ring[1],ring[2])<=epsilon)throw std::runtime_error("OBJ degenerate final triangle");result.push_back({faceIndex,{face.corners[ring[0]],face.corners[ring[1]],face.corners[ring[2]]}});
 }return result;
}
