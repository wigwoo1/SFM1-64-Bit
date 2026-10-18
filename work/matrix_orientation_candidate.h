#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
// Returns quaternion x,y,z,w followed by position x,y,z.
inline std::array<uint32_t,7> matrixOrientationCandidate(const std::array<uint32_t,12>& words){
    float m[12],q[4];std::memcpy(m,words.data(),48);
    const auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    uint32_t thresholdWord=0x3f800001;float threshold;std::memcpy(&threshold,&thresholdWord,4);
    const float trace=add(add(add(m[0],m[5]),m[10]),1.0f);
    if(trace>threshold){q[0]=sub(m[9],m[6]);q[1]=sub(m[2],m[8]);q[2]=sub(m[4],m[1]);q[3]=trace;}
    else if(m[0]>m[5]&&m[0]>m[10]){q[0]=sub(sub(add(m[0],1.0f),m[5]),m[10]);q[1]=add(m[4],m[1]);q[2]=add(m[8],m[2]);q[3]=sub(m[9],m[6]);}
    else if(m[5]>m[10]){q[0]=add(m[4],m[1]);q[1]=sub(sub(add(m[5],1.0f),m[0]),m[10]);q[2]=add(m[9],m[6]);q[3]=sub(m[2],m[8]);}
    else{q[0]=add(m[8],m[2]);q[1]=add(m[9],m[6]);q[2]=sub(sub(add(m[10],1.0f),m[0]),m[5]);q[3]=sub(m[4],m[1]);}
    const float norm=add(add(add(mul(q[0],q[0]),mul(q[1],q[1])),mul(q[2],q[2])),mul(q[3],q[3]));
    if(norm!=0.0f){const auto root=_mm_sqrt_ss(_mm_set_ss(norm));const float factor=_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(1.0f),root));for(int i=3;i>=0;--i)q[i]=mul(q[i],factor);}
    std::array<uint32_t,7> result{};std::memcpy(result.data(),q,16);
    for(unsigned i=0;i<3;++i){auto word=words[i*4+3];if((word&0x7f800000u)==0x7f800000u&&(word&0x007fffffu))word|=0x00400000u;result[4+i]=word;}
    return result;
}
