#include <cstdio>
#include "studio_material_queue_owner_candidate.h"
static unsigned invoked=0;
static void callback(uintptr_t,uintptr_t){++invoked;}
int main(){StudioMaterialQueueOwnerCandidate queue(0x10000,0x10000);unsigned accepted=0;bool full=false;unsigned failedReferences=0;
 for(unsigned i=0;i<5000;++i){auto* call=new StudioDecalQueuedCallCandidate;call->method=callback;try{queue.enqueue(call);++accepted;}catch(const std::bad_alloc&){full=true;failedReferences=call->references.load();for(unsigned n=0;n<failedReferences;++n)call->releaseReference([](auto* p){delete p;});break;}}
 auto pending=queue.queuedCount();queue.drain();if(!full||failedReferences!=1||pending!=accepted||invoked!=accepted||queue.queuedCount()){std::printf("plain_queue_failure full=%u references=%u accepted=%u invoked=%u\n",full,failedReferences,accepted,invoked);return 2;}
 auto* call=new StudioDecalQueuedCallCandidate;call->method=callback;queue.enqueue(call);queue.drain();if(invoked!=accepted+1)return 3;
 std::printf("plain_material_queue_allocation_failure_rollback=passed accepted=%u arena_reuse=passed\n",accepted);
}
