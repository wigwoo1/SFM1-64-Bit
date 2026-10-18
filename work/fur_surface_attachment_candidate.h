#pragma once
#include "model_skinning_geometry.h"
#include <algorithm>
// Persistent triangle anchors; caller supplies the same model/LOD topology.
// Length is in world units. Rendering, grooming and simulation are separate.
struct FurSurfaceAnchorCandidate{std::array<uint32_t,3>vertices;std::array<float,3>weights;float length;};
struct FurSurfaceGuideCandidate{std::array<float,3>root,tip;};
inline FurSurfaceGuideCandidate sampleFurSurfaceGuideCandidate(const FurSurfaceAnchorCandidate&anchor,const std::vector<GeometryDeformedVertexCandidate>&vertices){

 if(!std::isfinite(anchor.length)||anchor.length<0)throw std::runtime_error("invalid fur length");float sum=0;for(float weight:anchor.weights){if(!std::isfinite(weight)||weight<0||weight>1)throw std::runtime_error("invalid fur barycentric weight");sum+=weight;}if(std::fabs(sum-1)>1e-5f)throw std::runtime_error("fur weights must sum to one");FurSurfaceGuideCandidate guide{};std::array<double,3>normal{};
 for(size_t corner=0;corner<3;++corner){if(anchor.vertices[corner]>=vertices.size())throw std::runtime_error("fur vertex outside model topology");const auto&vertex=vertices[anchor.vertices[corner]];for(size_t axis=0;axis<3;++axis){if(!std::isfinite(vertex.position[axis])||!std::isfinite(vertex.normal[axis]))throw std::runtime_error("nonfinite fur surface");guide.root[axis]+=vertex.position[axis]*anchor.weights[corner];normal[axis]+=double(vertex.normal[axis])*anchor.weights[corner];}}
 double norm=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);if(!(norm>0)||!std::isfinite(norm))throw std::runtime_error("invalid fur surface normal");for(size_t axis=0;axis<3;++axis){guide.tip[axis]=guide.root[axis]+float(normal[axis]/norm*anchor.length);if(!std::isfinite(guide.root[axis])||!std::isfinite(guide.tip[axis]))throw std::runtime_error("fur guide overflow");}return guide;

}
inline std::vector<FurSurfaceGuideCandidate>sampleFurSurfaceGuidesCandidate(const std::vector<FurSurfaceAnchorCandidate>&anchors,const std::vector<GeometryDeformedVertexCandidate>&vertices){std::vector<FurSurfaceGuideCandidate>result;result.reserve(anchors.size());for(const auto&anchor:anchors)result.push_back(sampleFurSurfaceGuideCandidate(anchor,vertices));return result;}
struct FurTopologyCandidate{std::string modelName;uint32_t checksum,lod;size_t vertexCount;std::vector<std::array<uint32_t,3>>triangles;};
class FurSurfaceAttachmentSetCandidate{
 FurTopologyCandidate topology_;std::vector<FurSurfaceAnchorCandidate>anchors_;
 void validateTopology(const FurTopologyCandidate&topology,const std::vector<GeometryDeformedVertexCandidate>&vertices)const{if(topology.modelName!=topology_.modelName||topology.checksum!=topology_.checksum||topology.lod!=topology_.lod||topology.vertexCount!=topology_.vertexCount||topology.triangles!=topology_.triangles||vertices.size()!=topology_.vertexCount)throw std::runtime_error("fur attachment topology changed");}
public:
 FurSurfaceAttachmentSetCandidate(FurTopologyCandidate topology,std::vector<FurSurfaceAnchorCandidate>anchors):topology_(std::move(topology)),anchors_(std::move(anchors)){
  if(topology_.modelName.empty())throw std::runtime_error("fur model identity missing");for(const auto&triangle:topology_.triangles)for(auto index:triangle)if(index>=topology_.vertexCount)throw std::runtime_error("fur topology index invalid");
  auto orderedTriangles=topology_.triangles;std::sort(orderedTriangles.begin(),orderedTriangles.end());for(const auto&anchor:anchors_)if(!std::binary_search(orderedTriangles.begin(),orderedTriangles.end(),anchor.vertices))throw std::runtime_error("fur anchor triangle missing");
 }
 template<class Pool>std::vector<FurSurfaceGuideCandidate>sampleWith(const FurTopologyCandidate&topology,const std::vector<GeometryDeformedVertexCandidate>&vertices,Pool&pool)const{validateTopology(topology,vertices);return pool.sample(anchors_,vertices);}
 std::vector<FurSurfaceGuideCandidate>sample(const FurTopologyCandidate&topology,const std::vector<GeometryDeformedVertexCandidate>&vertices)const{
  validateTopology(topology,vertices);return sampleFurSurfaceGuidesCandidate(anchors_,vertices);
 }
};
