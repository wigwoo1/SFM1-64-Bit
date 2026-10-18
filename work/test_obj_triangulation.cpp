#include <cstdio>
#include <cstdlib>
#include "studio_obj_triangulation_candidate.h"
static void check(bool b){if(!b)std::abort();}
int main(){const std::array<std::array<float,3>,6> base{{{0,0,0},{2,0,0},{2,1,0},{1,1,0},{1,2,0},{0,2,0}}};unsigned cases=0;
 for(unsigned axis=0;axis<3;++axis)for(bool reverse:{false,true})for(float scale:{0.001f,1.0f,1000.0f}){
  StudioObjModelCandidate m;StudioObjPolygonCandidate polygon;polygon.material="skin";polygon.group="body";
  for(unsigned i=0;i<6;++i){auto point=base[i];std::array<float,3> p{};p[(axis+1)%3]=point[0]*scale;p[(axis+2)%3]=point[1]*scale;m.positions.push_back(p);m.textures.push_back({float(i),0,0});m.normals.push_back({0,0,1});}
  for(unsigned i=0;i<6;++i){unsigned index=reverse?5-i:i;polygon.corners.push_back({index,index,index});}m.polygons.push_back(polygon);auto triangles=studioTriangulateObjCandidate(m);check(triangles.size()==4);double total=0;
  for(const auto& t:triangles){check(t.polygon==0&&m.polygons[t.polygon].material=="skin"&&m.polygons[t.polygon].group=="body");for(auto c:t.corners)check(c.position==c.texture&&c.position==c.normal);
   auto a=base[t.corners[0].position],b=base[t.corners[1].position],c=base[t.corners[2].position];double area=((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))/2;check(reverse?area<0:area>0);total+=area;float x=(a[0]+b[0]+c[0])/3,y=(a[1]+b[1]+c[1])/3;check(x<=1||y<=1);
  }check(total==(reverse?-3:3));++cases;
 }
 unsigned rejects=0;
 for(unsigned bad=0;bad<4;++bad){StudioObjModelCandidate m;m.positions={{{0,0,0}},{{1,0,0}},{{1,1,0}},{{0,1,0}}};StudioObjPolygonCandidate f;f.corners={{0},{1},{2},{3}};
  if(bad==0)f.corners={{0},{2},{1},{3}};if(bad==1)m.positions[2][2]=1;if(bad==2)f.corners={{0},{1},{1},{3}};if(bad==3)m.positions={{{0,0,0}},{{1,0,0}},{{2,0,0}},{{3,0,0}}};m.polygons.push_back(f);bool rejected=false;try{studioTriangulateObjCandidate(m);}catch(const std::runtime_error&){rejected=true;}check(rejected);++rejects;
 }
 std::printf("obj_concave_triangulation_cases=%u winding_area_corner_attributes=passed invalid_polygons=%u rejected\n",cases,rejects);
}
