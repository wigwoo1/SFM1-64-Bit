#pragma once
#include "engine_projection_normalize_candidate.h"
#include "engine_projection_planes_candidate.h"
// Composition recovered from10249F00/10249B70. Strict FP required.
inline float projectionMulCandidate(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline float projectionAddCandidate(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline float projectionSubCandidate(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline void engineProjectionPointCandidate(const float* m,const float* point,float* result){
 float coordinates[4];
 for(int r=0;r<4;++r)coordinates[r]=projectionAddCandidate(projectionAddCandidate(projectionAddCandidate(projectionMulCandidate(m[r*4],point[0]),projectionMulCandidate(m[r*4+1],point[1])),projectionMulCandidate(m[r*4+2],point[2])),m[r*4+3]);
 float scale=coordinates[3];
 if(scale!=0.0f)scale=_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(1),_mm_set_ss(scale)));
 for(int r=0;r<3;++r)result[r]=projectionMulCandidate(coordinates[r],scale);
}
inline void engineProjectionGeneratePlanesCandidate(const float* inverse,EngineProjectionPlanesCandidate& output){
 const float points[6][3][3]={
 {{1,0,0},{1,1,1},{1,1,0}},{{0,0,0},{0,1,1},{0,0,1}},
 {{1,1,0},{1,1,1},{0,1,1}},{{1,0,0},{0,0,1},{1,0,1}},
 {{0,0,0},{1,0,0},{0,1,0}},{{0,0,1},{0,1,1},{1,0,1}}};
 float planes[6][4];
 const int callOrder[6]={4,5,0,1,2,3};
 for(int index:callOrder){
  float a[3],b[3],c[3],u[3],v[3];
  engineProjectionPointCandidate(inverse,points[index][0],a);
  engineProjectionPointCandidate(inverse,points[index][1],b);
  engineProjectionPointCandidate(inverse,points[index][2],c);
  for(int k=0;k<3;++k){u[k]=projectionSubCandidate(c[k],a[k]);v[k]=projectionSubCandidate(b[k],a[k]);}
  for(int k=0;k<3;++k){int j=(k+1)%3,l=(k+2)%3;planes[index][k]=projectionSubCandidate(projectionMulCandidate(v[j],u[l]),projectionMulCandidate(v[l],u[j]));}
  engineProjectionNormalizeCandidate(planes[index]);
  planes[index][3]=projectionAddCandidate(projectionAddCandidate(projectionMulCandidate(planes[index][1],a[1]),projectionMulCandidate(planes[index][0],a[0])),projectionMulCandidate(planes[index][2],a[2]));
 }
 uint32_t bits[24];std::memcpy(bits,planes,sizeof(bits));engineProjectionPackPlanesCandidate(bits,output);
}

