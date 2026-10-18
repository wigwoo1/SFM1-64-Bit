#include "studio_obj_triangulation_candidate.h"
#include <fstream>
#include <cstring>
int main(){
 std::ofstream out("outputs/cycles-conversion/triangulation-oracle.jsonl");
 if(!out)return 1;
 const std::array<std::array<float,3>,6> base{{{0,0,0},{2,0,0},{2,1,0},{1,1,0},{1,2,0},{0,2,0}}};
 for(unsigned axis=0;axis<3;++axis)for(bool reverse:{false,true})for(float scale:{0.001f,1.0f,1000.0f}){
  StudioObjModelCandidate model;StudioObjPolygonCandidate polygon;
  for(auto point:base){std::array<float,3> p{};p[(axis+1)%3]=point[0]*scale;p[(axis+2)%3]=point[1]*scale;model.positions.push_back(p);}
  for(unsigned i=0;i<6;++i){auto index=reverse?5-i:i;polygon.corners.push_back({index,index,index});}
  model.polygons.push_back(polygon);auto triangles=studioTriangulateObjCandidate(model);
  out<<"{\"position_bits\":[";
  for(size_t i=0;i<model.positions.size();++i){if(i)out<<',';out<<'[';for(unsigned k=0;k<3;++k){if(k)out<<',';uint32_t bits;std::memcpy(&bits,&model.positions[i][k],4);out<<bits;}out<<']';}
  out<<"],\"corners\":[";for(size_t i=0;i<polygon.corners.size();++i){if(i)out<<',';out<<polygon.corners[i].position;}
  out<<"],\"triangles\":[";for(size_t i=0;i<triangles.size();++i){if(i)out<<',';out<<'[';for(unsigned k=0;k<3;++k){if(k)out<<',';out<<triangles[i].corners[k].position;}out<<']';}out<<"]}\n";
 }
 return out?0:1;
}
