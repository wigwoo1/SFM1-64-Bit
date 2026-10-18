#pragma once
#include "quaternion_normalize.h"
#include <cmath>
#include <stdexcept>
inline std::array<float,4> interpolateRecoveredQuaternionUnaligned(const std::array<float,4>&a,std::array<float,4>b,float f){
 auto add=[](float x,float y){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(x),_mm_set_ss(y)));};
 auto sub=[](float x,float y){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(x),_mm_set_ss(y)));};
 auto mul=[](float x,float y){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(x),_mm_set_ss(y)));};
 float dot=add(mul(a[1],b[1]),mul(a[0],b[0]));dot=add(dot,mul(a[2],b[2]));dot=add(dot,mul(a[3],b[3]));
 constexpr float threshold=.000001f;
 if(!(add(1,dot)>threshold)){std::array<float,4>q{-b[1],b[0],-b[3],b[2]};float left=static_cast<float>(std::sin(static_cast<double>(sub(1,f))*1.5707963267948966)),right=static_cast<float>(std::sin(static_cast<double>(f)*1.5707963267948966));for(unsigned c=0;c<3;++c)q[c]=add(mul(q[c],right),mul(a[c],left));return q;}
 float left=sub(1,f),right=f;
 if(sub(1,dot)>threshold){float angle=std::acos(dot);float reciprocal=_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(1),_mm_set_ss(static_cast<float>(std::sin(static_cast<double>(angle))))));left=mul(static_cast<float>(std::sin(static_cast<double>(mul(left,angle)))),reciprocal);right=mul(static_cast<float>(std::sin(static_cast<double>(mul(angle,f)))),reciprocal);}
 std::array<float,4>q{};for(unsigned c=0;c<4;++c)q[c]=add(mul(b[c],right),mul(a[c],left));return q;
}

// 1049DD00 corresponds to normalize=false. Normalization is a separate caller
// operation (e.g. quaternion log playback); constraints must pass false.
inline std::array<float,4> interpolateRecoveredQuaternion(const std::array<float,4>&a,std::array<float,4>b,float f,bool normalize=true){
 auto add=[](float x,float y){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(x),_mm_set_ss(y)));};auto sub=[](float x,float y){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(x),_mm_set_ss(y)));};auto mul=[](float x,float y){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(x),_mm_set_ss(y)));};
 float difference=0,sum=0;for(unsigned c=0;c<4;++c){float d=sub(a[c],b[c]),s=add(a[c],b[c]);if(c==0){difference=mul(d,d);sum=mul(s,s);}else{difference=add(difference,mul(d,d));sum=add(sum,mul(s,s));}}
 if(difference>sum)for(auto&component:b)component=-component;
 auto q=interpolateRecoveredQuaternionUnaligned(a,b,f);return normalize?normalizeRecoveredQuaternion(q):q;
}
