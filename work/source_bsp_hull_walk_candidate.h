#pragma once
#include "source_bsp_line_walk_candidate.h"

// Nonzero-extent swept BSP traversal (engine 10148c30). Leaf callbacks own
// brush/displacement contact tests; this walker alone is not a world trace.
template<class VisitLeaf>
void sourceBspHullWalkCandidate(const std::vector<SourceLineWalkPlaneCandidate>& planes,
 const std::vector<SourceLineWalkNodeCandidate>& nodes,int32_t root,uint32_t leaves,
 const std::array<float,3>& start,const std::array<float,3>& end,
 const std::array<float,3>& extents,float from,float to,float& hit,VisitLeaf visit) {
 auto valid=[&](int32_t n){return n>=0?size_t(n)<nodes.size():uint64_t(-1-int64_t(n))<leaves;};
 if(!valid(root)||!std::isfinite(from)||!std::isfinite(to)||from<0||to>1||from>to||!std::isfinite(hit)||hit<0||hit>1)throw std::runtime_error("invalid hull traversal input");
 for(size_t a=0;a<3;++a)if(!std::isfinite(start[a])||!std::isfinite(end[a])||!std::isfinite(extents[a])||extents[a]<0)throw std::runtime_error("invalid hull geometry");
 for(const auto& p:planes){if(p.type>5||!std::isfinite(p.distance))throw std::runtime_error("invalid hull plane");for(float v:p.normal)if(!std::isfinite(v))throw std::runtime_error("invalid hull normal");}
 for(const auto& n:nodes)if(n.plane>=planes.size()||!valid(n.children[0])||!valid(n.children[1]))throw std::runtime_error("invalid hull node");
 std::vector<uint8_t> colors(nodes.size());
 for(size_t i=0;i<nodes.size();++i){if(colors[i]==2)continue;std::vector<std::pair<size_t,bool>> pending{{i,false}};while(!pending.empty()){auto item=pending.back();pending.pop_back();if(item.second){colors[item.first]=2;continue;}if(colors[item.first]==2)continue;if(colors[item.first]==1)throw std::runtime_error("hull traversal cycle");colors[item.first]=1;pending.push_back({item.first,true});for(auto child:nodes[item.first].children)if(child>=0)pending.push_back({size_t(child),false});}}
 struct Frame {int32_t node;std::array<float,3> start,end;float from,to;};
 std::vector<Frame> pending{{root,start,end,from,to}};
 auto clamp=[](float v){return v>1?1.f:v<0?0.f:v;};
 while(!pending.empty()) {
  auto f=pending.back();pending.pop_back();if(f.from>=hit)continue;
  if(f.node<0){visit(uint32_t(-1-int64_t(f.node)),f.from,f.to);if(!std::isfinite(hit)||hit<0||hit>1)throw std::runtime_error("invalid hull callback fraction");continue;}
  const auto& n=nodes[size_t(f.node)];const auto& p=planes[n.plane];
  auto distance=[&](const auto& point){if(p.type<3)return point[p.type]-p.distance;const float y=point[1]*p.normal[1],x=point[0]*p.normal[0],z=point[2]*p.normal[2];const float xy=y+x;return (xy+z)-p.distance;};
  const double offset=p.type<3?double(extents[p.type]):std::abs(double(extents[0])*p.normal[0])+std::abs(double(extents[1])*p.normal[1])+std::abs(double(extents[2])*p.normal[2]);
  const float roundedOffset=float(offset),a=distance(f.start),b=distance(f.end);
  if(a>roundedOffset&&b>roundedOffset){f.node=n.children[0];pending.push_back(f);continue;}
  if(a<-roundedOffset&&b<-roundedOffset){f.node=n.children[1];pending.push_back(f);continue;}
  unsigned side=0;float nearEnd=1,farStart=0;
  if(a<b){side=1;const double inverse=1.0/(double(a)-b);farStart=float((double(a)+offset+.03125)*inverse);nearEnd=float((double(a)-offset-.03125)*inverse);}
  else if(a>b){const double inverse=1.0/(double(a)-b);farStart=float((double(a)-offset-.03125)*inverse);nearEnd=float((double(a)+offset+.03125)*inverse);}
  nearEnd=clamp(nearEnd);farStart=clamp(farStart);
  auto point=[&](float t){std::array<float,3> v{};for(size_t axis=0;axis<3;++axis){const float delta=f.end[axis]-f.start[axis],scaled=delta*t;v[axis]=scaled+f.start[axis];if(!std::isfinite(v[axis]))throw std::runtime_error("hull midpoint overflow");}return v;};
  const float span=f.to-f.from,nearTime=span*nearEnd+f.from,farTime=span*farStart+f.from;
  pending.push_back({n.children[side^1],point(farStart),f.end,farTime,f.to});
  pending.push_back({n.children[side],f.start,point(nearEnd),f.from,nearTime});
 }
}
