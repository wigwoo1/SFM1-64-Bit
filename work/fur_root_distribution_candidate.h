#pragma once
#include "fur_surface_attachment_candidate.h"
inline std::vector<GeometryDeformedVertexCandidate>furRestSurfaceCandidate(const VvdGeometryCandidate&geometry){std::vector<GeometryDeformedVertexCandidate>surface(geometry.vertices.size());for(size_t i=0;i<surface.size();++i){std::memcpy(surface[i].position.data(),geometry.vertices[i].data()+16,12);std::memcpy(surface[i].normal.data(),geometry.vertices[i].data()+28,12);std::memcpy(surface[i].uv.data(),geometry.vertices[i].data()+40,8);}return surface;}
// Generate once on the chosen rest surface. Animation samples fixed anchors.
inline std::vector<FurSurfaceAnchorCandidate> distributeFurRootsCandidate(const std::vector<GeometryDeformedVertexCandidate>&surface,const std::vector<std::array<uint32_t,3>>&triangles,size_t count,float length,uint64_t seed){
 if(!std::isfinite(length)||length<0)throw std::runtime_error("invalid fur distribution length");
 std::vector<double>areas;std::vector<size_t>selected;double total=0;
 for(size_t i=0;i<triangles.size();++i){std::array<std::array<double,3>,3>points{};for(size_t c=0;c<3;++c){if(triangles[i][c]>=surface.size())throw std::runtime_error("fur distribution topology index");for(size_t axis=0;axis<3;++axis){float p=surface[triangles[i][c]].position[axis];if(!std::isfinite(p))throw std::runtime_error("nonfinite fur distribution surface");points[c][axis]=p;}}
 std::array<double,3>a{},b{};for(size_t axis=0;axis<3;++axis){a[axis]=points[1][axis]-points[0][axis];b[axis]=points[2][axis]-points[0][axis];}std::array<double,3>cross{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};double area=0.5*std::sqrt(cross[0]*cross[0]+cross[1]*cross[1]+cross[2]*cross[2]);if(!std::isfinite(area))throw std::runtime_error("fur triangle area overflow");if(area==0)continue;total+=area;if(!std::isfinite(total))throw std::runtime_error("fur surface area overflow");areas.push_back(total);selected.push_back(i);
 }
 if(count&&areas.empty())throw std::runtime_error("fur distribution surface has no area");std::vector<FurSurfaceAnchorCandidate>result;result.reserve(count);
 auto random=[&](){uint64_t z=(seed+=UINT64_C(0x9e3779b97f4a7c15));z=(z^(z>>30))*UINT64_C(0xbf58476d1ce4e5b9);z=(z^(z>>27))*UINT64_C(0x94d049bb133111eb);z^=z>>31;return double(z>>11)*0x1p-53;};
 for(size_t i=0;i<count;++i){double position=random()*total;auto it=std::upper_bound(areas.begin(),areas.end(),position);size_t triangle=it==areas.end()?selected.back():selected[size_t(it-areas.begin())];double radius=std::sqrt(random()),v=random();float first=float(1-radius),second=float(radius*(1-v));float third=float(radius*v);result.push_back({triangles[triangle],{first,second,third},length});}
 return result;
}
