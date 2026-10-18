#pragma once
#include "studio_obj_triangulation_candidate.h"
#include "scene_gpu_vertex_candidate.h"
#include <map>
struct StudioObjMeshRangeCandidate{uint32_t firstIndex=0,indexCount=0,material=0;std::string object,group;};
struct StudioObjRenderMeshCandidate{
 std::vector<SceneGpuVertexCandidate> vertices;std::vector<uint32_t> indices;
 std::vector<bool> hasTexture;std::vector<size_t> trianglePolygons;
 std::vector<std::string> materials;std::vector<StudioObjMeshRangeCandidate> ranges;
};
inline StudioObjRenderMeshCandidate studioPrepareObjRenderMeshCandidate(const StudioObjModelCandidate& model){
 auto triangles=studioTriangulateObjCandidate(model);if(triangles.size()>UINT32_MAX/3)throw std::runtime_error("OBJ render mesh too large");
 using Normal=std::array<double,3>;std::vector<Normal> faceNormals(model.polygons.size());
 for(const auto& t:triangles){auto a=model.positions.at(t.corners[0].position),b=model.positions.at(t.corners[1].position),c=model.positions.at(t.corners[2].position);Normal x{},y{};for(unsigned k=0;k<3;++k){x[k]=double(b[k])-a[k];y[k]=double(c[k])-a[k];}Normal n{x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0]};for(unsigned k=0;k<3;++k)faceNormals[t.polygon][k]+=n[k];}
 auto smooth=[](const std::string& s){return !s.empty()&&s!="off"&&s!="0";};
 std::map<std::pair<uint32_t,std::string>,Normal> shared;
 for(size_t i=0;i<model.polygons.size();++i){const auto& p=model.polygons[i];if(!smooth(p.smoothing))continue;std::vector<uint32_t> seen;
  for(const auto& c:p.corners)if(std::find(seen.begin(),seen.end(),c.position)==seen.end()){seen.push_back(c.position);auto& n=shared[{c.position,p.smoothing}];for(unsigned k=0;k<3;++k)n[k]+=faceNormals[i][k];}
 }
 auto normalized=[](const Normal& n){double length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(!std::isfinite(length)||length==0)throw std::runtime_error("OBJ zero/invalid normal");return std::array<float,3>{float(n[0]/length),float(n[1]/length),float(n[2]/length)};};
 StudioObjRenderMeshCandidate mesh;mesh.vertices.reserve(triangles.size()*3);mesh.indices.reserve(triangles.size()*3);mesh.hasTexture.reserve(triangles.size()*3);mesh.trianglePolygons.reserve(triangles.size());
 for(const auto& t:triangles){const auto& polygon=model.polygons[t.polygon];auto found=std::find(mesh.materials.begin(),mesh.materials.end(),polygon.material);uint32_t material=static_cast<uint32_t>(found-mesh.materials.begin());if(found==mesh.materials.end())mesh.materials.push_back(polygon.material);
  if(mesh.ranges.empty()||mesh.ranges.back().material!=material||mesh.ranges.back().object!=polygon.object||mesh.ranges.back().group!=polygon.group)mesh.ranges.push_back({static_cast<uint32_t>(mesh.indices.size()),0,material,polygon.object,polygon.group});
  for(const auto& c:t.corners){SceneGpuVertexCandidate vertex{};vertex.position=model.positions.at(c.position);Normal n=faceNormals[t.polygon];if(c.normal!=UINT32_MAX){auto supplied=model.normals.at(c.normal);for(unsigned k=0;k<3;++k)n[k]=supplied[k];}else if(smooth(polygon.smoothing))n=shared.at({c.position,polygon.smoothing});vertex.normal=normalized(n);
   const bool uv=c.texture!=UINT32_MAX;if(uv){auto texture=model.textures.at(c.texture);vertex.uv={texture[0],texture[1]};}if(!sceneGpuVertexFiniteCandidate(vertex))throw std::runtime_error("OBJ nonfinite render vertex");
   mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size()));mesh.vertices.push_back(vertex);mesh.hasTexture.push_back(uv);
  }mesh.ranges.back().indexCount+=3;mesh.trianglePolygons.push_back(t.polygon);
 }return mesh;
}
