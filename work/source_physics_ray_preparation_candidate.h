#pragma once
#include "angle_basis_candidate.h"
#include <cmath>
struct SourcePhysicsRayPreparationCandidate {
    // Original27310 ray fields; start/end/raw delta/unit delta/length/maximum/
    // inverse length/reserved. Separate from Source Ray_t's swept flag.
    std::array<float,16>ray{};
    std::array<float,9>rotation{}; // 3 columns, 3 components; original01120.
    std::array<float,3>localStart{},localDelta{},localDirection{},localMidpoint{};
    float lengthMetres=0,minimum=0,maximum=0,radius=0,tolerance=0;
};
// Point-ray preparation used by original full TraceBox before compact-tree/GJK
// intersection. Original units and float/x87 arithmetic are intentional. This
// does not decide collision, contents, surface normal or inside-solid state.
inline SourcePhysicsRayPreparationCandidate sourcePhysicsRayPreparationCandidate(
 const std::array<float,3>&start,const std::array<float,3>&delta,
 const std::array<float,3>&origin,const std::array<float,3>&angles){
    for(const auto&v:{start,delta,origin,angles})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite physics ray input");
    SourcePhysicsRayPreparationCandidate result;
    std::array<uint32_t,3>angleWords{};std::memcpy(angleWords.data(),angles.data(),12);const auto basisWords=angleBasisCandidate(angleWords);std::array<float,9>basis{};std::memcpy(basis.data(),basisWords.data(),36);
    // original01120 reorders AngleVectors into IVP axes, with explicit signs.
    auto&m=result.rotation;m={basis[0],-basis[6],-basis[3],-basis[2],basis[8],basis[5],basis[1],-basis[7],-basis[4]};
    auto&r=result.ray;for(size_t i=0;i<3;++i){r[i]=start[i]+(-origin[i]);r[3+i]=r[i]+delta[i];r[6+i]=delta[i];r[9+i]=delta[i];}
    const double xx=double(delta[0])*delta[0],yy=double(delta[1])*delta[1],zz=double(delta[2])*delta[2];r[12]=float(std::sqrt((xx+yy)+zz));
    if(r[12]>0){r[14]=1.f/r[12];for(size_t i=0;i<3;++i)r[9+i]=r[14]*r[9+i];}r[13]=r[12];
    constexpr float metres=0.02539999969303608f;result.lengthMetres=r[12]*metres;
    // Original inverse-point helper adds Y before X; inverse-vector helper
    // adds X before Y. Neither expression may be contracted into an FMA.
    const std::array<float,3>p{r[0]*metres-0.0f,(-(r[2]*metres))-(-0.0f),r[1]*metres-0.0f};
    const std::array<float,3>d{r[6]*metres,-(r[8]*metres),r[7]*metres};
    for(size_t i=0;i<3;++i){result.localStart[i]=(m[3+i]*p[1]+m[i]*p[0])+m[6+i]*p[2];result.localDelta[i]=(m[i]*d[0]+m[3+i]*d[1])+m[6+i]*d[2];}
    result.localDirection=result.localDelta;
    const float squaredX=result.localDelta[0]*result.localDelta[0],squaredY=result.localDelta[1]*result.localDelta[1],squaredZ=result.localDelta[2]*result.localDelta[2];const float squared=(squaredX+squaredY)+squaredZ;
    if(squared>=1.000000013351432e-10f){const double inverse=1.0/std::sqrt(double(squared));for(auto&v:result.localDirection)v=float(double(v)*inverse);}
    for(size_t i=0;i<3;++i)result.localMidpoint[i]=result.localDelta[i]*.5f+result.localStart[i];
    result.maximum=r[13]>9.99999993922529e-09f?r[13]:9.99999993922529e-09f;result.tolerance=.03125f;
    for(float v:r)if(!std::isfinite(v))throw std::runtime_error("physics ray preparation overflow");
    for(const auto&v:{result.localStart,result.localDelta,result.localDirection,result.localMidpoint})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("physics local ray overflow");
    if(!std::isfinite(result.lengthMetres)||!std::isfinite(squared))throw std::runtime_error("physics distance preparation overflow");return result;
}
inline std::array<float,42> sourcePhysicsRayPreparationWordsCandidate(const SourcePhysicsRayPreparationCandidate&r){
    std::array<float,42>out{};size_t at=0;for(float x:r.ray)out[at++]=x;
    for(const auto&v:{r.localMidpoint,r.localStart,r.localDirection,r.localDelta})for(float x:v)out[at++]=x;
    out[at++]=r.lengthMetres;for(float x:{r.minimum,r.maximum,r.radius,r.tolerance})out[at++]=x;for(float x:r.rotation)out[at++]=x;return out;
}
