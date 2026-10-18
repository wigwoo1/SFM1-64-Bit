#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
// Supplied original-rounded trig words: yaw sin/cos, pitch sin/cos, roll sin/cos.
// This is the arithmetic stage, not an angle->basis implementation yet.
inline std::array<uint32_t,9> angleBasisProductsCandidate(const std::array<uint32_t,6>& trig){
    float t[6],out[9];std::memcpy(t,trig.data(),24);
    const auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    out[0]=mul(t[3],t[1]);out[1]=mul(t[3],t[0]);uint32_t pitchSign=trig[2]^0x80000000u;std::memcpy(out+2,&pitchSign,4);
    const float rp=mul(t[4],t[2]);
    out[3]=sub(mul(t[5],t[0]),mul(rp,t[1]));
    out[4]=sub(mul(mul(t[5],-1.0f),t[1]),mul(rp,t[0]));out[5]=mul(mul(t[4],-1.0f),t[3]);
    const float cp=mul(t[5],t[2]);out[6]=add(mul(cp,t[1]),mul(t[4],t[0]));out[7]=sub(mul(cp,t[0]),mul(t[4],t[1]));out[8]=mul(t[5],t[3]);
    std::array<uint32_t,9> result{};std::memcpy(result.data(),out,36);return result;
}
