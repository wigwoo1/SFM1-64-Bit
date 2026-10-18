#include "studio_shader_viewport_submit_candidate.h"
#include <cstdio>
#include <initializer_list>
struct Ops {
 StudioShaderViewportWordsCandidate& live;bool queued;unsigned calls=0;
 bool queueEnabled(){return queued;}
 void submit(const StudioShaderViewportWordsCandidate& value,unsigned kind){if(value[0]!=0xabcdef12||!value[2]||!value[3])throw 1;calls=kind;live[0]=0x12345678;live[4]=0x80000000;}
 void queueViewport(const StudioShaderViewportWordsCandidate& value){submit(value,1);}
 void directViewport(const StudioShaderViewportWordsCandidate& value){submit(value,2);}
};
int main(){unsigned cases=0;
 for(uint32_t force:{0u,256u,1u,257u})for(bool same:{false,true})for(bool zeroWidth:{false,true})for(bool zeroHeight:{false,true})for(bool queued:{false,true}){
  StudioShaderViewportWordsCandidate live{0xabcdef12,7,zeroWidth?0u:512u,zeroHeight?0u:256u,0,0x3f800000};
  auto cache=live;if(!same)cache[4]=0x80000000;const auto original=cache;
  Ops ops{live,queued};studioShaderViewportSubmitCandidate(live,cache,force,ops);
  const bool execute=(uint8_t(force)||!same)&&!zeroWidth&&!zeroHeight;
  if(ops.calls!=(execute?(queued?1u:2u):0u)||cache!=(execute?live:original))return 2;
  ++cases;
 }
 std::printf("viewport_submit_cases=%u raw_depth_bits=1 lowbyte_force=1 zero_size_skip=1 live_post_callback_cache=1\n",cases);return 0;
}
