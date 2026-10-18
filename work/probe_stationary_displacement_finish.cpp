#include "source_prepared_world_ray_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef NATIVE_STATIONARY_FINISH
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::array<uint32_t,11>controlled{};
static void __cdecl controlledProbe(void*p,const float*,const float*,uint32_t){auto w=static_cast<uint8_t*>(p);std::memcpy(w+0x80,&controlled[0],4);std::memcpy(w+0x8c,&controlled[1],4);w[0x8b]=uint8_t(controlled[2]);w[0x8a]=uint8_t(controlled[3]);std::memcpy(w+0x84,&controlled[4],4);std::memcpy(w+0x6c,controlled.data()+5,16);uint16_t flags=uint16_t(controlled[9]);std::memcpy(w+0x88,&flags,2);std::memcpy(w+0x114,&controlled[10],4);}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_STATIONARY_FINISH
 NativePrivateWorldImageFixture image(argv[1]);auto base=image.base;base[0x14ce00]=0xe9;int32_t jump=int32_t(reinterpret_cast<uintptr_t>(&controlledProbe)-(reinterpret_cast<uintptr_t>(base)+0x14ce05));std::memcpy(base+0x14ce01,&jump,4);image.seal();std::array<uint8_t,244>runtime{};uint32_t runtimePointer=reinterpret_cast<uint32_t>(runtime.data());std::memcpy(base+0x637820,&runtimePointer,4);uint32_t meshContents=1;std::memcpy(runtime.data()+0x20,&meshContents,4);std::array<float,3>normal{0,0,1};std::memcpy(runtime.data()+0x60,normal.data(),12);
#endif
 std::array<uint32_t,11>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),44)){
#ifdef NATIVE_STATIONARY_FINISH
 controlled=q;std::array<uint8_t,0x220>work{};work[0x100]=1;uint16_t ref=0;std::array<float,3>point{},extents{};using Function=void(__cdecl*)(void*,const uint16_t*,int,const float*,const float*,const float*,uint32_t,void*);reinterpret_cast<Function>(base+0x14d320)(work.data(),&ref,1,point.data(),extents.data(),extents.data(),1,work.data()+0x54);
 std::memcpy(&q[0],work.data()+0x80,4);std::memcpy(&q[1],work.data()+0x8c,4);q[2]=work[0x8b]!=0;q[3]=work[0x8a]!=0;std::memcpy(&q[4],work.data()+0x84,4);std::memcpy(q.data()+5,work.data()+0x6c,16);uint16_t flags=0;std::memcpy(&flags,work.data()+0x88,2);q[9]=flags;std::memcpy(&q[10],work.data()+0x114,4);
#else
 SourcePreparedWorldRayStateCandidate state;std::memcpy(&state.trace.fraction,&q[0],4);std::memcpy(&state.trace.leftSolid,&q[1],4);state.trace.startSolid=q[2]!=0;state.trace.allSolid=q[3]!=0;state.trace.contents=q[4];std::memcpy(state.trace.plane.data(),q.data()+5,16);state.displacementFlags=uint16_t(q[9]);state.displacementHit=q[10];sourceStationaryDisplacementFinishCandidate(state);std::memcpy(&q[0],&state.trace.fraction,4);std::memcpy(&q[1],&state.trace.leftSolid,4);q[2]=state.trace.startSolid;q[3]=state.trace.allSolid;q[4]=state.trace.contents;std::memcpy(q.data()+5,state.trace.plane.data(),16);q[9]=state.displacementFlags;q[10]=state.displacementHit;
#endif
 out.write(reinterpret_cast<const char*>(q.data()),44);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_STATIONARY_FINISH
 image.verify();VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS stationary displacement finish cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
