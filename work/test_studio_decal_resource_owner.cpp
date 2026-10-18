#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_resource_owner_candidate.h"
static void check(bool v){if(!v)throw std::runtime_error("resource owner contract");}
int main(){try{StudioDecalResourceOwnerCandidate owner;std::vector<StudioDecalPointerNodeCandidate*> nodes(1024);unsigned releases=0;
for(unsigned round=0;round<3;++round){size_t priorBlocks=owner.blockCount();for(unsigned i=0;i<nodes.size();++i){int count=1+int(i%17);auto* n=owner.create(uintptr_t(0x100000000ULL+i),[&]{return count;});check(n&&n->entryCount==count&&n->descriptor==uintptr_t(0x100000000ULL+i));nodes[i]=n;auto* entries=reinterpret_cast<StudioDecalEntryCandidate*>(n->entries);for(int j=0;j<count;++j)check(entries[j].chainHead==0xffff&&entries[j].list.pool.maximumIndex==0xffff);check(owner.activeArrayCount()==i+1&&owner.poolState().linkedCount==i+1);}if(round)check(owner.blockCount()==priorBlocks);
for(unsigned parity=0;parity<2;++parity)for(unsigned i=parity;i<nodes.size();i+=2){auto* n=nodes[i];bool observed=false;owner.release(n,[&](auto* supplied){check(supplied==n&&n->entries);observed=true;},[](uint16_t){throw std::runtime_error("unexpected chain");},[](uintptr_t){throw std::runtime_error("unexpected nested buffer");});check(observed&&!n->entries&&n->entryCount==1+int(i%17));++releases;}
check(owner.activeArrayCount()==0&&!owner.poolState().head&&!owner.poolState().tail&&!owner.poolState().linkedCount);auto* free=owner.poolState().freeHead;unsigned freeCount=0;while(free){check(free->previous==free);free=free->next;check(++freeCount<=nodes.size());}check(freeCount==nodes.size());}
check(owner.poolState().freshCount==1024);std::cout<<"owner_resources=1024 lifecycle_releases="<<releases<<" blocks="<<owner.blockCount()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
