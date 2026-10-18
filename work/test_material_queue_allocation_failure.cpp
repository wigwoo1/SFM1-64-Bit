#include <cstdio>
#include "studio_material_mixed_queue_owner_candidate.h"
static size_t invoked=0;
static void zero(uintptr_t){++invoked;}
static void decal(uintptr_t,uintptr_t){++invoked;}
int main(){StudioMaterialMixedQueueOwnerCandidate queue;size_t accepted=0;bool full=false;
 for(size_t i=0;i<300000;++i){try{queue.enqueueZero(0,zero);++accepted;}catch(const std::bad_alloc&){full=true;break;}}
 if(!full)return 1;
 const auto pending=queue.queuedCount();auto* call=new StudioDecalQueuedCallCandidate;call->method=decal;bool rejected=false;
 try{queue.enqueue(call);}catch(const std::bad_alloc&){rejected=true;}
 const unsigned references=call->references.load();
 if(rejected){for(unsigned n=0;n<references;++n)call->releaseReference([](auto* object){delete object;});}
 queue.drain();
 const bool valid=rejected&&references==1&&pending==accepted&&invoked==accepted&&queue.queuedCount()==0;
 if(!valid){std::printf("failed_enqueue_rejected=%u caller_references=%u accepted=%zu invoked=%zu\n",rejected,references,accepted,invoked);return 2;}
 queue.enqueueZero(0,zero);queue.drain();if(invoked!=accepted+1)return 3;
 std::puts("material_queue_capacity_failure_reference_rollback_fifo_and_reuse=passed");
}
