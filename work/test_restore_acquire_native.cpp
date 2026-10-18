#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "restore_acquire_init.h"
#include "restore_refill_body_init.h"
static void check(bool b){if(!b)throw std::runtime_error("acquire mismatch");}
static std::array<std::array<unsigned,4>,500> blocks;
static std::array<unsigned*,500> pool;
static std::array<unsigned,4> emptyBlock;static void* refillBody;
static void __fastcall forbiddenPublish(void*,void*,unsigned*){throw std::runtime_error("empty refill published");}
static unsigned* __fastcall forbiddenAcquire(void*,void*,unsigned){throw std::runtime_error("empty refill allocated");}
static void* expectedOwner;static unsigned freeIndex,waits,refills,completionRound;
static void __fastcall refill(void* owner,void*){check(owner==expectedOwner);++refills;check(refills==waits+1&&refills<=completionRound);reinterpret_cast<void(__thiscall*)(void*)>(refillBody)(owner);}
static void __stdcall waitCallback(unsigned value){check(value==0);++waits;check(waits==refills&&waits<=completionRound);if(waits==completionRound)blocks[freeIndex][0]=0;}
int main(){try{
 std::ifstream installed("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);check(bool(installed));unsigned char raw[77]{};installed.seekg(acquireOffset);installed.read(reinterpret_cast<char*>(raw),77);check(installed.gcount()==77&&std::memcmp(raw,acquireBytes,77)==0);
 unsigned char refillRaw[76]{};installed.seekg(refillOffset);installed.read(reinterpret_cast<char*>(refillRaw),76);check(installed.gcount()==76&&std::memcmp(refillRaw,refillBytes,76)==0);
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(memory!=nullptr);std::memcpy(memory,raw,77);auto waitFunction=&waitCallback;auto cell=reinterpret_cast<unsigned>(&waitFunction);std::memcpy(memory+6,&cell,4);auto table=reinterpret_cast<unsigned>(pool.data());for(unsigned operand:{21u,55u,68u})std::memcpy(memory+operand,&table,4);auto delta=reinterpret_cast<unsigned>(&refill)-reinterpret_cast<unsigned>(memory+46);std::memcpy(memory+42,&delta,4);std::memcpy(memory+256,refillRaw,76);refillBody=memory+256;unsigned ring=0;std::memcpy(memory+256+40,&ring,4);auto publishDelta=reinterpret_cast<unsigned>(&forbiddenPublish)-reinterpret_cast<unsigned>(memory+256+49);std::memcpy(memory+256+45,&publishDelta,4);auto acquireDelta=reinterpret_cast<unsigned>(&forbiddenAcquire)-reinterpret_cast<unsigned>(memory+256+58);std::memcpy(memory+256+54,&acquireDelta,4);DWORD old=0;check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0);check(FlushInstructionCache(GetCurrentProcess(),memory,332)!=0);auto acquire=reinterpret_cast<unsigned*(__thiscall*)(void*,unsigned)>(memory);unsigned cases=0;
 std::ofstream records("outputs/native-restore-acquire.bin",std::ios::binary);check(bool(records));
 for(unsigned index:{0u,1u,499u})for(unsigned header:{0u,1u,2u})for(unsigned occupied=0;occupied<2;++occupied)for(unsigned rounds=1;rounds<=3;++rounds){
  for(unsigned i=0;i<500;++i){blocks[i].fill(0xabab0000u+i);blocks[i][0]=2;pool[i]=blocks[i].data();}if(!occupied)blocks[index][0]=0;auto expected=blocks;expected[index][0]=header;auto expectedPool=pool;std::array<unsigned,8> owner{};owner.fill(0xcccccccc);emptyBlock.fill(0xdddddddd);emptyBlock[0]=1;auto expectedEmpty=emptyBlock;owner[5]=reinterpret_cast<unsigned>(emptyBlock.data());owner[6]=reinterpret_cast<unsigned>(emptyBlock.data()+1);owner[7]=index;auto expectedState=owner;expectedOwner=owner.data();freeIndex=index;completionRound=rounds;waits=refills=0;
  auto result=acquire(owner.data(),header);check(result==blocks[index].data()&&blocks==expected&&pool==expectedPool&&owner==expectedState&&emptyBlock==expectedEmpty&&waits==occupied*rounds&&refills==occupied*rounds);records.write(reinterpret_cast<const char*>(blocks.data()),sizeof(blocks));auto normalizedOwner=owner;normalizedOwner[5]=1;normalizedOwner[6]=4;records.write(reinterpret_cast<const char*>(normalizedOwner.data()),sizeof(normalizedOwner));unsigned counts[]={index,waits,refills};records.write(reinterpret_cast<const char*>(counts),sizeof(counts));check(bool(records));++cases;
 }
 check(VirtualFree(memory,0,MEM_RELEASE)!=0);std::cout<<"original_acquire_cases="<<cases<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

