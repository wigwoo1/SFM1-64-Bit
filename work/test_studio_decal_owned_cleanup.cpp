#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include "studio_decal_chain_lists_candidate.h"
#include "studio_decal_index_unlink_candidate.h"
#include "studio_decal_resource_release_candidate.h"
#include "studio_decal_association_release_candidate.h"
#include "studio_decal_entry_delete_candidate.h"
static void check(bool v){if(!v)throw std::runtime_error("owned cleanup contract");}
struct Allocation {
 std::unique_ptr<unsigned char[]> bytes;std::vector<EngineShadowPoolLinkCandidate> links;
 StudioDecalChainListCandidate list;bool external=false,isList=false,released=false;
};
int main(){try{unsigned cases=0,totalFrees=0;
for(unsigned length=1;length<=64;++length)for(unsigned mask=0;mask<8;++mask){
 std::vector<StudioDecalChainNodeCandidate> nodes(length);std::vector<std::unique_ptr<Allocation>> allocations;std::unordered_map<uintptr_t,Allocation*> registry;
 auto allocate=[&](unsigned count,unsigned stride,bool external,bool isList){auto a=std::make_unique<Allocation>();a->bytes=std::make_unique<unsigned char[]>(count*stride);std::memset(a->bytes.get(),0xa5,count*stride);a->external=external;a->isList=isList;if(isList){a->links.resize(count);for(unsigned i=0;i<count;++i)a->links[i]={i?uint16_t(i-1):uint16_t(0xffff),i+1<count?uint16_t(i+1):uint16_t(0xffff)};a->list.pool={a->links.data(),int32_t(count),0,uint16_t(count-1),uint16_t(count),uint16_t(count-1)};}uintptr_t address=reinterpret_cast<uintptr_t>(a->bytes.get());registry[address]=a.get();allocations.push_back(std::move(a));return address;};
 for(unsigned i=0;i<length;++i){auto& n=nodes[i];n.next=i+1<length?uint16_t(i+1):uint16_t(0xffff);n.contribution=1;n.array={allocate(7,4,bool(mask&1),false),7,(mask&1)?-1:0};n.list48={allocate(1+i%5,48,bool(mask&2),true),int32_t(1+i%5),(mask&2)?-1:0};n.list24={allocate(1+i%7,24,bool(mask&4),true),int32_t(1+i%7),(mask&4)?-1:0};n.arrayCount=7;}
 std::vector<EngineShadowPoolLinkCandidate> innerLinks(length);for(unsigned i=0;i<length;++i)innerLinks[i]={i?uint16_t(i-1):uint16_t(0xffff),nodes[i].next};EngineShadowPoolStateCandidate innerPool{innerLinks.data(),int32_t(length),0,uint16_t(length-1),uint16_t(length),uint16_t(length-1)};
 uint16_t innerFree=0xffff;uint32_t counter=length;StudioDecalPointerPoolCandidate pool;StudioDecalPointerNodeCandidate outer;auto entries=std::make_unique<StudioDecalEntryCandidate[]>(1);int32_t savedCount=1;outer.descriptor=123;outer.entries=reinterpret_cast<uintptr_t>(entries.get());outer.entryCount=1;outer.previous=outer.next=nullptr;pool.head=pool.tail=&outer;pool.linkedCount=1;unsigned associations=0,unlinks=0,frees=0;bool locked=false;
 auto& entryState=entries[0];entryState.chainHead=0;entryState.buffer={allocate(3,12,bool(mask&4),true),3,(mask&4)?-1:0};auto* entryAllocation=registry.at(entryState.buffer.address);entryState.list=entryAllocation->list;
 StudioDecalAssociationNodeCandidate associationNodes[4];StudioDecalAssociationPoolCandidate associationPool{associationNodes,associationNodes+3,nullptr,4};for(unsigned i=0;i<4;++i){associationNodes[i].resource=i==1?456:123;associationNodes[i].previous=i?associationNodes+i-1:nullptr;associationNodes[i].next=i<3?associationNodes+i+1:nullptr;}
 auto resolve=[&](auto& buffer)->StudioDecalChainListCandidate&{auto* a=registry.at(buffer.address);check(a->isList&&!a->released);return a->list;};
 auto freeBuffer=[&](uintptr_t address){check(associationPool.count==1&&associationPool.head==associationNodes+1&&associationPool.tail==associationNodes+1);auto* a=registry.at(address);check(!a->released&&!a->external);if(a->isList){check(a->list.pool.count==0&&a->list.pool.head==0xffff&&a->list.pool.tail==0xffff);for(unsigned j=0;j<a->links.size();++j)check(a->links[j].previous==j&&a->links[j].next==(j+1<a->links.size()?uint16_t(j+1):uint16_t(0xffff)));}a->bytes.reset();a->released=true;++frees;};
 studioDecalTypedResourceReleaseCandidate(pool,&outer,[&](auto*){++associations;studioDecalAssociationReleaseCandidate(associationPool,outer.descriptor,[](auto&,auto*){return false;});},[&](uint16_t head){studioDecalChainReleaseWithListsCandidate(head,innerFree,counter,[&](uint16_t i){return &nodes[i];},[&](uint16_t i){check(i==unlinks++);studioDecalChainIndexUnlinkCandidate(innerPool,i,[&](uint16_t j){return &nodes[j];});},resolve,freeBuffer);},[&](uintptr_t p,int kind){uintptr_t base=reinterpret_cast<uintptr_t>(entries.get());check(p==base&&kind==3&&!locked);int32_t capturedCount=savedCount;check(innerPool.count==0&&counter==0);uintptr_t returned=studioDecalEntryDeleteCandidate(&entryState,int32_t(capturedCount),uint32_t(kind),p,base,[&](uintptr_t address){entryAllocation->list=entryState.list;freeBuffer(address);},[&](uintptr_t address){check(address==base);entryAllocation->list=entryState.list;check(entryState.list.pool.count==0&&entryState.chainHead==0);entries.reset();});check(returned==base);},[&]{locked=true;},[&]{locked=false;},[&](auto&,auto*){check(locked);return false;});
 check(innerPool.count==0&&innerPool.head==0xffff&&innerPool.tail==0xffff);check(associations==1&&unlinks==length&&counter==0&&innerFree==length-1&&!entries&&!locked&&!pool.head&&!pool.tail&&pool.linkedCount==0&&pool.freeHead==&outer&&outer.entries==0&&outer.entryCount==1);unsigned per=3-unsigned(bool(mask&1))-unsigned(bool(mask&2))-unsigned(bool(mask&4));check(frees==per*length+unsigned(!(mask&4)));
 for(auto& a:allocations){check(a->released!=a->external);if(a->external){check(bool(a->bytes));if(a->isList)check(a->list.pool.count==0);}}
 for(unsigned i=0;i<length;++i)check(nodes[i].previous==i&&nodes[i].next==(i?uint16_t(i-1):uint16_t(0xffff)));
 totalFrees+=frees;++cases;
}
std::cout<<"owned_cleanup_cases="<<cases<<" actual_buffer_frees="<<totalFrees<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
