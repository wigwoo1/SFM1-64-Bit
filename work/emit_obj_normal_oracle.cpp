#include "studio_obj_render_mesh_candidate.h"
#include <fstream>
#include <cstring>
int main(){
 std::ofstream out("outputs/cycles-conversion/normal-oracle.jsonl");if(!out)return 1;
 for(unsigned mode=0;mode<3;++mode){
  StudioObjModelCandidate m;m.positions={{{0,0,0}},{{1,0,0}},{{0,1,0}},{{0,0,2}}};m.normals={{{0,2,0}}};
  StudioObjPolygonCandidate a,b;a.smoothing=b.smoothing=mode?"1":"off";
  a.corners={{0,UINT32_MAX,mode==2?0u:UINT32_MAX},{1,UINT32_MAX,UINT32_MAX},{2,UINT32_MAX,UINT32_MAX}};
  b.corners={{0,UINT32_MAX,UINT32_MAX},{2,UINT32_MAX,UINT32_MAX},{3,UINT32_MAX,UINT32_MAX}};
  m.polygons={a,b};auto mesh=studioPrepareObjRenderMeshCandidate(m);
  out<<"{\"mode\":"<<mode<<",\"normal_bits\":[";
  for(size_t i=0;i<mesh.vertices.size();++i){if(i)out<<',';out<<'[';for(unsigned k=0;k<3;++k){if(k)out<<',';uint32_t bits;std::memcpy(&bits,&mesh.vertices[i].normal[k],4);out<<bits;}out<<']';}out<<"]}\n";
 }
 return out?0:1;
}
