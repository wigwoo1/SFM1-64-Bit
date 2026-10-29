// Whole original snapshot release; context destructors and pool release observed.
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
#include "private_x86_boundary_observer_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t serial,hardware,pattern,passes,salt,nullArray;};
struct Object{uintptr_t table;uint32_t id;};
static Query q;static std::array<std::array<uint32_t,11>,128>rows;
static std::array<Object,1024>objects;static Object hardware;
static std::vector<std::array<uint32_t,4>>events;static uint32_t pool=0;
static uint32_t __fastcall flags(void*self,void*){need(self==&hardware,"hardware receiver");events.push_back({1,q.hardware,0,0});return q.hardware;}
static Object* __fastcall destroy(void*self,void*,uint32_t flag){auto*p=static_cast<Object*>(self);need(p>=objects.data()&&p<objects.data()+objects.size()&&flag==1,"deleting virtual context contract");events.push_back({2,p->id,flag,0});return p;}
static void boundary(unsigned index,CONTEXT&c){need(index==0&&c.Eip==0x100702b0,"declared pool release");auto*stack=reinterpret_cast<uint32_t*>(c.Esp);need(stack[1]==reinterpret_cast<uintptr_t>(rows.data()),"retained snapshot allocation");need(c.Ecx==0x10105868||c.Ecx==0x10105898,"actual snapshot pool");pool=c.Ecx==0x10105868?1u:2u;events.push_back({3,pool,0,0});c.Eip=stack[0];c.Esp+=8;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT case="<<q.serial<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
int main(int argc,char**argv){try{need(argc==4&&!std::ifstream(argv[3]).good(),"material query fresh output");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);NativePrivateWorldImageFixture image(argv[1],0x100000);auto handler=AddVectoredExceptionHandler(0,&fault);
 std::array<uintptr_t,143>hwMethods{};hwMethods[142]=reinterpret_cast<uintptr_t>(&flags);hardware.table=reinterpret_cast<uintptr_t>(hwMethods.data());uint32_t pointer=uint32_t(reinterpret_cast<uintptr_t>(&hardware));std::memcpy(image.base+0xfc7b0,&pointer,4);
 uintptr_t destruction=reinterpret_cast<uintptr_t>(&destroy);for(unsigned i=0;i<objects.size();++i)objects[i]={reinterpret_cast<uintptr_t>(&destruction),i+1};image.seal();PrivateX86BoundaryObserverFixture observer({0x100702b0,0,0,0},&boundary);observer.configure(true);
 std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"IO");uint32_t cases=0,released=0,pools=0;
 while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&q.hardware<4&&q.passes<=4&&q.nullArray<2,"query domain");events.clear();pool=0;const auto count=q.hardware&2?128u:q.hardware&1?32u:16u;
  for(unsigned r=0;r<128;++r){rows[r].fill(q.salt);rows[r][0]=q.passes;for(unsigned p=0;p<4;++p){const auto id=r*8+p*2;const bool active=p<q.passes;rows[r][3+p]=active&&((r+p+q.pattern)&1)?uint32_t(reinterpret_cast<uintptr_t>(&objects[id])):0;rows[r][7+p]=active&&((r*3+p+q.pattern)&2)?uint32_t(reinterpret_cast<uintptr_t>(&objects[id+1])):0;}}
  auto before=rows;using Release=void(__stdcall*)(void*);reinterpret_cast<Release>(image.base+0x2b00)(q.nullArray?nullptr:rows.data());
  if(!q.nullArray){for(unsigned r=0;r<count;++r)for(unsigned p=0;p<q.passes;++p)before[r][3+p]=before[r][7+p]=0;need(pool==(q.hardware&3?2u:1u),"matching pool release");++pools;}
  need(rows==before,"only visited live context cells cleared; data and unused rows preserved");const uint32_t head[]{cases,uint32_t(events.size()),pool};out.write(reinterpret_cast<const char*>(head),sizeof(head));for(const auto&e:events){out.write(reinterpret_cast<const char*>(e.data()),sizeof(e));released+=e[0]==2;}++cases;
 }
 need(in.eof()&&in.gcount()==0&&cases,"query framing");out.flush();need(bool(out),"trace complete");observer.configure(false);need(observer.hits==pools,"exact pool boundaries");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);std::cout<<"PASS snapshot_release_cases="<<cases<<" deleting_calls="<<released<<" pool_releases="<<pools<<" original_code=UNCHANGED_RX\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
