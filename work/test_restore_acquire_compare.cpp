#include "studio_device_queue_acquire_candidate.h"
#include "studio_device_queue_refill_candidate.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
static void check(bool b){if(!b)throw std::runtime_error("allocator comparison mismatch");}
struct Services{
 std::array<std::array<unsigned,4>,500>& blocks;unsigned index,rounds,waits=0,refills=0;StudioDeviceQueueStateCandidate* state=nullptr;
 void refill(){++refills;check(refills==waits+1&&refills<=rounds);check(state!=nullptr);studioDeviceQueueRefillCandidate(*state,*this);}
 void publish(unsigned*){throw std::runtime_error("empty refill published");}
 unsigned* acquire(unsigned){throw std::runtime_error("empty refill allocated");}
 void waitZero(){++waits;check(waits==refills&&waits<=rounds);if(waits==rounds)blocks[index][0]=0;}
};
int main(){try{static_assert(sizeof(void*)==8,"x64 pool pointers");std::ifstream records("outputs/native-restore-acquire.bin",std::ios::binary);check(bool(records));unsigned cases=0;
 for(unsigned index:{0u,1u,499u})for(unsigned header:{0u,1u,2u})for(unsigned occupied=0;occupied<2;++occupied)for(unsigned rounds=1;rounds<=3;++rounds){
  std::array<std::array<unsigned,4>,500> blocks{},original{};std::array<unsigned*,500> pool{};for(unsigned i=0;i<500;++i){blocks[i].fill(0xabab0000u+i);blocks[i][0]=2;pool[i]=blocks[i].data();}if(!occupied)blocks[index][0]=0;auto beforePool=pool;std::array<unsigned,4> emptyBlock{};emptyBlock.fill(0xdddddddd);emptyBlock[0]=1;auto beforeEmpty=emptyBlock;StudioDeviceQueueStateCandidate state{emptyBlock.data(),emptyBlock.data()+1,index};Services services{blocks,index,rounds};services.state=&state;auto result=studioDeviceQueueAcquireCandidate(pool,header,services);check(result==blocks[index].data()&&pool==beforePool&&services.waits==occupied*rounds&&services.refills==occupied*rounds&&emptyBlock==beforeEmpty&&state.block==emptyBlock.data()&&state.cursor==emptyBlock.data()+1&&state.remaining==index);
  std::array<unsigned,8> owner{},originalOwner{};owner.fill(0xcccccccc);owner[5]=1;owner[6]=4;owner[7]=index;std::array<unsigned,3> originalCounts{};records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(records.gcount()==sizeof(original));records.read(reinterpret_cast<char*>(originalOwner.data()),sizeof(originalOwner));check(records.gcount()==sizeof(originalOwner));records.read(reinterpret_cast<char*>(originalCounts.data()),sizeof(originalCounts));check(records.gcount()==sizeof(originalCounts));check(blocks==original&&owner==originalOwner&&originalCounts==std::array<unsigned,3>{index,services.waits,services.refills});++cases;
 }
 check(records.peek()==std::char_traits<char>::eof());std::cout<<"x64_acquire_records_matched="<<cases<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
