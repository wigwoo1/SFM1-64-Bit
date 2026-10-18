#include "studio_shader_cull_transition_candidate.h"
#include "studio_shader_render_state_submit_candidate.h"
#include <cstdio>
#include <initializer_list>
struct SubmitOps {
 uint32_t& cache;uint32_t gate;bool queued;unsigned calls=0,flushes=0;
 uint32_t suppressed(){return gate;}
 void flush(){++flushes;queued=!queued;cache=999;}
 bool queueEnabled(){return queued;}
 void queueState(uint32_t key,uint32_t value){if(key!=22||value!=3)throw 4;calls=1;cache=888;}
 void directState(uint32_t key,uint32_t value){if(key!=22||value!=3)throw 5;calls=2;cache=777;}
};
int main(){unsigned cases=0;
 for(uint32_t input=0;input<512;++input)for(uint32_t configured=0;configured<5;++configured)for(uint32_t cached=0;cached<5;++cached)for(bool flip:{false,true}){
  StudioShaderCullTransitionStateCandidate state{configured,cached,77,flip};unsigned calls=0;
  const uint32_t requested=uint8_t(input)?configured:1;
  const uint32_t expected=flip&&requested!=1?(requested==2?3:2):requested;
  studioShaderCullTransitionCandidate(state,input,[&](uint32_t key,uint32_t value,uint32_t extra){
   if(key!=22||value!=expected||extra)throw 1;
   ++calls;state.configured=42;state.applied=43;state.enabled=44;
  });
  if(calls!=(requested==cached?0u:1u)||state.applied!=requested)return 2;
  if(state.enabled!=(calls?44:uint8_t(input)))return 3;
  ++cases;
 }
 std::printf("cull_transition_cases=%u lowbyte_input=1 cached_unflipped_mode=1 callback_mutation=1\n",cases);
 unsigned submits=0;
 for(uint32_t gate:{0u,256u,1u,257u})for(uint32_t flush:{0u,256u,1u,257u})for(bool same:{false,true})for(bool queued:{false,true}){
  uint32_t cache=same?3:2;SubmitOps ops{cache,gate,queued};
  studioShaderRenderStateSubmitCandidate(cache,22,3,flush,ops);
  const bool executes=!uint8_t(gate)&&!same;
  const bool didFlush=executes&&uint8_t(flush);
  const bool finalQueue=queued!=didFlush;
  if(ops.flushes!=(didFlush?1u:0u)||ops.calls!=(executes?(finalQueue?1u:2u):0u))return 6;
  if(cache!=(executes?3u:(same?3u:2u)))return 7;
  ++submits;
 }
 std::printf("render_state_submit_cases=%u live_queue_after_flush=1 final_cache_after_callback=1\n",submits);return 0;
}
