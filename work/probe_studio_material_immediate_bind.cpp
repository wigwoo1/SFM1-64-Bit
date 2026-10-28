// Same observer corpus for whole original x86 Bind/base helper and owned x64
// protocol. Native imports are explicit observers; no SFM/DllMain/GPU access.
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "studio_material_immediate_bind_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Event{uint32_t kind,id,data;};
struct Query{uint32_t supplied,defaults,mode,debug,cached,current,mutations,forward,token;};
struct Material{uintptr_t table;uint32_t id,value;};
static std::vector<Event>events;
static std::array<Material,3>materials;
static Query q;
static uint32_t checks=0,realtimeCalls=0;
static uintptr_t current=0,proxy=0,defaultId=0;
static void event(uint32_t kind,uint32_t id=0,uint32_t data=0){events.push_back({kind,id,data});}
static Material&mat(uintptr_t p){for(auto&m:materials)if(reinterpret_cast<uintptr_t>(&m)==p)return m;throw std::runtime_error("unknown material identity");}
static uintptr_t address(size_t i){return reinterpret_cast<uintptr_t>(&materials.at(i));}
#ifndef _WIN64
static std::array<uint8_t,0x1d8>context;
static uintptr_t contextAddress=0,systemAddress=0,shaderAddress=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void contextWord(size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(context.data()+at,&w,4);}
static uintptr_t contextWord(size_t at){uint32_t w;std::memcpy(&w,context.data()+at,4);return w;}
#endif
static uintptr_t getCurrent(){
#ifndef _WIN64
 return contextWord(0xc);
#else
 return current;
#endif
}
static void setCurrent(uintptr_t m){
#ifndef _WIN64
 contextWord(0xc,m);
#else
 current=m;
#endif
}
static void nullDiagnostic(bool base){event(base?2:1);}
static uintptr_t realtime(uintptr_t p){auto&m=mat(p);event(3,m.id,++realtimeCalls);return q.forward&&m.id==1?address(1):p;}
static void debugMaterial(uintptr_t p){event(4,mat(p).id);}
static bool precached(uintptr_t p){event(5,mat(p).id,q.cached);return q.cached!=0;}
static const char*name(uintptr_t p){event(6,mat(p).id);return "synthetic-material";}
static void warning(const char*p){need(std::strcmp(p,"synthetic-material")==0,"name preserved through warning");event(7);}
static void precache(uintptr_t p){event(8,mat(p).id);}
static void refresh(uintptr_t p){event(9,mat(p).id);}
static void flush(){event(10,mat(getCurrent()).id);if(q.mutations&1u)setCurrent(address(2));}
static uint32_t threadMode(){event(11,0,q.mode);if(q.mutations&2u)setCurrent(address(1));return q.mode;}
static void callProxies(uintptr_t p,uintptr_t data){need(data==q.token,"full proxy data preserved");auto&m=mat(p);event(12,m.id,m.value);m.value+=17;if(q.mutations&4u)setCurrent(address(0));++checks;}
static void shaderBind(uintptr_t p){auto&m=mat(p);event(13,m.id,m.value);}
#ifdef _WIN64
struct Ops{
 uintptr_t defaultMaterial(){return defaultId;}bool debugEnabled(){return q.debug!=0;}
 void nullDiagnostic(bool base){::nullDiagnostic(base);}uintptr_t realTimeMaterial(uintptr_t p){return realtime(p);}
 void debugMaterial(uintptr_t p){::debugMaterial(p);}uintptr_t currentMaterial(){return getCurrent();}
 bool precached(uintptr_t p){return ::precached(p);}const char*materialName(uintptr_t p){return name(p);}
 void uncachedDiagnostic(const char*p){warning(p);}void precache(uintptr_t p){::precache(p);}void refresh(uintptr_t p){::refresh(p);}
 void setMaterial(uintptr_t p){setCurrent(p);}void setProxyData(uintptr_t p){proxy=p;}
 void flush(){::flush();}uint32_t threadMode(){return ::threadMode();}void callProxies(uintptr_t p,uintptr_t d){::callProxies(p,d);}void shaderBind(uintptr_t p){::shaderBind(p);}
};
// Explicit pointer-width check outside the32bit shared corpus. The same helper
// must pass opaque IDs/data unchanged; this Ops has no fixture object pointers.
static void wideCheck(){struct Wide{uintptr_t value=0,token=0;unsigned draws=0;uintptr_t defaultMaterial(){return 0;}void nullDiagnostic(bool){}uintptr_t realTimeMaterial(uintptr_t p){return p;}bool debugEnabled(){return false;}void debugMaterial(uintptr_t){}uintptr_t currentMaterial(){return value;}bool precached(uintptr_t){return true;}const char*materialName(uintptr_t){return "";}void uncachedDiagnostic(const char*){}void precache(uintptr_t){}void refresh(uintptr_t){}void setMaterial(uintptr_t p){value=p;}void setProxyData(uintptr_t p){token=p;}void flush(){}uint32_t threadMode(){return 0;}void callProxies(uintptr_t p,uintptr_t d){need(p==0xfedcba9876543210ull&&d==0x123456789abcdef0ull,"wide callback identity");}void shaderBind(uintptr_t p){need(p==0xfedcba9876543210ull,"wide shader identity");++draws;}}ops;studioMaterialImmediateBindCandidate(uintptr_t(0xfedcba9876543210ull),uintptr_t(0x123456789abcdef0ull),ops);need(ops.token==0x123456789abcdef0ull&&ops.draws==1,"wide state identity");}
#else
static uintptr_t __fastcall realTimeNative(void*p,void*){return realtime(reinterpret_cast<uintptr_t>(p));}
static void __fastcall debugNative(void*p,void*){debugMaterial(reinterpret_cast<uintptr_t>(p));}
static bool __fastcall cachedNative(void*p,void*){return precached(reinterpret_cast<uintptr_t>(p));}
static const char*__fastcall nameNative(void*p,void*){return name(reinterpret_cast<uintptr_t>(p));}
static void __fastcall precacheNative(void*p,void*){precache(reinterpret_cast<uintptr_t>(p));}
static void __fastcall refreshNative(void*p,void*){refresh(reinterpret_cast<uintptr_t>(p));}
static void __fastcall flushNative(void*p,void*){need(reinterpret_cast<uintptr_t>(p)==contextAddress,"flush receiver");flush();}
static uint32_t __fastcall modeNative(void*p,void*){need(reinterpret_cast<uintptr_t>(p)==systemAddress,"system receiver");return threadMode();}
static void __fastcall proxyNative(void*p,void*,void*d){callProxies(reinterpret_cast<uintptr_t>(p),reinterpret_cast<uintptr_t>(d));}
static void __fastcall shaderNative(void*p,void*,void*m){need(reinterpret_cast<uintptr_t>(p)==shaderAddress,"shader receiver");shaderBind(reinterpret_cast<uintptr_t>(m));}
static void __cdecl nullNative(const char*fmt){auto at=reinterpret_cast<uintptr_t>(fmt);need(at==0x1009a1d4||at==0x1009a194,"native null diagnostic format");nullDiagnostic(at==0x1009a194);}
static void __cdecl warningNative(const char*fmt,const char*p){need(reinterpret_cast<uintptr_t>(fmt)==0x1009a150,"native uncached format");warning(p);}
#endif
static uint32_t randomWord(uint32_t&s){s=s*1664525u+1013904223u;return s;}
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==2&&!std::ifstream(argv[1]).good(),"fresh owned trace path");wideCheck();const char*outputPath=argv[1];
#else
 need(argc==3&&!std::ifstream(argv[2]).good(),"read-only material image and fresh native trace path");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);const char*outputPath=argv[2];NativePrivateWorldImageFixture image(argv[1],0x100000);
 std::array<uintptr_t,94>materialMethods{};materialMethods[93]=reinterpret_cast<uintptr_t>(&realTimeNative);materialMethods[74]=reinterpret_cast<uintptr_t>(&debugNative);materialMethods[69]=reinterpret_cast<uintptr_t>(&cachedNative);materialMethods[0]=reinterpret_cast<uintptr_t>(&nameNative);materialMethods[86]=reinterpret_cast<uintptr_t>(&precacheNative);materialMethods[61]=reinterpret_cast<uintptr_t>(&refreshNative);materialMethods[53]=reinterpret_cast<uintptr_t>(&proxyNative);
 std::array<uintptr_t,222>contextMethods{};contextMethods[198]=0x10009f80;contextMethods[199]=0x1002e490;contextMethods[221]=0x10012ab0;contextMethods[215]=reinterpret_cast<uintptr_t>(&flushNative);
 std::array<uintptr_t,15>systemMethods{};systemMethods[14]=reinterpret_cast<uintptr_t>(&modeNative);uintptr_t system=reinterpret_cast<uintptr_t>(systemMethods.data());systemAddress=reinterpret_cast<uintptr_t>(&system);
 std::array<uintptr_t,84>shaderMethods{};shaderMethods[83]=reinterpret_cast<uintptr_t>(&shaderNative);uintptr_t shader=reinterpret_cast<uintptr_t>(shaderMethods.data());shaderAddress=reinterpret_cast<uintptr_t>(&shader);auto shaderPointer=shaderAddress;std::memcpy(image.base+0x11b098,&shaderPointer,4);
 auto nullPointer=reinterpret_cast<uintptr_t>(&nullNative),warningPointer=reinterpret_cast<uintptr_t>(&warningNative);std::memcpy(image.base+0x951e4,&nullPointer,4);std::memcpy(image.base+0x951b8,&warningPointer,4);image.seal();
#endif
 std::ofstream output(outputPath,std::ios::binary);need(bool(output),"trace output");uint32_t cases=0,eventCount=0,random=0x28202610;
 for(uint32_t supplied=0;supplied<2;++supplied)for(uint32_t defaults=0;defaults<2;++defaults)for(uint32_t mode:{0u,1u,2u,0xffffffffu})for(uint32_t debug=0;debug<2;++debug)for(uint32_t cached=0;cached<2;++cached)for(uint32_t initial=0;initial<3;++initial)for(uint32_t mutations=0;mutations<8;++mutations)for(uint32_t forward=0;forward<2;++forward){
  q={supplied,defaults,mode,debug,cached,initial,mutations,forward,randomWord(random)};events.clear();realtimeCalls=0;for(uint32_t i=0;i<3;++i)materials[i]={0,i+1,randomWord(random)};defaultId=defaults?address(1):0;proxy=0xf0f0f0f0;setCurrent(address(initial));auto passed=supplied?address(0):0;
#ifdef _WIN64
  Ops ops;studioMaterialImmediateBindCandidate(passed,q.token,ops);
#else
  for(auto&m:materials)m.table=reinterpret_cast<uintptr_t>(materialMethods.data());contextWord(0,reinterpret_cast<uintptr_t>(contextMethods.data()));contextWord(0x10,proxy);contextWord(0x1d4,systemAddress);contextAddress=reinterpret_cast<uintptr_t>(context.data());std::memcpy(image.base+0x1059ec,&defaultId,4);image.base[0xfc7ff]=uint8_t(q.debug);using Bind=void(__thiscall*)(void*,void*,void*);reinterpret_cast<Bind>(0x100265e0)(context.data(),reinterpret_cast<void*>(passed),reinterpret_cast<void*>(uintptr_t(q.token)));proxy=contextWord(0x10);image.verify();
#endif
  const bool early=!supplied&&!defaults;need(realtimeCalls==(early?0u:2u)&&proxy==(early?0xf0f0f0f0u:q.token),"two resolution passes and stored proxy data");checks+=2;if(early)need(events.empty(),"default absent no-op");else{need(!events.empty()&&events.back().kind==13,"shader last");need((mode==0)==(std::count_if(events.begin(),events.end(),[](const auto&e){return e.kind==12;})==1),"only zero mode invokes proxies");checks+=2;}
  const uint32_t count=uint32_t(events.size());const uint32_t header[4]{cases,mat(getCurrent()).id,uint32_t(proxy),count};output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(&e),sizeof(e));++cases;eventCount+=count;
 }
 output.flush();need(bool(output),"complete binding trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image released");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS immediate_bind_cases="<<cases<<" events="<<eventCount<<" independent_checks="<<checks<<" material_values_at_shader=LIVE_OBSERVER full_shader_snapshot=UNVERIFIED";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64 wide_ids=PASS\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
