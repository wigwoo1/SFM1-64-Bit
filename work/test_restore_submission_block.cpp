#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "restore_submission_block_init.h"
#include "restore_owner_gate_init.h"
#include "studio_restore_owner_gate_candidate.h"
#include "restore_refill_body_init.h"
#include "restore_publish_init.h"
#include "restore_acquire_init.h"
static void check(bool b){if(!b)throw std::runtime_error("submission mismatch");}
static void* block;
static unsigned directCalls,expectedStage,expectedValue;
static void* device;
static std::array<unsigned*,500> allocatorPool;static std::array<std::array<unsigned,4>,500> occupiedBlocks;static unsigned publishCalls,acquireCalls,initialProduced,initialIndex,publicationWaits;
static std::array<unsigned,640> publicationRing;static unsigned* completionBlock=nullptr;
static void __stdcall publicationWait(unsigned value){check(value==0);++publicationWaits;check(publicationWaits==1);publicationRing[0x9d8/4]=initialProduced-499u;if(completionBlock){for(auto entry:allocatorPool)check(entry[0]!=0);check(completionBlock[0]==2);completionBlock[0]=0;}}
static unsigned __stdcall sampler(void* receiver,unsigned stage,unsigned state,unsigned value){check(receiver==device&&stage==expectedStage&&state==1&&value==expectedValue);++directCalls;return 0x80004005u;}
__declspec(naked) static void invoke(void*,unsigned,unsigned){__asm{
 push ebp
 mov ebp,esp
 sub esp,38h
 push ebx
 push edi
 mov ebx,[ebp+8]
 mov edi,[ebp+12]
 mov eax,[ebp+16]
 mov edx,eax
 mov [ebp-38h],eax
 call dword ptr [block]
 pop edi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
int main(){try{
 std::ifstream installed("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);check(bool(installed));unsigned char raw[134]{};installed.seekg(blockOffset);installed.read(reinterpret_cast<char*>(raw),134);check(installed.gcount()==134&&std::memcmp(raw,blockBytes,134)==0);unsigned char gate[36]{};installed.seekg(ownerGateOffset);installed.read(reinterpret_cast<char*>(gate),36);check(installed.gcount()==36&&std::memcmp(gate,ownerGateBytes,36)==0);
 unsigned char refill[76]{};installed.seekg(refillOffset);installed.read(reinterpret_cast<char*>(refill),76);check(installed.gcount()==76&&std::memcmp(refill,refillBytes,76)==0);
 unsigned char allocator[77]{};installed.seekg(acquireOffset);installed.read(reinterpret_cast<char*>(allocator),77);check(installed.gcount()==77&&std::memcmp(allocator,acquireBytes,77)==0);
 unsigned char publisher[105]{};installed.seekg(publishOffset);installed.read(reinterpret_cast<char*>(publisher),105);check(installed.gcount()==105&&std::memcmp(publisher,publishBytes,105)==0);
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(memory!=nullptr);std::memcpy(memory,raw,134);memory[134]=0xc3;std::memcpy(memory+512,gate,36);std::memcpy(memory+768,refill,76);unsigned delta=768-48;std::memcpy(memory+44,&delta,4);
 std::memcpy(memory+1024,publisher,105);auto waitFunction=&publicationWait;auto waitCell=reinterpret_cast<unsigned>(&waitFunction);std::memcpy(memory+1024+30,&waitCell,4);
 auto ring=reinterpret_cast<unsigned>(publicationRing.data());std::memcpy(memory+768+40,&ring,4);auto patch=[&](unsigned operand,unsigned next,void* target){auto displacement=reinterpret_cast<unsigned>(target)-reinterpret_cast<unsigned>(memory+768+next);std::memcpy(memory+768+operand,&displacement,4);};patch(45,49,memory+1024);patch(54,58,memory+1280);std::memcpy(memory+1280,allocator,77);std::memcpy(memory+1280+6,&waitCell,4);auto poolAddress=reinterpret_cast<unsigned>(allocatorPool.data());for(unsigned operand:{21u,55u,68u})std::memcpy(memory+1280+operand,&poolAddress,4);auto refillDelta=unsigned(768-(1280+46));std::memcpy(memory+1280+42,&refillDelta,4);
 DWORD old=0;check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0);check(FlushInstructionCache(GetCurrentProcess(),memory,4096)!=0);block=memory;
 std::array<std::uintptr_t,37> ownerTable{};ownerTable[36]=reinterpret_cast<std::uintptr_t>(memory+512);std::array<std::uintptr_t,70> deviceTable{};deviceTable[69]=reinterpret_cast<std::uintptr_t>(&sampler);auto deviceTablePointer=deviceTable.data();device=&deviceTablePointer;unsigned cases=0;
 std::ofstream records("outputs/native-restore-submission.bin",std::ios::binary);check(bool(records));
 for(unsigned flags=0;flags<256;++flags)for(unsigned field:{0u,1u,256u,0xffffffffu})for(unsigned depth:{0u,1u,256u,0xffffffffu})for(unsigned queued=0;queued<2;++queued){
  std::array<unsigned,432> owner{};owner.fill(0xabababab);std::array<unsigned,16> queue{};queue.fill(0xcccccccc);owner[0]=reinterpret_cast<unsigned>(ownerTable.data());owner[0xa8/4]=field;reinterpret_cast<unsigned char*>(owner.data())[0xac]=static_cast<unsigned char>(flags);owner[0x2cc/4]=depth;owner[0x648/4]=reinterpret_cast<unsigned>(device);owner[0x658/4]=queued;owner[0x660/4]=reinterpret_cast<unsigned>(queue.data());owner[0x664/4]=16;
  auto expectedOwner=owner;auto expectedQueue=queue;bool suppressed=studioRestoreOwnerGateCandidate(field,static_cast<unsigned char>(flags),depth)!=0;expectedStage=flags%16;expectedValue=flags*0x01010101u;directCalls=0;
  if(!suppressed&&queued){expectedQueue[0]=17;expectedQueue[1]=expectedStage;expectedQueue[2]=1;expectedQueue[3]=expectedValue;expectedOwner[0x660/4]+=16;expectedOwner[0x664/4]-=4;}
  invoke(owner.data(),expectedStage,expectedValue);if(owner!=expectedOwner||queue!=expectedQueue){std::cerr<<"flags="<<flags<<" field="<<field<<" depth="<<depth<<" queued="<<queued<<" owner="<<(owner==expectedOwner)<<" queue="<<(queue==expectedQueue)<<" words="<<queue[0]<<","<<queue[1]<<","<<queue[2]<<","<<queue[3]<<" remaining="<<owner[0x664/4]<<char(10);}check(owner==expectedOwner&&queue==expectedQueue);check(directCalls==(!suppressed&&!queued?1u:0u));++cases;
  auto normalized=owner;normalized[0]=normalized[0x648/4]=0;normalized[0x660/4]-=reinterpret_cast<unsigned>(queue.data());records.write(reinterpret_cast<const char*>(normalized.data()),sizeof(normalized));records.write(reinterpret_cast<const char*>(queue.data()),sizeof(queue));records.write(reinterpret_cast<const char*>(&directCalls),4);check(bool(records));
 }
 unsigned refillCases=0;
 std::ofstream refillRecords("outputs/native-restore-refill-submission.bin",std::ios::binary);check(bool(refillRecords));
 std::ofstream publicationRecords("outputs/native-restore-refill-ring.bin",std::ios::binary);check(bool(publicationRecords));
 for(unsigned seed=0;seed<64;++seed)for(unsigned capacity=0;capacity<4;++capacity)for(unsigned mode=0;mode<3;++mode){unsigned full=unsigned(mode!=0);
  std::array<unsigned,432> owner{};owner.fill(0xabababab);std::array<unsigned,16> previous{};previous.fill(0xcccccccc);std::array<unsigned,4096> replacement{};replacement.fill(0xdddddddd);replacement[0]=mode==2?2u:0u;completionBlock=mode==2?replacement.data():nullptr;for(unsigned i=0;i<500;++i){occupiedBlocks[i].fill(0xaabb0000u+i);occupiedBlocks[i][0]=2;allocatorPool[i]=occupiedBlocks[i].data();}unsigned poolIndex=seed%3==2?499:seed%3;allocatorPool[poolIndex]=replacement.data();auto expectedPool=allocatorPool;auto expectedOccupied=occupiedBlocks;
  owner[0]=reinterpret_cast<unsigned>(ownerTable.data());owner[0xa8/4]=owner[0x2cc/4]=0;reinterpret_cast<unsigned char*>(owner.data())[0xac]=0;owner[0x648/4]=reinterpret_cast<unsigned>(device);owner[0x658/4]=1;owner[0x65c/4]=reinterpret_cast<unsigned>(previous.data());owner[0x660/4]=reinterpret_cast<unsigned>(previous.data()+2);owner[0x664/4]=capacity;previous[0]=1;
  auto expectedOwner=owner;auto expectedPrevious=previous;auto expectedReplacement=replacement;expectedPrevious[0]=2;expectedPrevious[2]=0;expectedReplacement[0]=1;expectedReplacement[1]=17;expectedReplacement[2]=seed%16;expectedReplacement[3]=1;expectedReplacement[4]=seed*0x01010101u;expectedOwner[0x65c/4]=reinterpret_cast<unsigned>(replacement.data());expectedOwner[0x660/4]=reinterpret_cast<unsigned>(replacement.data()+5);expectedOwner[0x664/4]=4091;
  initialProduced=seed;initialIndex=seed%3==2?499:seed%3;publicationRing.fill(0xeeeeeeee);publicationRing[0x8d0/4]=seed;publicationRing[0x9d8/4]=seed-(full?500u:0u);publicationRing[0x8d4/4]=initialIndex;auto expectedRing=publicationRing;expectedRing[initialIndex]=reinterpret_cast<unsigned>(previous.data());expectedRing[0x9d8/4]=full?seed-499u:seed;expectedRing[0x8d0/4]=seed+1;expectedRing[0x8d4/4]=(initialIndex+1)%500;
  publishCalls=acquireCalls=directCalls=publicationWaits=0;invoke(owner.data(),seed%16,seed*0x01010101u);publishCalls=publicationRing[0x8d0/4]-initialProduced;acquireCalls=unsigned(replacement[0]==1);check(allocatorPool==expectedPool&&occupiedBlocks==expectedOccupied);check(owner==expectedOwner&&previous==expectedPrevious&&replacement==expectedReplacement&&publicationRing==expectedRing);check(publishCalls==1&&acquireCalls==1&&directCalls==0&&publicationWaits==full);++refillCases;
  auto normalizedRing=publicationRing;normalizedRing[initialIndex]=1;publicationRecords.write(reinterpret_cast<const char*>(normalizedRing.data()),sizeof(normalizedRing));publicationRecords.write(reinterpret_cast<const char*>(&publicationWaits),4);check(bool(publicationRecords));
  auto normalized=owner;normalized[0]=normalized[0x648/4]=0;normalized[0x65c/4]=owner[0x65c/4]==reinterpret_cast<unsigned>(replacement.data());normalized[0x660/4]-=reinterpret_cast<unsigned>(replacement.data());refillRecords.write(reinterpret_cast<const char*>(normalized.data()),sizeof(normalized));refillRecords.write(reinterpret_cast<const char*>(previous.data()),sizeof(previous));refillRecords.write(reinterpret_cast<const char*>(replacement.data()),sizeof(replacement));unsigned counts[]={publishCalls,acquireCalls,directCalls};refillRecords.write(reinterpret_cast<const char*>(counts),sizeof(counts));check(bool(refillRecords));
 }
 check(VirtualFree(memory,0,MEM_RELEASE)!=0);std::cout<<"original_gate_submission_cases="<<cases<<" actual_refill_cases="<<refillCases<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}


