#pragma once
#include "material_proxy_lifetime_candidate.h"
#include "material_vector_component_candidate.h"
#include "material_matrix_value_write_candidate.h"

struct MaterialProxyWriteCandidate {
    uint32_t index=0,operation=0,count=0;
    std::array<uint32_t,4>arguments{};
    // Operation9 carries all16 matrix values. Existing scalar wire fields and
    // operations0..8 are unchanged; matrix-aware consumers must read payload.
    std::optional<MaterialMatrixMathCandidate>matrix;
};
struct MaterialProxyEvaluationAttemptCandidate {
    bool evaluated=false;
    std::vector<MaterialProxyWriteCandidate>writes;
};
// Explicit per-evaluation clock snapshot. Absence is not time zero or wall time.
struct MaterialProxyEvaluationInputsCandidate {
    std::optional<uint32_t>currentTimeBits;
    // Explicit snapshot of the material system's shared mutable missing-var
    // float cache. Absence is unknown, not four invented zeroes. The owner
    // supplies one consistent snapshot across materials for this evaluation.
    std::optional<std::array<uint32_t,4>>sharedMissingVariableCacheBits;
};
// A per-call write plan. Reads see earlier writes in declaration order, while
// public binding reads still see the committed state. Only the store can
// construct it, and only a fully successful frame is published.
class MaterialProxyEvaluationCandidate {
    std::shared_ptr<MaterialProxyBindingCandidate::State>origin;
    MaterialObjectParameterStateCandidate&plan;
    std::vector<MaterialProxyWriteCandidate>journal;
    MaterialProxyEvaluationInputsCandidate inputs;
    friend class MaterialObjectStoreCandidate;
    MaterialProxyEvaluationCandidate(const MaterialProxyListCandidate&list,MaterialObjectParameterStateCandidate&value,const MaterialProxyEvaluationInputsCandidate&context):origin(list.state),plan(value),inputs(context){}
    bool accepts(const MaterialProxyBindingCandidate&b)const{
        auto s=b.state.lock();return origin&&s==origin&&s->parameters&&b.index<plan.material.parameters.values.size();
    }
    bool write(const MaterialProxyBindingCandidate&b,uint32_t operation,std::array<uint32_t,4>args,uint32_t count){
        if(!accepts(b))return false;
        auto next=plan.material.parameters.values[b.index];
        if(!materialTypedNumericWriteCandidate(next,operation,args,count))return false;
        journal.push_back({uint32_t(b.index),operation,count,args});
        plan.material.parameters.values[b.index]=std::move(next);return true;
    }
public:
    std::optional<uint32_t>currentTime()const noexcept{return inputs.currentTimeBits;}
    std::optional<std::array<uint32_t,4>>sharedMissingVariableCache()const noexcept{return inputs.sharedMissingVariableCacheBits;}
    MaterialProxyEvaluationCandidate(const MaterialProxyEvaluationCandidate&)=delete;
    MaterialProxyEvaluationCandidate&operator=(const MaterialProxyEvaluationCandidate&)=delete;
    MaterialProxyEvaluationCandidate(MaterialProxyEvaluationCandidate&&)=delete;
    MaterialProxyEvaluationCandidate&operator=(MaterialProxyEvaluationCandidate&&)=delete;
    std::optional<MaterialTypedDispatchCandidate>read(const MaterialProxyBindingCandidate&b)const{
        if(!accepts(b))return {};return plan.material.parameters.values[b.index];
    }
    bool setFloat(const MaterialProxyBindingCandidate&b,uint32_t bits){return write(b,1,{bits,0,0,0},0);}
    bool setInt(const MaterialProxyBindingCandidate&b,uint32_t value){return write(b,0,{value,0,0,0},0);}
    bool setVector(const MaterialProxyBindingCandidate&b,std::array<uint32_t,4>bits,uint32_t count){if(count<1||count>4)return false;for(size_t i=count;i<4;++i)bits[i]=0;return write(b,5,bits,count);}
    bool setComponent(const MaterialProxyBindingCandidate&b,uint32_t index,uint32_t bits){return write(b,8,{bits,0,0,0},index);}
    bool setMatrix(const MaterialProxyBindingCandidate&b,const MaterialMatrixMathCandidate&matrix){
        if(!accepts(b))return false;auto next=plan.material.parameters.values[b.index];
        if(!materialTypedMatrixSetCandidate(next,matrix))return false;
        MaterialProxyWriteCandidate event;event.index=uint32_t(b.index);event.operation=9;event.count=16;event.matrix=matrix;
        journal.push_back(std::move(event));plan.material.parameters.values[b.index]=std::move(next);return true;
    }
    std::optional<MaterialMatrixMathCandidate>readMatrix(const MaterialProxyBindingCandidate&b)const{
        auto value=read(b);return value?materialTypedMatrixReadCandidate(*value):std::optional<MaterialMatrixMathCandidate>{};
    }
};

inline bool materialProxySetResultFloatCandidate(MaterialProxyEvaluationCandidate&frame,
        const MaterialProxyBindingCandidate&result,int32_t component,uint32_t bits){
    auto out=frame.read(result);if(!out)return false;
    if((out->value.numeric.typeAndMetadata&15)==2){
        if(component>=0)return frame.setComponent(result,uint32_t(component),bits);
        const auto count=(out->value.numeric.typeAndMetadata>>4)&7u;
        return frame.setVector(result,{bits,bits,bits,bits},count);
    }
    return frame.setFloat(result,bits);
}
