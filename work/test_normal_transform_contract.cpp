#include "model_skinning_geometry.h"
#include "scene_gpu_vertex_candidate.h"
#include <iostream>
#include <cstddef>
int main(){try{
 static_assert(sizeof(void*)==8);static_assert(sizeof(SceneGpuVertexCandidate)==32);
 static_assert(offsetof(SceneGpuVertexCandidate,normal)==12);
 std::array<uint8_t,48> v{};auto put=[&](size_t o,float x){std::memcpy(v.data()+o,&x,4);};
 put(0,.25f);put(4,.75f);v[12]=0;v[13]=1;v[15]=2;
 put(16,2);put(20,3);put(24,4);put(28,1);put(32,0);put(36,0);
 GeometryMatrixCandidate z{0,-1,0,100,1,0,0,200,0,0,1,300};
 GeometryMatrixCandidate y{0,0,1,-100,0,1,0,-200,-1,0,0,-300};
 std::array<uint8_t,16> tangent{};std::array<float,4> t{0,1,0,-1};std::memcpy(tangent.data(),t.data(),16);
 auto out=skinGeometryVertexCandidate(v,&tangent,{z,y});
 if(out.normal!=std::array<float,3>{0,.25f,-.75f})throw std::runtime_error("weighted rotation normal");
 if(out.tangent!=std::array<float,4>{-.25f,.75f,0,-1})throw std::runtime_error("tangent rotation/handedness");
 z[3]=z[7]=z[11]=y[3]=y[7]=y[11]=0;
 auto untranslated=skinGeometryVertexCandidate(v,&tangent,{z,y});
 if(out.normal!=untranslated.normal||out.tangent!=untranslated.tangent||out.position==untranslated.position)throw std::runtime_error("translation contamination");
 // Linear blending intentionally leaves length correction to the fragment shader.
 if(out.normal[1]*out.normal[1]+out.normal[2]*out.normal[2]!=.625f)throw std::runtime_error("unexpected normalization");
 std::cout<<"x64 normal/tangent weighted rigid rotation, translation exclusion, unnormalized blend and GPU normal offset passed\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
