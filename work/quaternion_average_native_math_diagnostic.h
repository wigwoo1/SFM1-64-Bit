#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <stdexcept>
inline unsigned nativeMathMask=0;
__declspec(noinline) inline float nativeFloatMath(float input,uint32_t address){float output;
 __asm {movss xmm0,input
 mov eax,address
 call eax
 movss output,xmm0}
 return output;}
__declspec(noinline) inline double nativeAtanMath(double y,double x){double output;uint32_t address=0x10A0E901;
 __asm {movsd xmm0,y
 movsd xmm1,x
 mov eax,address
 call eax
 movsd output,xmm0}
 return output;}
inline std::array<float,4> diagnosticNativeQuaternionAverage(
    const std::vector<std::array<float,4>>& inputs,const std::vector<float>&weights) {
    if(inputs.size()!=weights.size())throw std::runtime_error("quaternion weight count mismatch");
    if(inputs.size()==1)return inputs[0];
    float sum=0;for(float w:weights)sum=w+sum;
    const float reciprocal=sum>0?1.0f/sum:1.0f;
    std::array<float,4> aggregate{};
    for(unsigned i=0;i<inputs.size();++i){auto q=inputs[i];float difference=0,total=0;
        for(unsigned c=0;c<4;++c){const float d=inputs[0][c]-q[c],s=inputs[0][c]+q[c];
            difference=difference+d*d;total=total+s*s;}
        if(difference>total)for(auto&c:q)c=-c;
        const float xx=q[0]*q[0],yy=q[1]*q[1],zz=q[2]*q[2];
        const float xy=xx+yy;const float vectorSquare=xy+zz;
        const float magnitude=std::sqrt(vectorSquare);
        const float factor=magnitude>1e-5f?float(((nativeMathMask & 16)?nativeAtanMath(double(magnitude),double(q[3])):std::atan2(double(magnitude),double(q[3]))))/magnitude:0.0f;
        const float normSquare=q[3]*q[3]+vectorSquare;
        const float logarithm=((nativeMathMask & 1)?nativeFloatMath(normSquare,0x10A13A91):float(std::log(double(normSquare))))*.5f;
        const float weight=weights[i]*reciprocal;
        for(unsigned c=0;c<3;++c){const float logComponent=q[c]*factor;aggregate[c]=logComponent*weight+aggregate[c];}
        aggregate[3]=logarithm*weight+aggregate[3];
    }
    const float yy=aggregate[1]*aggregate[1],xx=aggregate[0]*aggregate[0],zz=aggregate[2]*aggregate[2];
    const float magnitude=std::sqrt((yy+xx)+zz);
    const float exponential=((nativeMathMask & 2)?nativeFloatMath(aggregate[3],0x10A157DA):float(std::exp(double(aggregate[3]))));
    const float factor=magnitude>=1e-5f?(((nativeMathMask & 4)?nativeFloatMath(magnitude,0x10A0E7B2):float(std::sin(double(magnitude))))*exponential)/magnitude:0.0f;
    return {factor*aggregate[0],factor*aggregate[1],factor*aggregate[2],((nativeMathMask & 8)?nativeFloatMath(magnitude,0x10A0E65B):float(std::cos(double(magnitude))))*exponential};
}
