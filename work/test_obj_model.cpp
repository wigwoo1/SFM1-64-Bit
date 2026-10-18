#include <cstdio>
#include <cstdlib>
#include "studio_obj_model_candidate.h"
static void check(bool x){if(!x)std::abort();}
static StudioObjModelCandidate read(const char* text){std::istringstream input(text);return studioReadObjCandidate(input);}
int main(){
 const auto m=read("mtllib body.mtl fur.mtl\nv 0 0 0\nv 2 0 0 2\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\no My object\ng body detail\nusemtl Skin\ns 1\nf -3/-3/-1 -2/-2/-1 -1/-1/-1 # triangle\nf 1//1 2//1 3//1\nf 1 2 3\n");
 check(m.positions.size()==3&&m.positions[1][0]==1&&m.polygons.size()==3&&m.materialLibraries.size()==2);
 const auto& a=m.polygons[0];check(a.object=="My object"&&a.group=="body detail"&&a.material=="Skin"&&a.smoothing=="1");for(unsigned i=0;i<3;++i)check(a.corners[i].position==i&&a.corners[i].texture==i&&a.corners[i].normal==0);
 check(m.polygons[1].corners[0].texture==UINT32_MAX&&m.polygons[2].corners[0].normal==UINT32_MAX);
 auto forward=read("f 1 2 3\nv 0 0 0\nv 1 0 0\nv 0 1 0\n");check(forward.polygons[0].corners[2].position==2);
 unsigned rejected=0;const std::string prefix="v 0 0 0\nv 1 0 0\nv 0 1 0\n";
 for(auto suffix:{"f 0 2 3","f -4 2 3","f 1 2 4","f 1 2","f 1/ 2 3","f 1///1 2 3","f 1/1 2 3","f 1//1 2 3","vt 0 nan\nf 1 2 3","v 0 0 0 0\nf 1 2 3","f 999999999999999999999 2 3","curv 1 2 3"}){bool failed=false;try{read((prefix+suffix).c_str());}catch(const std::runtime_error&){failed=true;}check(failed);++rejected;}
 std::printf("obj_model_positions_uv_normals_material_groups_relative_forward_indices=passed malformed_cases=%u rejected\n",rejected);
}
