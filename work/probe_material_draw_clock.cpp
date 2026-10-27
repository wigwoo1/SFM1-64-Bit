// Whole unchanged original methods. Host proxy callbacks observe dispatch only;
// they are NOT built-in proxy/GPU/renderer implementations. No engine startup.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <float.h>
#include <xmmintrin.h>
#include "native_private_world_image_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&in){uint32_t v=0;need(bool(in.read(reinterpret_cast<char*>(&v),4)),"query word");return v;}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
static inline void field(void*p,size_t offset,uint32_t v){std::memcpy(static_cast<uint8_t*>(p)+offset,&v,4);}
static inline uint32_t load(const void*p,size_t offset){uint32_t v;std::memcpy(&v,static_cast<const uint8_t*>(p)+offset,4);return v;}
#ifdef SFM64_NATIVE_PROXY_DISPATCH
static uint8_t*material=nullptr;static uint32_t value=0,systemCalls=0,cutAfter=0,newCount=0;
struct Proxy{uintptr_t table;uint32_t identity;};
static std::vector<std::array<uint32_t,3>>events;
static std::array<uint32_t,4>bindData{};
static uint32_t __fastcall tracking(void*,void*){++systemCalls;return 0;}
static void __fastcall onBind(void*object,void*,void*data){
 need(data==bindData.data()&&bindData[1]==0x31415926,"exact unchanged opaque bind-data pointer");auto id=static_cast<Proxy*>(object)->identity;auto before=value;value+=id+1;events.push_back({id,before,value});if(cutAfter&&events.size()==cutAfter)material[0x23]=uint8_t(newCount);
}
#endif
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==4,"isolated original x86 only");need(argc==4&&!std::ifstream(argv[3]).good(),"private image query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 NativePrivateWorldImageFixture image(argv[1],0x1000);auto count=read(in);need(count&&count<100000,"case extent");put(out,count);
#ifdef SFM64_NATIVE_PROXY_DISPATCH
 std::array<uintptr_t,145>systemMethods{};systemMethods[144]=reinterpret_cast<uintptr_t>(&tracking);uintptr_t system=reinterpret_cast<uintptr_t>(systemMethods.data());field(image.base,0xfc7b0,uint32_t(reinterpret_cast<uintptr_t>(&system)));
 std::array<uintptr_t,2>methods{};methods[1]=reinterpret_cast<uintptr_t>(&onBind);image.seal();
 for(unsigned ci=0;ci<count;++ci){auto mode=read(in),n=read(in);cutAfter=read(in);newCount=read(in);value=read(in);need(n<=255&&mode<=255&&mode!=2&&mode!=3&&newCount<=n&&cutAfter<=n,"normal/disabled dispatch extent");std::vector<Proxy>proxies(n);std::vector<Proxy*>order(n);
  for(unsigned i=0;i<n;++i)proxies[i]={reinterpret_cast<uintptr_t>(methods.data()),i};for(unsigned i=0;i<n;++i){auto id=read(in);need(id<n,"proxy index");order[i]=&proxies[id];}
  std::array<uint8_t,0x88>raw{};material=raw.data();raw[0x23]=uint8_t(n);field(raw.data(),0x28,uint32_t(reinterpret_cast<uintptr_t>(order.data())));const auto before=raw;events.clear();events.reserve(n);systemCalls=0;bindData={ci,0x31415926,0xabcdef01,mode};const auto inputData=bindData;image.base[0xfc7f9]=uint8_t(mode);
  using Call=void(__thiscall*)(void*,void*);reinterpret_cast<Call>(0x10004980)(raw.data(),bindData.data());
  auto expectedRaw=before;if(mode==0&&cutAfter&&events.size()>=cutAfter)expectedRaw[0x23]=uint8_t(newCount);need(raw==expectedRaw&&bindData==inputData&&systemCalls==1,"raw material/data unchanged except explicit host count edit");
  put(out,ci);put(out,systemCalls);put(out,raw[0x23]);put(out,value);put(out,uint32_t(events.size()));for(const auto&e:events)for(auto v:e)put(out,v);image.verify();
 }
#elif defined(SFM64_NATIVE_TOOL_CLOCK)
 image.seal();unsigned original=0,ignored=0;_controlfp_s(&original,0,0);_controlfp_s(&ignored,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);_mm_setcsr(0x1f80);
 for(unsigned ci=0;ci<count;++ci){auto current=read(in),frame=read(in),replacement=read(in);std::array<uint32_t,16>seed;for(unsigned i=0;i<seed.size();++i)seed[i]=0x12340000u+i;seed[3]=current;seed[4]=frame;std::memcpy(image.base+0x3ff7ac,seed.data(),sizeof(seed));
  using Set=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Set>(0x101e57b0)(nullptr,replacement);
  auto expected=seed;expected[4]=replacement;need(std::memcmp(image.base+0x3ff7ac,expected.data(),sizeof(expected))==0,"slot50 changes only frame interval, NOT current time");
  using Get=float(__thiscall*)(void*);float a=reinterpret_cast<Get>(0x101e5780)(nullptr),b=reinterpret_cast<Get>(0x101e5790)(nullptr);uint32_t ga,gb;std::memcpy(&ga,&a,4);std::memcpy(&gb,&b,4);
  put(out,ci);put(out,load(image.base,0x3ff7b8));put(out,load(image.base,0x3ff7bc));put(out,ga);put(out,gb);image.verify();
 }
 _controlfp_s(&ignored,original,_MCW_RC|_MCW_PC);
#else
#error Select an original reference mode
#endif
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"output complete");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image retired");std::cout<<"PASS original_method_cases="<<count<<" executable_bytes=UNCHANGED tracking_branch=NOT_TESTED GPU=NOT_RUN\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
