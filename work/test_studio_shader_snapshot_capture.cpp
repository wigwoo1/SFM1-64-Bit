#include <iostream>
#include <vector>
#include "studio_shader_snapshot_capture_candidate.h"
struct Ops {
 StudioShaderRuntimePassStateCandidate& state;std::array<StudioShaderSnapshotEntryCandidate,2> entries;
 int32_t level;uint32_t handle;unsigned initial,newCount;bool mutate;std::vector<unsigned> calls;
 int32_t hardwareLevel(){calls.push_back(1);return level;}
 void sampler21(uint32_t index,uint32_t enabled){if(index!=15||enabled!=1)throw 1;calls.push_back(21);}
 void sampler20(uint32_t index,uint32_t enabled){if(index!=15||enabled!=1)throw 2;calls.push_back(20);}
 StudioShaderSnapshotEntryCandidate& entry(uint8_t snapshot){if(snapshot!=0)throw 3;calls.push_back(2);return entries[0];}
 uint32_t capture(){calls.push_back(78);if(mutate){state.snapshot=1;entries[0].passCount=newCount;}return handle;}
};
int main(){unsigned cases=0;for(int32_t level:{-1,0,91,92,100})for(unsigned count=0;count<4;++count)for(unsigned updated=0;updated<4;++updated)for(bool mutate:{false,true})for(unsigned bits=0;bits<256;++bits){
 StudioShaderRuntimePassStateCandidate state;uint32_t handle=bits*0x1020304u;Ops ops{state,{},level,handle,count,updated,mutate,{}};
 for(auto& e:ops.entries){e.passCount=count;e.handles.fill(0xa5a5);}
 studioShaderSnapshotCaptureCandidate(state,ops);auto index=mutate?updated:count;
 auto expected=level>=92?std::vector<unsigned>{1,21,20,2,78}:std::vector<unsigned>{1,2,78};
 if(ops.calls!=expected||ops.entries[0].passCount!=index+1||ops.entries[1].passCount!=count||state.snapshot!=unsigned(mutate))return 1;
 for(unsigned n=0;n<4;++n)if(ops.entries[0].handles[n]!=(n==index?uint16_t(handle):0xa5a5)||ops.entries[1].handles[n]!=0xa5a5)return 2;++cases;
}std::cout<<"snapshot_capture_cases="<<cases<<" retained_entry=1 live_count_reload=1 handle_truncation=1 hardware_gate=1\n";}
