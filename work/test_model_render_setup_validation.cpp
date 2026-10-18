#include "model_render_setup_validation_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool x){if(!x)std::abort();}
int main(){unsigned cases=0;for(unsigned flags=0;flags<65536;++flags)for(unsigned mode=0;mode<8;++mode){uint32_t words[4]={mode&1?3u:2u,mode&2?101u:100u,0xdeadbeefu,mode&4?17u:16u};auto result=modelRenderSetupValidationCandidate(uint16_t(flags),words,100,4);check(result==(mode?uint16_t(flags|4):uint16_t((flags&0xfffb)|2)));check((result&0xfff9)==(flags&0xfff9));++cases;}uint32_t words[4]={2,100,0,0xfffffffc};check(modelRenderSetupValidationCandidate(0,words,100,0xffffffff)==2);words[3]=0;check(modelRenderSetupValidationCandidate(0,words,100,0x40000000)==2);std::printf("PASS: %u setup validation states/all flags/eight predicate combinations; uint32 multiplication wrap verified. Full caller/native parity pending.\n",cases);}
