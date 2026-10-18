#include <cstdio>
#include "studio_decal_release_dispatch_candidate.h"
#include "studio_material_queued_context_control_candidate.h"
static void noop(uintptr_t){}
int main(){StudioMaterialQueuedContextControlCandidate context(0,noop,noop);bool full=false;
 for(unsigned i=0;i<200000;++i){try{context.queue().enqueueZero(0,noop);}catch(const std::bad_alloc&){full=true;break;}}
 if(!full)return 1;unsigned later=0,releases=0;bool rejected=false;auto identity=reinterpret_cast<uintptr_t>(&context);
 try{studioDecalReleaseDispatchCandidate(0,[&]{context.addReference();return identity;},[&](uintptr_t){context.begin();},[&](uintptr_t){++later;return uintptr_t(0);},[&]{++later;return false;},[&]{++later;return uintptr_t(0);},[&](uintptr_t,uintptr_t,const uintptr_t*){++later;},[&](uintptr_t,uintptr_t){++later;},[&](uintptr_t){++later;context.end();},[&](uintptr_t){++releases;context.releaseReference([](auto*){std::abort();});});}catch(const std::bad_alloc&){rejected=true;}
 auto refs=context.referenceCount();context.flush();if(!rejected||later||releases!=1||refs!=1||context.nestingCount()){std::printf("dispatch_enter_failure later=%u releases=%u references=%u\n",later,releases,refs);return 2;}
 context.begin();context.end();context.flush();if(context.referenceCount()!=1||context.nestingCount())return 3;std::puts("decal_dispatch_failed_context_enter_reference_balance_and_reuse=passed");
}
