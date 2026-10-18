#include "studio_shader_manager_restore_reset_candidate.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
struct Owner{uint32_t cached9c,cachedA0;unsigned table;};
struct Ops{
 bool mutate,replace;uint32_t initial;unsigned order=0;
 unsigned captureSlot9(Owner& owner){assert(order++==0&&owner.cached9c==initial&&owner.cachedA0==initial);return owner.table;}
 void invokeSlot9(unsigned target,Owner& owner,uint32_t argument){assert(order++==1&&target==1&&argument==0xffffffff&&owner.cached9c==0xffffffff&&owner.cachedA0==0xffffffff);if(mutate){owner.cached9c=42;owner.cachedA0=77;}if(replace)owner.table=2;}
 void invokeSlot10(Owner& owner,uint32_t argument){assert(order++==2&&argument==0xffffffff&&owner.table==(replace?2u:1u));assert(owner.cached9c==(mutate?42u:0xffffffff)&&owner.cachedA0==(mutate?77u:0xffffffff));}
};
int main(){unsigned cases=0;for(uint32_t initial:{0u,123u})for(bool mutate:{false,true})for(bool replace:{false,true}){Owner owner{initial,initial,1};Ops ops{mutate,replace,initial};studioShaderManagerRestoreResetCandidate(owner,ops);assert(ops.order==3);++cases;}std::printf("shader_manager_restore_reset_cases=%u passed\n",cases);}
