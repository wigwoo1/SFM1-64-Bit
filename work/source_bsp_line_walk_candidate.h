#pragma once
#include "source_line_plane_split_candidate.h"
#include <vector>
#include <cstdint>
struct SourceLineWalkPlaneCandidate{std::array<float,3>normal{};float distance=0;uint8_t type=0;};
struct SourceLineWalkNodeCandidate{uint32_t plane=0;std::array<int32_t,2>children{};};
// Callback receives leaf index and global ray interval, and may update hit.
// Explicit frames avoid native recursion depth on large trees.
template<class VisitLeaf>
void sourceBspLineWalkCandidate(const std::vector<SourceLineWalkPlaneCandidate>&planes,
 const std::vector<SourceLineWalkNodeCandidate>&nodes,int32_t root,uint32_t leaves,
 const std::array<float,3>&start,const std::array<float,3>&end,float from,float to,float&hit,VisitLeaf visit){
 auto valid=[&](int32_t n){return n>=0?size_t(n)<nodes.size():uint64_t(-1-int64_t(n))<leaves;};
 if(!valid(root)||!std::isfinite(from)||!std::isfinite(to)||from<0||to>1||from>to||!std::isfinite(hit)||hit<0||hit>1)throw std::runtime_error("invalid line traversal input");
 for(float v:start)if(!std::isfinite(v))throw std::runtime_error("invalid line start");for(float v:end)if(!std::isfinite(v))throw std::runtime_error("invalid line end");
 for(const auto&p:planes){if(p.type>5||!std::isfinite(p.distance))throw std::runtime_error("invalid traversal plane");for(float v:p.normal)if(!std::isfinite(v))throw std::runtime_error("invalid traversal normal");}
 for(const auto&n:nodes)if(n.plane>=planes.size()||!valid(n.children[0])||!valid(n.children[1]))throw std::runtime_error("invalid traversal node");
 std::vector<uint8_t>colors(nodes.size());
 for(size_t i=0;i<nodes.size();++i){if(colors[i]==2)continue;std::vector<std::pair<size_t,bool>>pending{{i,false}};while(!pending.empty()){auto item=pending.back();pending.pop_back();if(item.second){colors[item.first]=2;continue;}if(colors[item.first]==2)continue;if(colors[item.first]==1)throw std::runtime_error("traversal cycle");colors[item.first]=1;pending.push_back({item.first,true});for(auto child:nodes[item.first].children)if(child>=0)pending.push_back({size_t(child),false});}}
 struct Frame{int32_t node;std::array<float,3>start,end;float from,to;};std::vector<Frame>pending{{root,start,end,from,to}};
 while(!pending.empty()){auto f=pending.back();pending.pop_back();if(f.from>=hit)continue;
  if(f.node<0){visit(uint32_t(-1-int64_t(f.node)),f.from,f.to);if(!std::isfinite(hit)||hit<0||hit>1)throw std::runtime_error("invalid callback hit fraction");continue;}
  const auto&n=nodes[size_t(f.node)];const auto&p=planes[n.plane];auto distance=[&](const auto&point){if(p.type<3)return point[p.type]-p.distance;const float y=point[1]*p.normal[1],x=point[0]*p.normal[0],z=point[2]*p.normal[2];const float xy=y+x;return (xy+z)-p.distance;};
  const auto split=sourceLinePlaneSplitCandidate(distance(f.start),distance(f.end));if(!split.split){f.node=n.children[split.side];pending.push_back(f);continue;}
  auto point=[&](float fraction){std::array<float,3>v{};for(size_t axis=0;axis<3;++axis){const float delta=f.end[axis]-f.start[axis];const float scaled=delta*fraction;v[axis]=scaled+f.start[axis];if(!std::isfinite(v[axis]))throw std::runtime_error("traversal midpoint overflow");}return v;};
  const float span=f.to-f.from;const float nearTime=span*split.nearEnd+f.from,farTime=span*split.farStart+f.from;
  pending.push_back({n.children[split.side^1],point(split.farStart),f.end,farTime,f.to});pending.push_back({n.children[split.side],f.start,point(split.nearEnd),f.from,nearTime});
 }
}
