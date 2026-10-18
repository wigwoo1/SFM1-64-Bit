#pragma once
#include "engine_projection_generate_candidate.h"
inline float projectionDivCandidate(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}
inline float projectionSqrtCandidate(float a){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a)));}
//1024A9C0 recovered construction. Strict FP; no zero-length fallback.
inline void engineProjectionSphereCandidate(const float* inverse,float* center,float& radius){
 // Original C is the origin, not the opposite near-plane corner.
 // Group X+Y before scaling; generic point multiplication differs in rounding.
 float p[4][3];
 for(int i=0;i<4;++i){float c[4];for(int r=0;r<4;++r){const float* row=inverse+r*4;
  float xy=projectionAddCandidate(row[0],row[1]);
  if(i<2){c[r]=projectionAddCandidate(projectionAddCandidate(projectionMulCandidate(xy,.5f),i==0?projectionMulCandidate(row[2],0):row[2]),row[3]);}
  else if(i==2)c[r]=projectionAddCandidate(projectionMulCandidate(projectionAddCandidate(xy,row[2]),0),row[3]);
  else c[r]=projectionAddCandidate(projectionAddCandidate(projectionMulCandidate(xy,0),row[2]),row[3]);
 }
 float scale=c[3];if(scale!=0)scale=projectionDivCandidate(1,scale);
 for(int k=0;k<3;++k)p[i][k]=projectionMulCandidate(c[k],scale);
 }
 float ab[3],ac[3],bd[3];for(int k=0;k<3;++k){ab[k]=projectionSubCandidate(p[1][k],p[0][k]);ac[k]=projectionSubCandidate(p[0][k],p[2][k]);bd[k]=projectionSubCandidate(p[1][k],p[3][k]);}
 auto square=[](float x){return projectionMulCandidate(x,x);};
 float length=projectionSqrtCandidate(projectionAddCandidate(projectionAddCandidate(square(ab[1]),square(ab[2])),square(ab[0])));
 float u=projectionAddCandidate(projectionAddCandidate(square(ac[1]),square(ac[0])),square(ac[2]));
 float v=projectionAddCandidate(projectionAddCandidate(square(bd[1]),square(bd[0])),square(bd[2]));
 float t=projectionDivCandidate(projectionSubCandidate(projectionAddCandidate(v,square(length)),u),projectionMulCandidate(length,2));
 float factor=projectionDivCandidate(t,length);
 for(int k=0;k<3;++k)center[k]=projectionAddCandidate(p[0][k],projectionMulCandidate(factor,ab[k]));
 radius=projectionSqrtCandidate(projectionAddCandidate(square(t),u));
}
