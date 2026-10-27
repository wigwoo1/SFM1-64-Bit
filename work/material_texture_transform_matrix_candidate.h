#pragma once
#include "material_matrix_math_candidate.h"
#include <optional>
// Cached inputs, before the original GetVec/GetFloat x87 return conversion.
// Absence means no configured variable; it is distinct from a zero-valued
// variable or an explicitly supplied missing-variable cache snapshot.
struct MaterialTextureTransformInputsCandidate {
    std::optional<std::array<uint32_t,2>>center,scale,translation;
    std::optional<uint32_t>rotation;
};
inline uint32_t materialTextureTransformReadCandidate(uint32_t bits){
    return (bits&0x7fffffffu)>0x7f800000u?bits|0x400000u:bits;
}
inline MaterialMatrixMathCandidate materialTextureTransformMatrixCandidate(const MaterialTextureTransformInputsCandidate&input,bool trigSse2=false){
    const auto read=[](uint32_t b){return materialTextureTransformReadCandidate(b);};
    std::array<uint32_t,2>center{{0x3f000000,0x3f000000}};
    if(input.center)for(size_t i=0;i<2;++i)center[i]=read((*input.center)[i]);
    auto out=materialMatrixTranslateCandidate(center[0]^0x80000000u,center[1]^0x80000000u,0);
    if(input.scale){auto m=materialMatrixScaleMathCandidate(read((*input.scale)[0]),read((*input.scale)[1]),0x3f800000);materialMatrixMultiplyCandidate(m,out,out);}
    if(input.rotation){auto m=materialMatrixRotateCandidate(read(*input.rotation),trigSse2);materialMatrixMultiplyCandidate(m,out,out);}
    // The real OnBind uses two distinct products, not center+translation
    // followed by one product as in the VMT construction helper.
    auto m=materialMatrixTranslateCandidate(center[0],center[1],0);materialMatrixMultiplyCandidate(m,out,out);
    if(input.translation){m=materialMatrixTranslateCandidate(read((*input.translation)[0]),read((*input.translation)[1]),0);materialMatrixMultiplyCandidate(m,out,out);}
    return out;
}
