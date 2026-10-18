#include <iostream>
#include "studio_shader_instance_state_candidate.h"
int main(){unsigned cases=0;
for(unsigned i=0;i<65536;++i)for(bool enabled:{false,true}){
 StudioShaderInstanceStateCandidate state;state.pendingMatrices=(i==65535)?0xffffffffu:i;state.matrixBatchEnabled=enabled;
 state.currentMatrix.fill(0xa5a5a5a5);state.lighting=uintptr_t(0x123456789000ull);state.metadata1=uintptr_t(0xabcde1234000ull);state.metadata2=state.metadata1-2;
 auto oldPending=state.pendingMatrices;unsigned calls=0;
 auto result=studioShaderInstanceStateCandidate(state,[&](auto& matrix){if(state.pendingMatrices!=oldPending)throw 1;for(unsigned n=0;n<12;++n)matrix[n]=i*977+n;++calls;});
 bool fast=oldPending&&enabled;
 if(calls!=unsigned(!fast)||state.pendingMatrices!=(fast?0:oldPending)||result.instance.scalars[2]!=(fast?oldPending+1:1)||result.instance.matrices!=reinterpret_cast<uintptr_t>(state.currentMatrix.data())||result.instance.lighting!=state.lighting||result.metadata1!=state.metadata1||result.metadata2!=state.metadata2)return 1;
 for(unsigned n=0;n<19;++n)if(n!=2&&result.instance.scalars[n])return 2;
 for(unsigned n=0;n<12;++n)if(state.currentMatrix[n]!=(fast?0xa5a5a5a5:i*977+n))return 3;++cases;
}
std::cout<<"instance_getter_cases="<<cases<<" full_width_pointers=1 matrix_copy=1 count_wrap=1 zeroed_scalars=1\n";
}
