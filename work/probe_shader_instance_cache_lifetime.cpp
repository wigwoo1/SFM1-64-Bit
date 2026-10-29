// Whole original command finalizer and per-instance deleting destructor.
// Allocator virtual services are explicit controlled observers. No live SFM.
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
struct Query{uint32_t serial,kind,capacity,n0,n1,flags0,flags1,destruction,slot,poison,seed,failure;};
struct Entry{uint32_t table;uint32_t*data;uint32_t capacity;};
struct Var{uint32_t pad[2],flags;};struct Object{uintptr_t table;};
struct Block{uint8_t*raw;uint8_t*data;uint32_t size,id;bool alive;};
static Query q;static uint8_t*imageBase;static std::vector<Block>blocks;
static std::array<Entry*,4>slots;static uint32_t allocations=0,releases=0;
static std::vector<std::array<uint32_t,9>>events;
static std::vector<uint32_t>states;static Object allocator;
static void put(size_t at,uintptr_t x){uint32_t v=uint32_t(x);std::memcpy(imageBase+at,&v,4);}
static uint32_t id(const void*p){if(!p)return 0;for(const auto&b:blocks)if(p==b.data)return b.id;throw std::runtime_error("foreign pointer");}
static Block&block(const void*p){auto i=id(p);need(i>0,"nonnull block");return blocks.at(i-1);}
static uint32_t table(const Entry*p){if(!p)return 0;need(p->table==0x1009ea58||p->table==0x1009ea50,"original context vtable");return p->table==0x1009ea58?2u:1u;}
static void record(uint32_t kind,uint32_t ident,uint32_t size){auto*p=slots[q.slot];events.push_back({kind,ident,size,id(p),table(p),p?id(p->data):0,p?p->capacity:0,imageBase[0xdb960],allocations});}
static void*make(uint32_t n){need(n&&n<4096&&!(n&3),"bounded word allocation");auto*raw=static_cast<uint8_t*>(HeapAlloc(GetProcessHeap(),0,n+32));need(raw!=nullptr,"host heap");std::memset(raw,0x6b,n+32);auto*data=raw+16;for(unsigned i=0;i<n/4;++i)std::memcpy(data+4*i,&q.poison,4);blocks.push_back({raw,data,n,uint32_t(blocks.size()+1),true});return data;}
static void* __fastcall allocate(void*self,void*,uint32_t n){need(self==&allocator,"actual allocator receiver");++allocations;if(q.failure==allocations){record(3,0,n);return nullptr;}auto*p=make(n);record(1,id(p),n);return p;}
static void __fastcall release(void*self,void*,void*p){need(self==&allocator&&p,"actual release receiver/pointer");auto&b=block(p);need(b.alive,"double release refused by reference service");record(2,b.id,b.size);b.alive=false;++releases;}
static void snapshot(){auto*p=slots[q.slot];const uint32_t words=p&&p->data&&block(p->data).alive?p->capacity/4:0;
 const uint32_t s[]{id(p),table(p),p?id(p->data):0,p?p->capacity:0,p?uint32_t(block(p).alive):0,p&&p->data?uint32_t(block(p->data).alive):0,imageBase[0xdb960],words};states.insert(states.end(),std::begin(s),std::end(s));
 if(words){need(words<=128,"snapshot bounds");states.insert(states.end(),p->data,p->data+words);}
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;
 if(q.failure&&p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION){auto*e=slots[q.slot];std::cerr<<"EXPECTED_UNSAFE_NULL_ALLOCATION allocation="<<allocations<<" releases="<<releases<<" eip="<<std::hex<<p->ContextRecord->Eip<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::dec<<" slot="<<id(e)<<" capacity="<<(e?e->capacity:0)<<" recording="<<unsigned(imageBase[0xdb960])<<'\n';ExitProcess(238);}
 std::cerr<<"REFERENCE_FAULT case="<<q.serial<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
int main(int argc,char**argv){try{need(argc==4&&!std::ifstream(argv[3]).good(),"original query fresh output");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 NativePrivateWorldImageFixture image(argv[1],0xf0000);imageBase=image.base;auto handler=AddVectoredExceptionHandler(0,&fault);
 std::array<uintptr_t,6>methods{};methods[1]=reinterpret_cast<uintptr_t>(&allocate);methods[5]=reinterpret_cast<uintptr_t>(&release);allocator.table=reinterpret_cast<uintptr_t>(methods.data());Object*allocatorCell=&allocator;put(0x89100,reinterpret_cast<uintptr_t>(&allocatorCell));
 Var flags{};std::array<Var*,3>parameters{&flags,&flags,&flags};put(0xdb944,reinterpret_cast<uintptr_t>(parameters.data()));put(0xdb95c,reinterpret_cast<uintptr_t>(slots.data()));image.seal();
 std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"IO");uint32_t cases=0,totalAlloc=0,totalRelease=0,totalEvents=0;
 while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&q.kind<3&&q.slot<4&&q.n0<=24&&q.n1<=24&&q.capacity<=64&&q.destruction<4,"query domain");slots={};events.clear();states.clear();allocations=releases=0;
  if(q.kind){auto*p=static_cast<Entry*>(make(12));*p={0x1009ea58,nullptr,0};slots[q.slot]=p;if(q.kind==2&&q.capacity){p->data=static_cast<uint32_t*>(make(q.capacity*4));p->capacity=q.capacity*4;}}
  put(0xdb958,q.slot);
  for(unsigned phase=0;phase<2;++phase){reinterpret_cast<void(__cdecl*)()>(imageBase+0x654a0)();const auto n=phase?q.n1:q.n0;for(unsigned i=0;i<n;++i)put(0xbcf48+4*i,q.seed^(phase<<24)^(i*0x9e3779b9u));put(0xbd148,0x100bcf48+4*n);flags.flags=phase?q.flags1:q.flags0;
   reinterpret_cast<void(__cdecl*)()>(imageBase+0x65980)();snapshot();}
  auto*p=slots[q.slot];using Destroy=Entry*(__thiscall*)(Entry*,uint32_t);need(reinterpret_cast<Destroy>(p->table?*reinterpret_cast<uint32_t*>(p->table):0)(p,q.destruction)==p,"original destructor returned identity");snapshot();
  // This clear is an explicit CALLER action, not a destructor side effect.
  slots[q.slot]=nullptr;snapshot();
  uint32_t alive=0;for(const auto&b:blocks){for(unsigned i=0;i<16;++i)need(b.raw[i]==0x6b&&b.data[b.size+i]==0x6b,"allocation guards");alive+=b.alive;}
  const uint32_t header[]{cases,allocations,releases,alive,uint32_t(states.size()),uint32_t(events.size())};out.write(reinterpret_cast<const char*>(header),sizeof(header));out.write(reinterpret_cast<const char*>(states.data()),std::streamsize(states.size()*4));for(const auto&e:events)out.write(reinterpret_cast<const char*>(e.data()),sizeof(e));
  totalAlloc+=allocations;totalRelease+=releases;totalEvents+=uint32_t(events.size());for(auto&b:blocks)need(HeapFree(GetProcessHeap(),0,b.raw)!=0,"host deferred cleanup");blocks.clear();++cases;
 }
 need(in.eof()&&in.gcount()==0&&cases,"query framing");out.flush();need(bool(out),"trace complete");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
 std::cout<<"PASS cache_lifetime_cases="<<cases<<" allocations="<<totalAlloc<<" releases="<<totalRelease<<" events="<<totalEvents<<" original_code=UNCHANGED_RX deferred_host_cleanup=COMPLETE\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
