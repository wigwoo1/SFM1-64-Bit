#include <iostream>
#include <stdexcept>
#include "studio_material_queue_owner_candidate.h"
#include "studio_decal_resource_owner_candidate.h"
static unsigned released;
static void check(bool v){if(!v)throw std::runtime_error("virtual queue owner contract");}
static void cleanup(uintptr_t receiver,uintptr_t argument){auto& owner=*reinterpret_cast<StudioDecalResourceOwnerCandidate*>(receiver);owner.release(reinterpret_cast<StudioDecalPointerNodeCandidate*>(argument),[](auto*){},[](uint16_t){throw std::runtime_error("unexpected chain");},[](uintptr_t){throw std::runtime_error("unexpected nested buffer");});++released;}
int main(){try{StudioMaterialQueueOwnerCandidate queue;check(queue.reservedBytes()==0x800000&&queue.committedBytes()==0x40000);StudioDecalResourceOwnerCandidate owner;uintptr_t firstAddress=0;size_t committed=0;
for(unsigned round=0;round<3;++round){for(unsigned i=0;i<4096;++i){int count=1+int(i%7);auto* node=owner.create(uintptr_t(0x100000000ULL+i),[&]{return count;});uintptr_t argument=reinterpret_cast<uintptr_t>(node);studioDecalQueueCallCandidate(reinterpret_cast<uintptr_t>(&owner),&cleanup,&argument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){queue.enqueue(call);check(call->references.load()==1);});argument=0;check(queue.queuedCount()==i+1&&owner.activeArrayCount()==i+1);}
check(owner.poolState().linkedCount==4096&&queue.committedBytes()>=4096*sizeof(StudioMaterialQueueNodeCandidate));if(!round){firstAddress=reinterpret_cast<uintptr_t>(queue.queueState().head);committed=queue.committedBytes();}else check(firstAddress==reinterpret_cast<uintptr_t>(queue.queueState().head)&&queue.committedBytes()==committed);queue.drain();check(queue.queuedCount()==0&&!queue.queueState().head&&!queue.queueState().tail&&!owner.activeArrayCount()&&!owner.poolState().linkedCount);queue.drain();check(queue.committedBytes()==committed);}
check(released==12288);
unsigned invoked=0;
auto noop=[](uintptr_t receiver,uintptr_t){++*reinterpret_cast<unsigned*>(receiver);};
for(unsigned i=0;i<20000;++i){auto* call=new StudioDecalQueuedCallCandidate;call->owner=reinterpret_cast<uintptr_t>(&invoked);call->method=noop;queue.enqueue(call);}
check(queue.committedBytes()==0x50000&&queue.queuedCount()==20000);queue.drain();check(invoked==20000&&queue.queuedCount()==0&&queue.committedBytes()==0x50000);
std::cout<<"virtual_queue_releases="<<released<<" committed_bytes="<<committed<<" reused_base=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
