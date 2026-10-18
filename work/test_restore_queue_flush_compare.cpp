
#include "studio_device_queue_flush_candidate.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
static void check(bool b){if(!b)throw std::runtime_error("flush comparison mismatch");}
struct Ops{
 std::array<unsigned,16>& oldBlock;std::array<unsigned,16>& newBlock;unsigned producer,completion,publishes=0,acquires=0;
 void publish(unsigned* block){check(block==oldBlock.data()&&block[0]==2&&block[2]==0);++publishes;++producer;}
 unsigned* acquire(unsigned header){check(header==1);++acquires;newBlock[0]=header;completion=producer;return newBlock.data();}
 unsigned produced(){return producer;}unsigned completed(){return completion;}void pollCompletion(){throw std::runtime_error("unexpected pending completion");}
};
int main(){try{static_assert(sizeof(void*)==8,"x64 required");std::ifstream in("outputs/native-restore-queue-flush.bin",std::ios::binary);check(bool(in));unsigned cases=0;
 for(unsigned seed:{0u,499u,0xffffffffu})for(unsigned enabled=0;enabled<2;++enabled)for(unsigned mode=0;mode<3;++mode){
  std::array<unsigned,16> oldBlock{},newBlock{};oldBlock.fill(0xcccccccc);oldBlock[0]=1;newBlock.fill(0xdddddddd);newBlock[0]=0;StudioDeviceQueueStateCandidate state{mode?oldBlock.data():nullptr,mode?oldBlock.data()+(mode==1?1:2):nullptr,7};
  Ops ops{oldBlock,newBlock,seed,enabled&&mode==1?seed:seed-1};studioDeviceQueueFlushCandidate(enabled!=0,state,ops);
  std::array<unsigned,8> owner{};owner.fill(0xabababab);owner[4]=enabled;owner[5]=state.block?(state.block==oldBlock.data()?1u:2u):0;check(!state.block||state.block==oldBlock.data()||state.block==newBlock.data());owner[6]=state.block?static_cast<unsigned>((state.cursor-state.block)*4):0;owner[7]=state.remaining;
  std::array<unsigned,44> actual{},original{};unsigned offset=0;for(auto v:owner)actual[offset++]=v;for(auto v:oldBlock)actual[offset++]=v;for(auto v:newBlock)actual[offset++]=v;actual[offset++]=ops.producer;actual[offset++]=ops.completion;actual[offset++]=ops.publishes;actual[offset++]=ops.acquires;check(offset==actual.size());in.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(in.gcount()==sizeof(original)&&actual==original);++cases;
 }
 check(in.peek()==std::char_traits<char>::eof());std::cout<<"x64_queue_flush_records_matched="<<cases<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
