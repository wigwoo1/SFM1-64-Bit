#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

// Point lookup uses SDK conventions; constrainToStartLeaf is separately checked
// against installed low-level instructions. Neither implies world-trace parity.
class SourceBspPointLeafCandidate {
 struct Plane{std::array<float,3>normal;float distance;int32_t type;};
 struct Node{uint32_t plane;std::array<int32_t,2>children;};
 std::vector<Plane>planes_;
 std::vector<Node>nodes_;
 int32_t root_=0;
 uint32_t leaves_=0;
public:
 explicit SourceBspPointLeafCandidate(const std::vector<uint8_t>&b){
  auto u32=[&](size_t p){return uint32_t(b.at(p))|(uint32_t(b.at(p+1))<<8)|(uint32_t(b.at(p+2))<<16)|(uint32_t(b.at(p+3))<<24);};
  auto signedWord=[&](size_t p){auto bits=u32(p);int32_t value;std::memcpy(&value,&bits,4);return value;};
  auto scalar=[&](size_t p){auto bits=u32(p);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value))throw std::runtime_error("nonfinite BSP plane");return value;};
  if(b.size()<1036||u32(0)!=0x50534256||u32(4)!=20)throw std::runtime_error("point leaf requires BSP20");
  auto lump=[&](unsigned id,size_t stride,uint32_t version){size_t p=8+id*16;auto start=u32(p),bytes=u32(p+4);if(start>INT32_MAX||bytes>INT32_MAX||uint64_t(start)+bytes>b.size()||(bytes&&start<1036)||bytes%stride||u32(p+8)!=version||u32(p+12))throw std::runtime_error("invalid point-leaf BSP lump");return std::array<size_t,2>{start,bytes/stride};};
  auto planes=lump(1,20,0),nodes=lump(5,32,0),leaves=lump(10,32,1),models=lump(14,48,0);
  if(!leaves[1]||leaves[1]>65536||nodes[1]>65536||planes[1]>65536||!models[1])throw std::runtime_error("unsupported point-leaf table coverage");leaves_=uint32_t(leaves[1]);root_=signedWord(models[0]+36);
  for(size_t i=0;i<planes[1];++i){size_t p=planes[0]+i*20;Plane plane{{scalar(p),scalar(p+4),scalar(p+8)},scalar(p+12),signedWord(p+16)};if(plane.type<0||plane.type>5)throw std::runtime_error("BSP plane type");planes_.push_back(plane);}
  auto childValid=[&](int32_t child){return child>=0?uint64_t(child)<nodes[1]:uint64_t(-1-int64_t(child))<leaves_;};
  if(!childValid(root_))throw std::runtime_error("BSP world root outside tables");
  for(size_t i=0;i<nodes[1];++i){size_t p=nodes[0]+i*32;Node node{u32(p),{signedWord(p+4),signedWord(p+8)}};
   if(node.plane>=planes_.size()||!childValid(node.children[0])||!childValid(node.children[1]))throw std::runtime_error("BSP node reference");
   const auto&plane=planes_[node.plane];if(plane.type<3){for(int axis=0;axis<3;++axis)if(plane.normal[size_t(axis)]!=(axis==plane.type?1.f:0.f))throw std::runtime_error("noncanonical referenced axial plane");}
   else if(plane.normal==std::array<float,3>{0,0,0})throw std::runtime_error("zero general plane");nodes_.push_back(node);
  }
  // Validate the entire node forest without recursion; querying never walks a cycle.
  std::vector<uint8_t>state(nodes_.size());
  for(uint32_t start=0;start<nodes_.size();++start){if(state[start]==2)continue;std::vector<std::pair<uint32_t,bool>>stack{{start,false}};
   while(!stack.empty()){auto entry=stack.back();stack.pop_back();auto node=entry.first;if(entry.second){state[node]=2;continue;}if(state[node]==2)continue;if(state[node]==1)throw std::runtime_error("BSP node cycle");state[node]=1;stack.push_back({node,true});for(auto child:nodes_[node].children)if(child>=0)stack.push_back({uint32_t(child),false});}
  }
 }
 uint32_t leafAt(const std::array<float,3>&point)const{
  for(float value:point)if(!std::isfinite(value))throw std::runtime_error("nonfinite BSP point");int32_t cursor=root_;
  while(cursor>=0){const auto&node=nodes_.at(size_t(cursor));const auto&plane=planes_.at(node.plane);float distance;
   if(plane.type<3)distance=point[size_t(plane.type)]-plane.distance;
   else distance=((point[0]*plane.normal[0]+point[1]*plane.normal[1])+point[2]*plane.normal[2])-plane.distance;
   if(!std::isfinite(distance))throw std::runtime_error("BSP point distance overflow");cursor=node.children[distance<0?1:0];
  }
  return uint32_t(-1-int64_t(cursor));
 }
 size_t nodeCount()const{return nodes_.size();}size_t planeCount()const{return planes_.size();}uint32_t leafCount()const{return leaves_;}int32_t worldRoot()const{return root_;}
 // Recovered native 10145690 plane-by-plane target adjustment. This is not a
 // brush collision trace. Exact plane ties intentionally retain native rules.
 std::array<float,3>constrainToStartLeaf(const std::array<float,3>&start,float tolerance,const std::array<float,3>&target)const{
  for(float value:start)if(!std::isfinite(value))throw std::runtime_error("nonfinite BSP segment start");
  for(float value:target)if(!std::isfinite(value))throw std::runtime_error("nonfinite BSP segment target");
  if(!std::isfinite(tolerance)||tolerance<0)throw std::runtime_error("invalid BSP segment tolerance");
  auto result=target;int32_t cursor=root_;
  while(cursor>=0){const auto&node=nodes_.at(size_t(cursor));const auto&plane=planes_.at(node.plane);
   auto distance=[&](const auto&point){float value;if(plane.type<3)value=point[size_t(plane.type)]-plane.distance;
    else{const float y=point[1]*plane.normal[1],x=point[0]*plane.normal[0],z=point[2]*plane.normal[2];const float xy=y+x;value=(xy+z)-plane.distance;}
    if(!std::isfinite(value))throw std::runtime_error("BSP segment distance overflow");return value;};
   const float from=distance(start),to=distance(result);const bool negative=from<0.0f;
   if(negative?to>0.0f:to<0.0f){const float amount=negative?to+tolerance:tolerance-to;
    for(size_t axis=0;axis<3;++axis){const float correction=plane.normal[axis]*amount;result[axis]=negative?result[axis]-correction:result[axis]+correction;if(!std::isfinite(result[axis]))throw std::runtime_error("BSP segment adjustment overflow");}
   }
   cursor=node.children[negative?1:0];
  }
  return result;
 }
};
