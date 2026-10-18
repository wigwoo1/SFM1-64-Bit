#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "viewport_callback_init.h"
#include "restore_refill_body_init.h"
#include "restore_publish_init.h"
#include "restore_acquire_init.h"
#include "restore_shared_queue_refill_init.h"
#include "studio_shader_viewport_submit_candidate.h"
#include "studio_device_queue_refill_candidate.h"
#include "studio_device_queue_publish_candidate.h"
#include "studio_device_queue_acquire_candidate.h"
static void check(bool b){if(!b)std::abort();}
static std::array<unsigned,8> command;
static std::array<unsigned,640> ring;
static std::array<unsigned*,500> pool;
static std::array<std::array<unsigned,4>,500> occupied;
static std::array<unsigned,4096> previous,replacement;
static unsigned waits,cookie,seed;static bool recursivePool;
#ifdef _M_IX86
static void __stdcall waitZero(unsigned value){check(value==0&&++waits<=(recursivePool?2u:1u));ring[0x9d8/4]=ring[0x8d0/4]-499u;if(recursivePool){check(replacement[0]==(waits==1?2u:1u));replacement[0]=0;}}
static void __fastcall cookieCheck(unsigned value,void*){check(value==cookie);}
#endif
#ifndef _M_IX86
struct Ops{
 StudioDeviceQueueStateCandidate state;
 StudioDeviceQueuePublishCandidate publication;
 bool queueEnabled(){return true;}
 void waitZero(){check(++waits<=(recursivePool?2u:1u));publication.completed=publication.produced-499u;if(recursivePool){check(replacement[0]==(waits==1?2u:1u));replacement[0]=0;}}
 void publish(unsigned* block){check(block==previous.data());studioDeviceQueuePublishCandidate(publication,reinterpret_cast<uintptr_t>(block),*this);}
 unsigned* acquire(unsigned header){return studioDeviceQueueAcquireCandidate(pool,header,*this);}
 void refill(){studioDeviceQueueRefillCandidate(state,*this);}
 void queueViewport(const StudioShaderViewportWordsCandidate& words){if(state.remaining<7)refill();check(state.remaining>=7);*state.cursor++=20;for(auto word:words)*state.cursor++=word;state.remaining-=7;}
 void directViewport(const StudioShaderViewportWordsCandidate&){std::abort();}
};
#endif
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=nullptr);
 std::ifstream bundle("outputs/restore-shared-queue-refill-template.bin",std::ios::binary);bundle.read(reinterpret_cast<char*>(code),8192);check(bool(bundle)&&bundle.peek()==std::char_traits<char>::eof());
 auto verify=[&](unsigned offset,const unsigned char* raw,unsigned size){std::array<unsigned char,249> bytes{};dll.seekg(offset);dll.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(dll)&&!std::memcmp(bytes.data(),raw,size));};
 verify(viewportOffset,viewportBytes,249);verify(refillOffset,refillBytes,76);verify(publishOffset,publishBytes,105);verify(acquireOffset,acquireBytes,77);
 // Reconstruct the linked bundle from freshly verified bodies rather than trust an artifact alone.
 std::memcpy(code+5120,refillBytes,76);std::memcpy(code+5632,publishBytes,105);std::memcpy(code+6144,acquireBytes,77);
 for(auto link:sharedRefillLinks){unsigned delta=link[1]-(link[0]+4);std::memcpy(code+link[0],&delta,4);}
 auto waitFunction=&waitZero;unsigned bindings[]{static_cast<unsigned>(reinterpret_cast<uintptr_t>(ring.data())),static_cast<unsigned>(reinterpret_cast<uintptr_t>(pool.data())),static_cast<unsigned>(reinterpret_cast<uintptr_t>(&waitFunction))};
 for(auto binding:sharedRefillBindings)std::memcpy(code+binding[0],bindings+binding[1],4);
 std::memcpy(code,viewportBytes,249);
 for(auto fix:viewportFixups){auto address=fix[1]==0x1017c370?&cookie:command.data()+(fix[1]-0x10180aa8)/4;unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(address));std::memcpy(code+fix[0],&value,4);}
 for(auto call:viewportCalls){uintptr_t target=call[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&cookieCheck):reinterpret_cast<uintptr_t>(code+5120);unsigned delta=static_cast<unsigned>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;check(VirtualProtect(code,8192,PAGE_EXECUTE_READ,&old)!=0&&FlushInstructionCache(GetCurrentProcess(),code,8192)!=0);
 auto callback=reinterpret_cast<void(__cdecl*)(void*,void*,void*,unsigned)>(code);
 std::ofstream records("outputs/native-viewport-refill-chain.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream records("outputs/native-viewport-refill-chain.bin",std::ios::binary);
#endif
 unsigned cases=0;
 for(unsigned capacity=0;capacity<=8;++capacity)for(unsigned index:{0u,1u,499u})for(unsigned mode=0;mode<3;++mode){bool full=mode==1;recursivePool=mode==2;
 seed=0xffffffffu;waits=0;previous.fill(0xcccccccc);replacement.fill(0xdddddddd);previous[0]=1;previous[1]=0xcafef00d;replacement[0]=recursivePool?2u:0u;
 for(unsigned i=0;i<500;++i){occupied[i].fill(0xaabb0000+i);occupied[i][0]=2;pool[i]=occupied[i].data();}pool[index]=replacement.data();auto expectedPool=pool;auto expectedOccupied=occupied;
 auto expectedPrevious=previous,expectedReplacement=replacement;
 StudioShaderViewportWordsCandidate live{9,7,64,32,0x80000000u,0x3f800000u},cache{};
 bool crosses=capacity<7;auto& output=crosses?expectedReplacement:expectedPrevious;unsigned at=crosses?1u:2u;output[at]=20;std::copy(live.begin(),live.end(),output.begin()+at+1);if(crosses){expectedPrevious[0]=2;expectedPrevious[2]=0;expectedReplacement[0]=1;}
 ring.fill(0xeeeeeeee);ring[0x8d0/4]=seed;ring[0x8d4/4]=index;ring[0x9d8/4]=seed-(recursivePool?499u:full?500u:0u);auto expectedRing=ring;if(crosses){expectedRing[index]=1;if(recursivePool)expectedRing[(index+1)%500]=1;expectedRing[0x8d0/4]=seed+(recursivePool?2u:1u);expectedRing[0x8d4/4]=(index+(recursivePool?2u:1u))%500;expectedRing[0x9d8/4]=recursivePool?seed-497u:full?seed-499u:seed;}
 command.fill(0xabababab);command[0]=0;command[4]=1;command[5]=0;command[6]=8;command[7]=capacity;
#ifdef _M_IX86
 command[5]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(previous.data()));command[6]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(previous.data()+2));callback(nullptr,live.data(),cache.data(),1);
 auto* selected=crosses?replacement.data():previous.data();check(command[5]==reinterpret_cast<uintptr_t>(selected));command[5]=crosses?1u:0u;command[6]-=static_cast<unsigned>(reinterpret_cast<uintptr_t>(selected));if(crosses){for(unsigned j=0;j<(recursivePool?2u:1u);++j){unsigned entry=(index+j)%500;check(ring[entry]==reinterpret_cast<uintptr_t>(previous.data()));ring[entry]=1;}}
#else
 Ops ops;ops.state={previous.data(),previous.data()+2,capacity};ops.publication.ring.fill(0xeeeeeeee);ops.publication.produced=seed;ops.publication.completed=seed-(recursivePool?499u:full?500u:0u);ops.publication.index=index;
 studioShaderViewportSubmitCandidate(live,cache,1,ops);check(ops.state.block==(crosses?replacement.data():previous.data()));command[5]=crosses?1u:0u;command[6]=static_cast<unsigned>((ops.state.cursor-ops.state.block)*4);command[7]=ops.state.remaining;
 for(unsigned i=0;i<500;++i){if(crosses&&(i==index||(recursivePool&&i==(index+1)%500))){check(ops.publication.ring[i]==reinterpret_cast<uintptr_t>(previous.data()));ring[i]=1;}else ring[i]=static_cast<unsigned>(ops.publication.ring[i]);}ring[0x8d0/4]=ops.publication.produced;ring[0x8d4/4]=ops.publication.index;ring[0x9d8/4]=ops.publication.completed;
#endif
 check(cache==live&&previous==expectedPrevious&&replacement==expectedReplacement&&ring==expectedRing&&pool==expectedPool&&occupied==expectedOccupied&&waits==(crosses?(recursivePool?2u:full?1u:0u):0u));check(command[6]==(crosses?32u:36u)&&command[7]==(crosses?4088u:capacity-7));
 std::array<unsigned,8853> record{};auto cursor=record.begin();cursor=std::copy(previous.begin(),previous.end(),cursor);cursor=std::copy(replacement.begin(),replacement.end(),cursor);cursor=std::copy(command.begin(),command.end(),cursor);cursor=std::copy(ring.begin(),ring.end(),cursor);cursor=std::copy(live.begin(),live.end(),cursor);cursor=std::copy(cache.begin(),cache.end(),cursor);*cursor++=waits;check(cursor==record.end());
#ifdef _M_IX86
 records.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<unsigned,8853> original{};records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(original==record);
#endif
 check(bool(records));++cases;
 }
#ifdef _M_IX86
 check(VirtualFree(code,0,MEM_RELEASE)!=0);
#else
 check(records.peek()==std::char_traits<char>::eof());
#endif
 std::printf("viewport_refill_chain_cases=%u passed\n",cases);
}

