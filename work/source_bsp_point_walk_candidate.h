#pragma once
#include "source_bsp_line_walk_candidate.h"

// Point-route10148950. Work extents contribute only at axial planes; oblique
// planes use zero offset even when isRay=true has small nonzero hull extents.
template<class VisitLeaf>
void sourceBspPointWalkCandidate(const std::vector<SourceLineWalkPlaneCandidate>& planes,
 const std::vector<SourceLineWalkNodeCandidate>& nodes,int32_t root,uint32_t leaves,
 const std::array<float,3>& start,const std::array<float,3>& end,const std::array<float,3>& axialExtents,
 float from,float to,float& hit,VisitLeaf visit){
 auto valid=[&](int32_t n){return n>=0?size_t(n)<nodes.size():uint64_t(-1-int64_t(n))<leaves;};
 if(!valid(root)||!std::isfinite(from)||!std::isfinite(to)||from<0||to>1||from>to||!std::isfinite(hit)||hit<0||hit>1)throw std::runtime_error("invalid point traversal input");
 for(size_t a=0;a<3;++a)if(!std::isfinite(start[a])||!std::isfinite(end[a])||!std::isfinite(axialExtents[a])||axialExtents[a]<0)throw std::runtime_error("invalid point traversal geometry");
 for(const auto& p:planes){if(p.type>5||!std::isfinite(p.distance))throw std::runtime_error("invalid point traversal plane");for(float v:p.normal)if(!std::isfinite(v))throw std::runtime_error("invalid point normal");}
 for(const auto& n:nodes)if(n.plane>=planes.size()||!valid(n.children[0])||!valid(n.children[1]))throw std::runtime_error("invalid point node");
 std::vector<uint8_t> colors(nodes.size());
 for(size_t i=0;i<nodes.size();++i){if(colors[i]==2)continue;std::vector<std::pair<size_t,bool>> pending{{i,false}};while(!pending.empty()){auto item=pending.back();pending.pop_back();if(item.second){colors[item.first]=2;continue;}if(colors[item.first]==2)continue;if(colors[item.first]==1)throw std::runtime_error("point traversal cycle");colors[item.first]=1;pending.push_back({item.first,true});for(auto child:nodes[item.first].children)if(child>=0)pending.push_back({size_t(child),false});}}
 struct Frame{int32_t node;std::array<float,3> start,end;float from,to;};
 std::vector<Frame> pending{{root,start,end,from,to}};
 auto clamp=[](float value){return value>1?1.f:value<0?0.f:value;};
 while(!pending.empty()){
  auto f=pending.back();pending.pop_back();if(f.from>=hit)continue;
  if(f.node<0){visit(uint32_t(-1-int64_t(f.node)),f.from,f.to);if(!std::isfinite(hit)||hit<0||hit>1)throw std::runtime_error("invalid point callback fraction");continue;}
  const auto& n=nodes[size_t(f.node)];const auto& p=planes[n.plane];
  auto distance=[&](const auto& point){if(p.type<3)return point[p.type]-p.distance;const float y=point[1]*p.normal[1],x=point[0]*p.normal[0],z=point[2]*p.normal[2],xy=y+x;return (xy+z)-p.distance;};
  const float a=distance(f.start),b=distance(f.end),offset=p.type<3?axialExtents[p.type]:0.f;
  if(a>offset&&b>offset){f.node=n.children[0];pending.push_back(f);continue;}
  if(a<-offset&&b<-offset){f.node=n.children[1];pending.push_back(f);continue;}
  unsigned side=0;float nearEnd=1,farStart=0;
  if(a<b){side=1;const double inverse=1.0/(double(a)-b);nearEnd=float((double(a)-offset-.03125)*inverse);farStart=float((double(a)+offset+.03125)*inverse);}
  else if(a>b){const double inverse=1.0/(double(a)-b);nearEnd=float((double(a)+offset+.03125)*inverse);farStart=float((double(a)-offset-.03125)*inverse);}
  nearEnd=clamp(nearEnd);farStart=clamp(farStart);
  auto point=[&](float t){std::array<float,3> v{};for(size_t axis=0;axis<3;++axis){const float d=f.end[axis]-f.start[axis],scaled=d*t;v[axis]=scaled+f.start[axis];if(!std::isfinite(v[axis]))throw std::runtime_error("point midpoint overflow");}return v;};
  const float span=f.to-f.from,nearTime=span*nearEnd+f.from,farTime=span*farStart+f.from;
  pending.push_back({n.children[side^1],point(farStart),f.end,farTime,f.to});
  pending.push_back({n.children[side],f.start,point(nearEnd),f.from,nearTime});
 }
}
