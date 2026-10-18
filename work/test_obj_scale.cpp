#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include "studio_obj_scale_candidate.h"
static void check(bool b){if(!b)std::abort();}
int main(){std::vector<SceneGpuVertexCandidate> original{{{0,0,0},{0,0,1},{0,0}},{{2,0,0},{0,0,1},{1,0}},{{0,2,0},{0,0,1},{0,1}}};auto scaled=studioScaleObjVerticesCandidate(original,.5f);check(scaled[0].position==std::array<float,3>{.5f,.5f,0}&&scaled[1].position==std::array<float,3>{1.5f,.5f,0});for(unsigned i=0;i<3;++i)check(scaled[i].normal==original[i].normal&&scaled[i].uv==original[i].uv);check(original[0].position[0]==0);unsigned rejected=0;for(float f:{0.0f,-1.0f,INFINITY,NAN,1e-40f}){try{studioScaleObjVerticesCandidate(original,f);}catch(const std::runtime_error&){++rejected;}}check(rejected==5);std::puts("obj_positive_uniform_scale_bounds_center_original_uv_normal_preservation=passed invalid_or_collapsed_factors=5 rejected");}

