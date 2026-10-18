#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_restore_owner_gate_candidate.h"
#include "studio_sampler_submit_candidate.h"
#include "studio_device_queue_refill_candidate.h"
#include "studio_device_queue_publish_candidate.h"
#include "studio_device_queue_acquire_candidate.h"
static void check(bool b){if(!b)throw std::runtime_error("submission comparison mismatch");}
struct Ops{
 std::array<unsigned,432>& owner;std::array<unsigned,16>& queue;unsigned directCalls=0,stage=0,value=0;
 unsigned suppressed(){return studioRestoreOwnerGateCandidate(owner[0xa8/4],reinterpret_cast<unsigned char*>(owner.data())[0xac],owner[0x2cc/4]);}
 bool queueEnabled(){return owner[0x658/4]!=0;}
 void queueSampler(const std::array<unsigned,4>& words){check(owner[0x664/4]>=4);auto offset=owner[0x660/4]/4;check(offset+4<=queue.size());for(unsigned i=0;i<4;++i)queue[offset+i]=words[i];owner[0x660/4]+=16;owner[0x664/4]-=4;}
 void directSampler(unsigned s,unsigned type,unsigned v){check(s==stage&&type==1&&v==value);++directCalls;}
};
struct RefillOps{
 StudioDeviceQueueStateCandidate& state;std::array<unsigned,16>& previous;std::array<unsigned,4096>& replacement;unsigned publishes=0,acquires=0;StudioDeviceQueuePublishCandidate* publication=nullptr;std::array<unsigned*,500>* pool=nullptr;unsigned waits=0;bool releaseBlock=false;
 unsigned suppressed(){return 0;}bool queueEnabled(){return true;}
 void waitZero(){check(publication!=nullptr);++waits;check(waits==1);publication->completed=publication->produced-499u;if(releaseBlock){check(pool!=nullptr);for(auto entry:*pool)check(entry[0]!=0);check(replacement[0]==2);replacement[0]=0;}} void publish(unsigned* p){check(p==previous.data()&&p[0]==2&&p[2]==0);++publishes;check(publication!=nullptr);studioDeviceQueuePublishCandidate(*publication,reinterpret_cast<std::uintptr_t>(p),*this);}
 void refill(){studioDeviceQueueRefillCandidate(state,*this);} unsigned* acquire(unsigned header){check(header==1&&pool!=nullptr);++acquires;return studioDeviceQueueAcquireCandidate(*pool,header,*this);}
 void queueSampler(const std::array<unsigned,4>& words){if(state.remaining<4)studioDeviceQueueRefillCandidate(state,*this);check(state.remaining>=4);for(auto word:words)*state.cursor++=word;state.remaining-=4;}
 void directSampler(unsigned,unsigned,unsigned){throw std::runtime_error("unexpected direct call");}
};
int main(){try{static_assert(sizeof(void*)==8,"x64 comparison");std::ifstream records("outputs/native-restore-submission.bin",std::ios::binary);check(bool(records));unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned field:{0u,1u,256u,0xffffffffu})for(unsigned depth:{0u,1u,256u,0xffffffffu})for(unsigned queued=0;queued<2;++queued){
  std::array<unsigned,432> owner{},originalOwner{};owner.fill(0xabababab);std::array<unsigned,16> queue{},originalQueue{};queue.fill(0xcccccccc);owner[0]=owner[0x648/4]=0;owner[0xa8/4]=field;reinterpret_cast<unsigned char*>(owner.data())[0xac]=static_cast<unsigned char>(flags);owner[0x2cc/4]=depth;owner[0x658/4]=queued;owner[0x660/4]=0;owner[0x664/4]=16;
  Ops ops{owner,queue,0,flags%16,flags*0x01010101u};studioSamplerSubmitCandidate(ops.stage,1,ops.value,ops);
  unsigned originalCalls=0;records.read(reinterpret_cast<char*>(originalOwner.data()),sizeof(originalOwner));check(records.gcount()==sizeof(originalOwner));records.read(reinterpret_cast<char*>(originalQueue.data()),sizeof(originalQueue));check(records.gcount()==sizeof(originalQueue));records.read(reinterpret_cast<char*>(&originalCalls),4);check(records.gcount()==4);check(owner==originalOwner&&queue==originalQueue&&ops.directCalls==originalCalls);++cases;
 }
 check(records.peek()==std::char_traits<char>::eof());std::ifstream refillRecords("outputs/native-restore-refill-submission.bin",std::ios::binary);check(bool(refillRecords));unsigned refillCases=0;std::ifstream ringRecords("outputs/native-restore-refill-ring.bin",std::ios::binary);check(bool(ringRecords));
 for(unsigned seed=0;seed<64;++seed)for(unsigned capacity=0;capacity<4;++capacity)for(unsigned mode=0;mode<3;++mode){unsigned full=unsigned(mode!=0);
  std::array<unsigned,432> owner{},originalOwner{};owner.fill(0xabababab);owner[0]=owner[0x648/4]=0;owner[0xa8/4]=owner[0x2cc/4]=0;reinterpret_cast<unsigned char*>(owner.data())[0xac]=0;owner[0x658/4]=1;
  std::array<unsigned,16> previous{},originalPrevious{};previous.fill(0xcccccccc);previous[0]=1;std::array<unsigned,4096> replacement{},originalReplacement{};replacement.fill(0xdddddddd);replacement[0]=mode==2?2u:0u;std::array<std::array<unsigned,4>,500> occupied{};std::array<unsigned*,500> pool{};for(unsigned i=0;i<500;++i){occupied[i].fill(0xaabb0000u+i);occupied[i][0]=2;pool[i]=occupied[i].data();}pool[seed%3==2?499:seed%3]=replacement.data();auto expectedPool=pool;auto expectedOccupied=occupied;StudioDeviceQueueStateCandidate state{previous.data(),previous.data()+2,capacity};StudioDeviceQueuePublishCandidate publication;publication.ring.fill(0xeeeeeeee);publication.produced=seed;publication.completed=seed-(full?500u:0u);publication.index=seed%3==2?499:seed%3;auto initialIndex=publication.index;RefillOps ops{state,previous,replacement};ops.publication=&publication;ops.pool=&pool;ops.releaseBlock=mode==2;studioSamplerSubmitCandidate(seed%16,1,seed*0x01010101u,ops);
  check(pool==expectedPool&&occupied==expectedOccupied);owner[0x65c/4]=state.block==replacement.data();owner[0x660/4]=static_cast<unsigned>((state.cursor-state.block)*4);owner[0x664/4]=state.remaining;
  refillRecords.read(reinterpret_cast<char*>(originalOwner.data()),sizeof(originalOwner));check(refillRecords.gcount()==sizeof(originalOwner));refillRecords.read(reinterpret_cast<char*>(originalPrevious.data()),sizeof(originalPrevious));check(refillRecords.gcount()==sizeof(originalPrevious));refillRecords.read(reinterpret_cast<char*>(originalReplacement.data()),sizeof(originalReplacement));check(refillRecords.gcount()==sizeof(originalReplacement));std::array<unsigned,3> counts{};refillRecords.read(reinterpret_cast<char*>(counts.data()),sizeof(counts));check(refillRecords.gcount()==sizeof(counts));check(owner==originalOwner&&previous==originalPrevious&&replacement==originalReplacement&&counts==std::array<unsigned,3>{ops.publishes,ops.acquires,0});std::array<unsigned,640> actualRing{},originalRing{};actualRing.fill(0xeeeeeeee);for(unsigned i=0;i<500;++i)actualRing[i]=i==initialIndex?unsigned(publication.ring[i]==reinterpret_cast<std::uintptr_t>(previous.data())):static_cast<unsigned>(publication.ring[i]);actualRing[0x8d0/4]=publication.produced;actualRing[0x8d4/4]=publication.index;actualRing[0x9d8/4]=publication.completed;ringRecords.read(reinterpret_cast<char*>(originalRing.data()),sizeof(originalRing));check(ringRecords.gcount()==sizeof(originalRing));unsigned waits=99;ringRecords.read(reinterpret_cast<char*>(&waits),4);check(ringRecords.gcount()==4&&waits==ops.waits&&ops.waits==full&&actualRing==originalRing);++refillCases;
 }
 check(refillRecords.peek()==std::char_traits<char>::eof());check(ringRecords.peek()==std::char_traits<char>::eof());std::cout<<"x64_gate_submission_records_matched="<<cases<<" refill_records_matched="<<refillCases<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}


