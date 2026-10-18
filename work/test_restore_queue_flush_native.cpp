
#include <windows.h>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include "restore_queue_flush_init.h"
static void check(bool b){if(!b)throw std::runtime_error("flush mismatch");}
static std::array<unsigned,16> oldBlock,newBlock;
static unsigned produced,completed,publishes,acquires;
static void* expectedOwner;
static unsigned ringToken;
static void __fastcall publish(void* receiver,void*,unsigned* block){check(receiver==&ringToken&&block==oldBlock.data()&&block[0]==2&&block[2]==0);++publishes;++produced;}
static unsigned* __fastcall acquire(void* receiver,void*,unsigned header){check(receiver==expectedOwner&&header==1);++acquires;newBlock[0]=header;completed=produced;return newBlock.data();}
int main(){try{
 std::ifstream dll("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);check(bool(dll));unsigned char raw[97]{};dll.seekg(queueFlushOffset);dll.read(reinterpret_cast<char*>(raw),97);check(dll.gcount()==97&&std::memcmp(raw,queueFlushBytes,97)==0);
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(code!=nullptr);std::memcpy(code,raw,97);
 auto ptr=[&](unsigned offset,void* target){auto value=reinterpret_cast<unsigned>(target);std::memcpy(code+offset,&value,4);};
 ptr(46,&ringToken);ptr(82,&produced);ptr(87,&completed);
 auto call=[&](unsigned operand,unsigned next,void* target){auto delta=reinterpret_cast<unsigned>(target)-reinterpret_cast<unsigned>(code+next);std::memcpy(code+operand,&delta,4);};
 call(51,55,reinterpret_cast<void*>(&publish));call(60,64,reinterpret_cast<void*>(&acquire));
 DWORD protection=0;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&protection)!=0);check(FlushInstructionCache(GetCurrentProcess(),code,97)!=0);
 auto flush=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream out("outputs/native-restore-queue-flush.bin",std::ios::binary);check(bool(out));unsigned cases=0;
 for(unsigned seed:{0u,499u,0xffffffffu})for(unsigned enabled=0;enabled<2;++enabled)for(unsigned mode=0;mode<3;++mode){
  oldBlock.fill(0xcccccccc);oldBlock[0]=1;newBlock.fill(0xdddddddd);newBlock[0]=0;std::array<unsigned,8> owner{};owner.fill(0xabababab);owner[4]=enabled;owner[5]=mode?reinterpret_cast<unsigned>(oldBlock.data()):0;owner[6]=mode?reinterpret_cast<unsigned>(oldBlock.data()+(mode==1?1:2)):0;owner[7]=7;expectedOwner=owner.data();
  produced=seed;completed=enabled&&mode==1?seed:seed-1;publishes=acquires=0;
  auto expected=owner;auto expectedOld=oldBlock,expectedNew=newBlock;unsigned expectedPublish=enabled&&mode==2?1u:0u;unsigned expectedAcquire=enabled&&mode!=1?1u:0u;
  if(expectedPublish){expectedOld[0]=2;expectedOld[2]=0;}if(expectedAcquire){expectedNew[0]=1;expected[5]=reinterpret_cast<unsigned>(newBlock.data());expected[6]=reinterpret_cast<unsigned>(newBlock.data()+1);expected[7]=4095;}
  flush(owner.data());check(owner==expected&&oldBlock==expectedOld&&newBlock==expectedNew&&publishes==expectedPublish&&acquires==expectedAcquire&&produced==seed+expectedPublish&&completed==(enabled?produced:seed-1));
  auto normalized=owner;if(owner[5]){auto base=owner[5];normalized[5]=base==reinterpret_cast<unsigned>(oldBlock.data())?1u:2u;check(base==reinterpret_cast<unsigned>(oldBlock.data())||base==reinterpret_cast<unsigned>(newBlock.data()));normalized[6]=owner[6]-base;}
  out.write(reinterpret_cast<const char*>(normalized.data()),sizeof(normalized));out.write(reinterpret_cast<const char*>(oldBlock.data()),sizeof(oldBlock));out.write(reinterpret_cast<const char*>(newBlock.data()),sizeof(newBlock));unsigned tail[]={produced,completed,publishes,acquires};out.write(reinterpret_cast<const char*>(tail),sizeof(tail));check(bool(out));++cases;
 }
 check(VirtualFree(code,0,MEM_RELEASE)!=0);std::cout<<"original_queue_flush_cases="<<cases<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
