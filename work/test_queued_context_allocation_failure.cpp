#include <initializer_list>
#include <cstdio>
#include "studio_material_queued_context_control_candidate.h"
static unsigned begins=0,ends=0;
static void begin(uintptr_t){++begins;}static void end(uintptr_t){++ends;}static void noop(uintptr_t){}
int main(){for(bool ending:{false,true}){StudioMaterialQueuedContextControlCandidate context(0,begin,end);if(ending)context.begin();bool full=false;
 for(unsigned i=0;i<200000;++i){try{context.queue().enqueueZero(0,noop);}catch(const std::bad_alloc&){full=true;break;}}
 if(!full)return 1;const auto before=context.nestingCount();bool rejected=false;try{if(ending)context.end();else context.begin();}catch(const std::bad_alloc&){rejected=true;}
 const auto after=context.nestingCount();context.flush();if(!rejected||before!=after){std::printf("context_enqueue_failure ending=%u before=%u after=%u\n",ending,before,after);return 2;}
 if(ending)context.end();else{context.begin();context.end();}context.flush();if(context.nestingCount()||context.queue().queuedCount())return 3;
 }if(begins!=2||ends!=2)return 4;std::puts("queued_context_begin_end_allocation_failure_depth_rollback_and_reuse=passed");}

