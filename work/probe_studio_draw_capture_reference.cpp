// Private, separate x86 reference test. No original instruction arrays, import
// resolution, DLL entry point, SFM process, GPU or production integration.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "native_private_world_image_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
using Info=std::array<uint8_t,0x1e0>;
using State=std::array<uint8_t,0x254>;
using Packet=std::array<uint8_t,0x45c>;
template<class A>static void put(A&a,size_t at,uint32_t value){need(at+4<=a.size(),"fixture extent");std::memcpy(a.data()+at,&value,4);}
template<class A>static uint32_t get(const A&a,size_t at){uint32_t v;need(at+4<=a.size(),"fixture extent");std::memcpy(&v,a.data()+at,4);return v;}
static uint32_t next(uint32_t&s){s=s*1664525u+1013904223u;return s;}
alignas(16) static std::array<Packet,2>arena;
static std::array<void*,2>queued;
static uint32_t allocations=0,enqueues=0,invocations=0,byteChecks=0,checks=0;
static bool failAllocation=false;
static uintptr_t allocatorIdentity=0,queueIdentity=0;
static std::array<Info,2>wantedInfo;
static std::array<State,2>wantedState;
static std::array<uint32_t,2>wantedScalar,wantedFlags;
static std::array<std::array<uint32_t,2>,2>wantedPair;
static uint32_t sharedValue=0,otherValue=0,wantedShared=0,wantedOther=0;
static void*__fastcall allocate(void*self,void*,uint32_t size){
 need(reinterpret_cast<uintptr_t>(self)==allocatorIdentity&&size==0x45c,"original allocator request");++allocations;if(failAllocation)return nullptr;need(allocations<=2,"bounded allocations");return arena[allocations-1].data();
}
static void __fastcall enqueue(void*self,void*,void*packet){need(reinterpret_cast<uintptr_t>(self)==queueIdentity&&enqueues<2,"queue boundary identity");queued[enqueues++]=packet;}
// Observer of the REAL native invoke method's arguments. Dereferencing these
// synthetic pointer fields here demonstrates shallow copying only; it does
// not claim the original renderer uses these arbitrary fixture offsets.
static void __fastcall observe(void*self,void*,const void*info,const void*state,uint32_t scalar,const uint32_t*pair,uint32_t flags){
 const auto slot=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self)-0x12340000u);need(slot<2,"captured owner");
 need(info==arena[slot].data()+0x18&&state==arena[slot].data()+0x1f8&&pair==reinterpret_cast<const uint32_t*>(arena[slot].data()+0x450),"native internal argument addresses");
 need(std::memcmp(info,wantedInfo[slot].data(),wantedInfo[slot].size())==0&&std::memcmp(state,wantedState[slot].data(),wantedState[slot].size())==0,"complete copied argument bytes");byteChecks+=uint32_t(wantedInfo[slot].size()+wantedState[slot].size());
 need(scalar==wantedScalar[slot]&&flags==wantedFlags[slot]&&pair[0]==wantedPair[slot][0]&&pair[1]==wantedPair[slot][1],"copied scalar/pair values");
 uint32_t pointer=0;std::memcpy(&pointer,static_cast<const uint8_t*>(info)+4,4);need(pointer==reinterpret_cast<uintptr_t>(&sharedValue)&&*reinterpret_cast<const uint32_t*>(pointer)==wantedShared,"first shallow pointer observes later pointee");
 std::memcpy(&pointer,static_cast<const uint8_t*>(state)+0x250,4);need(pointer==reinterpret_cast<uintptr_t>(&otherValue)&&*reinterpret_cast<const uint32_t*>(pointer)==wantedOther,"second shallow pointer observes later pointee");
 ++invocations;checks+=5;
}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==4,"Original x86 reference only");need(argc==3&&!std::ifstream(argv[2]).good(),"read-only studio image and fresh disposable output");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);
 NativePrivateWorldImageFixture image(argv[1],0x100000);
 std::array<uintptr_t,2>allocatorMethods{0,reinterpret_cast<uintptr_t>(&allocate)};uintptr_t allocator=reinterpret_cast<uintptr_t>(allocatorMethods.data());allocatorIdentity=reinterpret_cast<uintptr_t>(&allocator);auto allocatorPointer=allocatorIdentity;auto imported=reinterpret_cast<uintptr_t>(&allocatorPointer);std::memcpy(image.base+0x70118,&imported,4);
 std::array<uintptr_t,1>queueMethods{reinterpret_cast<uintptr_t>(&enqueue)};uintptr_t queue=reinterpret_cast<uintptr_t>(queueMethods.data());queueIdentity=reinterpret_cast<uintptr_t>(&queue);image.seal();
 using Target=void(__fastcall*)(void*,void*,const void*,const void*,uint32_t,const uint32_t*,uint32_t);
 using Submit=void(__thiscall*)(void*,void*,Target,const void*,const void*,const uint32_t*,const uint32_t*,const uint32_t*);
 using Invoke=void(__thiscall*)(void*);auto submit=reinterpret_cast<Submit>(0x10057080);auto invoke=reinterpret_cast<Invoke>(0x100553b0);std::ofstream output(argv[2]);need(bool(output),"fresh reference output");uint32_t random=0x20261028;
 for(uint32_t scenario=0;scenario<512;++scenario){
  allocations=enqueues=invocations=0;failAllocation=false;for(auto&a:arena)a.fill(0xa5);Info info;State state;uint32_t scalar=0,flags=0;std::array<uint32_t,2>pair{};sharedValue=next(random);otherValue=next(random);
  for(uint32_t slot=0;slot<2;++slot){
   for(size_t at=0;at<info.size();at+=4)put(info,at,next(random));for(size_t at=0;at<state.size();at+=4)put(state,at,next(random));
   // Native helper uses x87 load/store for these float fields. Keep this
   // pointer/copy investigation finite and exact; no NaN/FP-mode claim.
   for(size_t at=0x78;at<=0xd4;at+=4)put(state,at,(scenario%8==0?0x80000000u:0x3f000000u|(next(random)&0x807fffffu)));put(state,0x248,scenario%8==0?0u:0x3f000000u|(next(random)&0x807fffffu));
   put(info,4,uint32_t(reinterpret_cast<uintptr_t>(&sharedValue)));put(state,0x250,uint32_t(reinterpret_cast<uintptr_t>(&otherValue)));scalar=next(random);pair={next(random),next(random)};flags=next(random);
   wantedInfo[slot]=info;wantedState[slot]=state;wantedScalar[slot]=scalar;wantedPair[slot]=pair;wantedFlags[slot]=flags;
   submit(&queue,reinterpret_cast<void*>(uintptr_t(0x12340000u+slot)),&observe,info.data(),state.data(),&scalar,pair.data(),&flags);
   need(allocations==slot+1&&enqueues==slot+1&&queued[slot]==arena[slot].data()&&invocations==0,"whole enqueue defers invocation");
   need(get(arena[slot],0)==0x100712b8&&get(arena[slot],8)==0x100712ac&&get(arena[slot],0xc)==1&&get(arena[slot],4)==0xa5a5a5a5,"original functor header and untouched gap");checks+=2;
   info.fill(0xcc);state.fill(0xdd);scalar=flags=0;pair={0,0};
  }
  wantedShared=sharedValue=next(random);wantedOther=otherValue=next(random);const auto before=arena;
  for(uint32_t n=0;n<2;++n)invoke(queued[scenario&1u?1-n:n]);need(invocations==2&&arena==before,"two delayed invokes preserve packet bytes");++checks;image.verify();output<<scenario<<" copied=2152 scalar_words=8 shared_pointers=4 delayed_invokes=2 packet_unchanged=1\n";
 }
 // The original helper forwards null to queue slot0 after failed allocation.
 // Our observer accepts it; this is NOT safe behavior of the real queue/drain.
 allocations=enqueues=invocations=0;failAllocation=true;Info info{};State state{};uint32_t word=0;std::array<uint32_t,2>pair{};submit(&queue,nullptr,&observe,info.data(),state.data(),&word,pair.data(),&word);need(allocations==1&&enqueues==1&&queued[0]==nullptr&&invocations==0,"native null allocation forwards null");++checks;
 output<<"null_allocation forwarded=1 real_queue_recovery=UNVERIFIED\n";output.flush();need(bool(output),"complete record output");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image released");RemoveVectoredExceptionHandler(handler);
 std::cout<<"PASS studio_draw_capture_cases=512 queued_functors=1024 delayed_invocations=1024 copied_bytes_checked="<<byteChecks<<" independent_checks="<<checks<<" null_forwarding=1 original_code=UNCHANGED_RX material_value_snapshot=UNVERIFIED renderer=NOT_RUN\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
