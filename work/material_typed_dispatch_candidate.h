#pragma once
#include "material_value_construction_candidate.h"
#include "material_matrix_scan_candidate.h"
#include "material_matrix_math_candidate.h"
#include "material_vector_literal_candidate.h"
#include <optional>
#include <algorithm>

// Isolated value-owned analogue of the KeyValues-to-CMaterialVar dispatcher.
// Name symbols, pooling/allocator identity and live material ownership are not
// adopted into the x64 representation. Callers explicitly choose CPU states.
struct MaterialTypedDispatchCandidate {
    bool created=false,matrixIdentity=false;
    uint32_t warnings=0;
    MaterialStringValueCandidate value;
    std::optional<MaterialMatrixMathCandidate> matrix;
    std::string name;
};
inline bool materialTypedDispatchCandidate(MaterialTypedDispatchCandidate&out,
        uint32_t keyType,uint32_t scalar,std::string_view name,std::string_view input,
        bool numericSse2,bool trigSse2,bool poolAvailable=true) {
    if(keyType>255||name.size()>1023||input.size()>4095)return false;
    name=name.substr(0,name.find('\0'));input=input.substr(0,input.find('\0'));
    const auto question=name.find('?');if(question!=std::string_view::npos)name.remove_prefix(question+1);
    MaterialTypedDispatchCandidate result;
    auto finish=[&](){
        if(result.created){
            result.name.assign(name);
            for(auto&c:result.name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
            // Low16 carries type/count/queue metadata; symbol IDs have their
            // own representation and are deliberately not packed into it.
            result.value.numeric.typeAndMetadata&=0xffffu;
        }
        out=std::move(result);return true;
    };
    auto construct=[&](MaterialValueConstructionKindCandidate kind,
                       const std::array<uint32_t,4>&args=std::array<uint32_t,4>{},
                       uint32_t count=0,std::string_view text=std::string_view{}){
        if(!poolAvailable)return;
        result.created=materialValueConstructCandidate(result.value,kind,args,count,text,numericSse2);
    };
    if(keyType==2){construct(MaterialValueConstructionKindCandidate::Integer,{scalar,0,0,0});return finish();}
    if(keyType==3){
        // The actual KeyValues float getter returns through x87 before the
        // factory call, quieting signaling NaNs without discarding payload.
        if((scalar&0x7fffffffu)>0x7f800000u)scalar|=0x00400000u;
        construct(MaterialValueConstructionKindCandidate::Float,{scalar,0,0,0});return finish();
    }
    if(keyType!=1||input.empty())return finish();
    MaterialMatrixMathCandidate matrix{};
    bool parsed=materialMatrixScanCandidate(input,false,matrix)==16;
    if(!parsed){
        std::array<uint32_t,16>scratch{};
        if(materialMatrixScanCandidate(input,true,scratch)==7){
            std::array<uint32_t,7>parameters{};std::copy_n(scratch.begin(),7,parameters.begin());
            matrix=materialMatrixTransformCandidate(parameters,trigSse2);parsed=true;
        }
    }
    if(parsed&&poolAvailable){
        result.created=true;result.matrix=matrix;result.matrixIdentity=true;
        result.value.numeric.typeAndMetadata=0xff47u;
        for(unsigned i=0;i<16;++i){
            const float expected=i%5==0?1.0f:0.0f;
            if(materialMatrixFloatCandidate(matrix[i])!=expected)result.matrixIdentity=false;
        }
        return finish();
    }
    // A failed matrix allocation returns null to the dispatcher, which still
    // attempts its normal vector/string fallback. Preserve possible warnings.
    if(materialVectorLiteralStartCandidate(input)){
        MaterialVectorLiteralCandidate vector;
        if(!materialVectorLiteralCandidate(input,vector))return false;
        result.warnings=vector.warnings;
        if(vector.count)construct(MaterialValueConstructionKindCandidate::Vector,vector.words,vector.count);
    }else construct(MaterialValueConstructionKindCandidate::String,{},0,input);
    return finish();
}
