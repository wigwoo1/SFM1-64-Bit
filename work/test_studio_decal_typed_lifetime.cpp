#include <memory>
#include <iostream>
#include <stdexcept>
#include "studio_decal_typed_create_candidate.h"
static void check(bool v){if(!v)throw std::runtime_error("typed lifecycle contract");}
int main(){try{unsigned cases=0;
for(int count=1;count<=128;++count)for(unsigned mutate=0;mutate<2;++mutate){StudioDecalPointerPoolCandidate pool;StudioDecalPointerNodeCandidate node;StudioDecalPointerBlockCandidate block{nullptr,1,&node};int live=count,saved=0;bool locked=false;std::unique_ptr<StudioDecalEntryCandidate[]> entries;unsigned reads=0;
auto* created=studioDecalTypedResourceCreateLiveCandidate(pool,uintptr_t(0x100000000ULL),[&]{++reads;return live;},[&]{check(!locked);locked=true;},[&]{check(locked);locked=false;},[&](auto& p,int n){check(locked&&n==1);p.first=&block;},[]{throw std::runtime_error("unexpected warning");},[&](auto&,auto*){check(locked);return false;},[&](int n){check(!locked&&n==count);saved=n;entries=std::make_unique<StudioDecalEntryCandidate[]>(n);for(int i=0;i<n;++i)entries[i].chainHead=0x1234;if(mutate)live/=2;return entries.get();});
check(created==&node&&!locked&&node.descriptor==uintptr_t(0x100000000ULL)&&node.entries==reinterpret_cast<uintptr_t>(entries.get())&&node.entryCount==live&&pool.head==&node&&pool.tail==&node&&pool.linkedCount==1);check(reads==unsigned(4+live));for(int i=0;i<saved;++i){check(entries[i].chainHead==(i<live?0xffff:0x1234));check(!entries[i].buffer.address&&!entries[i].buffer.capacity&&!entries[i].buffer.growth&&entries[i].list.pool.maximumIndex==0xffff&&!entries[i].list.pool.count);}
unsigned associations=0,deleted=0;studioDecalTypedResourceReleaseCandidate(pool,&node,[&](auto*){++associations;},[](uint16_t){throw std::runtime_error("unexpected chain");},[&](uintptr_t identity,int flags){check(!locked&&flags==3&&identity==reinterpret_cast<uintptr_t>(entries.get()));uintptr_t result=studioDecalEntryDeleteCandidate(entries.get(),saved,uint32_t(flags),identity,identity,[](uintptr_t){throw std::runtime_error("unexpected buffer");},[&](uintptr_t p){check(p==identity);entries.reset();++deleted;});check(result==identity);},[&]{check(!locked);locked=true;},[&]{check(locked);locked=false;},[&](auto&,auto*){check(locked);return false;});
check(associations==1&&deleted==1&&!entries&&!locked&&!node.entries&&node.entryCount==live&&!pool.head&&!pool.tail&&!pool.linkedCount&&pool.freeHead==&node);++cases;
}
std::cout<<"typed_lifetime_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
