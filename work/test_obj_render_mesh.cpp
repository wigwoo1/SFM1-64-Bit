#include <cstdio>
#include <cstdlib>
#include "studio_obj_render_mesh_candidate.h"
static void check(bool b){if(!b)std::abort();}
static bool near(float a,float b){return std::abs(a-b)<1e-6f;}
int main(){unsigned cases=0;
 for(bool smooth:{false,true})for(bool supplied:{false,true}){
  StudioObjModelCandidate m;m.positions={{{0,0,0}},{{1,0,0}},{{0,1,0}},{{0,0,1}}};m.textures={{{0,0,0}},{{1,0,0}},{{0,1,0}},{{.75f,.25f,0}}};m.normals={{{2,0,0}}};
  StudioObjPolygonCandidate a,b;a.corners={{0,0},{1,1},{2,2}};b.corners={{0,3},{3},{1}};a.material="Skin";b.material="Fur";a.object=b.object="Model";a.group="body";b.group="detail";a.smoothing=b.smoothing=smooth?"1":"off";if(supplied)for(auto& c:b.corners)c.normal=0;m.polygons={a,b};
  auto mesh=studioPrepareObjRenderMeshCandidate(m);check(mesh.vertices.size()==6&&mesh.indices.size()==6&&mesh.ranges.size()==2&&mesh.materials==std::vector<std::string>({"Skin","Fur"}));check(mesh.ranges[0].firstIndex==0&&mesh.ranges[0].indexCount==3&&mesh.ranges[1].firstIndex==3&&mesh.ranges[1].material==1&&mesh.ranges[1].group=="detail");
  for(unsigned i=0;i<6;++i){check(mesh.indices[i]==i&&sceneGpuVertexFiniteCandidate(mesh.vertices[i]));auto n=mesh.vertices[i].normal;check(near(n[0]*n[0]+n[1]*n[1]+n[2]*n[2],1));}
  check(mesh.vertices[0].uv==std::array<float,2>{0,0}&&mesh.vertices[3].uv==std::array<float,2>{.75f,.25f}&&mesh.hasTexture[3]&&!mesh.hasTexture[4]);
  auto first=mesh.vertices[0].normal;check(near(first[1],smooth?float(1/std::sqrt(2.0)):0)&&near(first[2],smooth?float(1/std::sqrt(2.0)):1));
  auto second=mesh.vertices[3].normal;check(supplied?near(second[0],1):near(second[1],smooth?float(1/std::sqrt(2.0)):1));check(m.normals[0][0]==2&&m.positions[0][0]==0);++cases;
 }
 std::istringstream input("v 0 0 0\nv 2 0 0\nv 2 1 0\nv 1 1 0\nv 1 2 0\nv 0 2 0\nf 1 2 3 4 5 6\n");auto parsed=studioReadObjCandidate(input);auto concave=studioPrepareObjRenderMeshCandidate(parsed);check(concave.vertices.size()==12&&concave.indices.size()==12&&concave.ranges.size()==1&&concave.ranges[0].indexCount==12);for(auto& v:concave.vertices)check(v.normal==std::array<float,3>{0,0,1});
 std::puts("obj_render_mesh_normal_smoothing_supplied_normals_uv_seams_material_ranges_and_concave_import=passed");std::printf("normal_policy_cases=%u vertex_stride=%zu\n",cases,sizeof(SceneGpuVertexCandidate));
}
