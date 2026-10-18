#pragma once
#include <array>
#include <cstdint>
#include "studio_shader_instance_matrix_candidate.h"
struct StudioShaderInstanceStateCandidate {
 uint32_t pendingMatrices=0;bool matrixBatchEnabled=false;
 std::array<uint32_t,12> currentMatrix{};
 uintptr_t lighting=0,metadata1=0,metadata2=0;
};
struct StudioShaderInstanceCandidate {
 // Zeroed scalar slots of native76-byte record; pointer slots4/6 reserved.
 // Full-width pointers are separate: this is a logical adapter, not public ABI.
 std::array<uint32_t,19> scalars{};uintptr_t matrices=0,lighting=0;
};
struct StudioShaderInstanceResultCandidate {StudioShaderInstanceCandidate instance;uintptr_t metadata1=0,metadata2=0;};
// Matrix callback encompasses native matrix-service query+conversion helper.
// Callers retain state and its matrix storage through renderer consumption.
template<class Matrix>StudioShaderInstanceResultCandidate studioShaderInstanceStateCandidate(StudioShaderInstanceStateCandidate& state,Matrix matrix){
 StudioShaderInstanceResultCandidate result;result.instance.lighting=state.lighting;
 if(state.pendingMatrices&&state.matrixBatchEnabled){result.instance.scalars[2]=state.pendingMatrices+uint32_t(1);state.pendingMatrices=0;}
 else{std::array<uint32_t,12> converted;matrix(converted);state.currentMatrix=converted;result.instance.scalars[2]=1;}
 result.instance.matrices=reinterpret_cast<uintptr_t>(state.currentMatrix.data());result.metadata1=state.metadata1;result.metadata2=state.metadata2;return result;
}
inline StudioShaderInstanceResultCandidate studioShaderInstanceStateFromMatrixCandidate(StudioShaderInstanceStateCandidate& state,const std::array<uint32_t,16>& source,bool integerPath=false){
 return studioShaderInstanceStateCandidate(state,[&](auto& converted){converted=studioShaderInstanceMatrixCandidate(source,integerPath);});
}
