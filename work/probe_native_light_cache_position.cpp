#include "source_light_cache_position_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#ifdef SFM64_NATIVE_POSITION_ORACLE
#include <windows.h>
#include "native_light_cache_cell_bytes.h"
#include "native_light_cache_position_bytes.h"
#endif
using Point=std::array<float,3>;
struct World {
 Point point{};int32_t leaf=7;uint32_t content=0;float first=1,second=1,move=0;bool solid1=false,solid2=false;
 size_t traceCount=0;std::vector<uint32_t>events;
 void word(uint32_t value){events.push_back(value);}
 void scalar(float value){uint32_t bits;std::memcpy(&bits,&value,4);word(bits);}
 void vector(const Point&p){for(float v:p)scalar(v);}
 int32_t leafAt(const Point&p){word(1);vector(p);return leaf;}
 uint32_t contents(int32_t l){word(2);word(uint32_t(l));return content;}
 void constrainToStartLeaf(const Point&start,float tolerance,Point&target){word(3);vector(start);scalar(tolerance);vector(target);target[0]+=move;vector(target);}
 SourceLightCacheTraceCandidate traceDelta(const Point&start,const Point&delta,uint32_t mask){word(4);vector(start);vector(delta);word(mask);++traceCount;return traceCount==1?SourceLightCacheTraceCandidate{first,solid1}:SourceLightCacheTraceCandidate{second,solid2};}
 SourceLightCacheTraceCandidate trace(const Point&start,const Point&end,uint32_t mask){Point delta{};for(size_t i=0;i<3;++i)delta[i]=end[i]-start[i];return traceDelta(start,delta,mask);}
};
#ifdef SFM64_NATIVE_POSITION_ORACLE
static World*active=nullptr;static bool badDependency=false;
static int __cdecl leafHook(const float*p){Point point;std::memcpy(point.data(),p,12);return active->leafAt(point);}
static uint32_t __cdecl contentsHook(int32_t leaf){return active->contents(leaf);}
static void __cdecl clipHook(const float*start,float tolerance,float*target){Point a,b;std::memcpy(a.data(),start,12);std::memcpy(b.data(),target,12);active->constrainToStartLeaf(a,tolerance,b);std::memcpy(target,b.data(),12);}
static void __fastcall securityHook(uint32_t cookie,void*){if(cookie!=0x13572468u)badDependency=true;}
static void __fastcall traceHook(void*,void*,const unsigned char*ray,uint32_t mask,void*,unsigned char*out){
 Point start,delta;std::memcpy(start.data(),ray,12);std::memcpy(delta.data(),ray+16,12);
 const auto result=active->traceDelta(start,delta,mask);std::memcpy(out+0x2c,&result.fraction,4);out[0x37]=uint8_t(result.startSolid);
}
static void jump(unsigned char*base,uint32_t va,const void*target){auto p=base+va-0x10000000;p[0]=0xe9;const int32_t delta=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p+5));std::memcpy(p+1,&delta,4);}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: probe workspace-output.bin");
 std::ofstream output(argv[1],std::ios::binary);if(!output)throw std::runtime_error("test output open failed");
#ifdef SFM64_NATIVE_POSITION_ORACLE
 auto base=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6d0000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("private oracle base unavailable");
 std::memcpy(base+0x1947b0,nativePositionBytes,sizeof(nativePositionBytes));std::memcpy(base+0x192950,nativeCellBytes,sizeof(nativeCellBytes));std::memcpy(base+0x2ded50,nativeTruncateBytes,sizeof(nativeTruncateBytes));
 std::memcpy(base+0x3593ec,nativeWidthXY,4);std::memcpy(base+0x322e30,nativeWidthZ,4);
 const float one=1,half=0.5f;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x31cb50,&half,4);
 *reinterpret_cast<uint32_t*>(base+0x6c1228)=1;*reinterpret_cast<uint32_t*>(base+0x415d00)=0x13572468u;
 jump(base,0x10145660,reinterpret_cast<void*>(leafHook));jump(base,0x10146590,reinterpret_cast<void*>(contentsHook));jump(base,0x101457e0,reinterpret_cast<void*>(clipHook));jump(base,0x102de362,reinterpret_cast<void*>(securityHook));
 uintptr_t traceTable[6]{};traceTable[5]=reinterpret_cast<uintptr_t>(traceHook);uintptr_t traceObject=reinterpret_cast<uintptr_t>(traceTable);const auto objectPointer=reinterpret_cast<uintptr_t>(&traceObject);std::memcpy(base+0x407e90,&objectPointer,4);
 DWORD old=0;for(size_t offset:{size_t(0x140000),size_t(0x190000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("oracle protection failed");
 FlushInstructionCache(GetCurrentProcess(),base,0x6d0000);
 using Function=void(__cdecl*)(unsigned char*,const float*,int32_t);const auto native=reinterpret_cast<Function>(base+0x1947b0);
#endif
 uint32_t count=0;
 for(Point point:{Point{1,2,3},Point{0,0,0},Point{16,16,64},Point{-16,-16,-64},Point{-32,-32,-128},Point{32,32,128},Point{-2026.15625f,-4541.78125f,-351.90625f},Point{-299.5f,-2278.5f,-239.9375f}})
 for(bool different:{false,true})for(uint32_t content:{0u,1u,0x80u,0x4000u,0x4081u,2u})for(bool solid1:{false,true})for(float first:{0.0f,0.5f,1.0f})for(float second:{0.0f,1.0f})for(bool solid2:{false,true})for(float move:{0.0f,0.25f}){
  World world;world.point=point;world.leaf=different?8:7;world.content=content;world.first=first;world.second=second;world.solid1=solid1;world.solid2=solid2;world.move=move;Point result{};
#ifdef SFM64_NATIVE_POSITION_ORACLE
  active=&world;std::array<unsigned char,0x1e0>record;record.fill(0xa5);const auto originalPoint=point;native(record.data(),point.data(),7);std::memcpy(result.data(),record.data()+0x198,12);
  for(size_t i=0;i<record.size();++i)if((i<0x198||i>=0x1a4)&&record[i]!=0xa5)throw std::runtime_error("original routine modified unrelated record bytes");
  if(point!=originalPoint)throw std::runtime_error("original routine modified query input");
  if(badDependency)throw std::runtime_error("native security cookie contract differs");
#else
  result=sourceLightCachePositionCandidate(point,7,world);
#endif
  world.word(5);world.vector(result);const uint32_t words=uint32_t(world.events.size());output.write(reinterpret_cast<const char*>(&words),4);output.write(reinterpret_cast<const char*>(world.events.data()),words*4);++count;
 }
 output.flush();if(!output)throw std::runtime_error("oracle result write failed");
#ifdef SFM64_NATIVE_POSITION_ORACLE
 if(std::memcmp(base+0x1947b0,nativePositionBytes,sizeof(nativePositionBytes))||std::memcmp(base+0x192950,nativeCellBytes,sizeof(nativeCellBytes)))throw std::runtime_error("oracle routine bytes changed during execution");
 VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<" synthetic_world_queries=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
