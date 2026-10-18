#include "source_leaf_displacement_policy_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_LEAF_DISP_POLICY
#define NOMINMAX
#include <windows.h>
#include "native_leaf_policy_bytes.h"
#include "native_displacement_bounds_gate_bytes.h"
#include "native_leaf_displacement_policy_bytes.h"
static uintptr_t activeRuntime=0;static std::array<uint32_t,4>actions{},modes{};static std::vector<uint32_t>nativeCalls;
static std::array<float,3>normalFor(uint32_t mode){switch(mode&3){case 0:return {0,0,1};case 1:return {0,0,-1};case 2:return {1,0,0};default:return {-1,0,0};}}
static void __fastcall controlledDisplacement(void*work,void*runtime,float,float){const uint32_t index=uint32_t((reinterpret_cast<uintptr_t>(runtime)-activeRuntime)/244);nativeCalls.push_back(index);const auto action=actions.at(index);if(!action)return;auto b=static_cast<uint8_t*>(work);const float fraction=action==1?.5f:action==2?0.f:.25f;const uint32_t marker=1;const auto normal=normalFor(modes.at(index));std::memcpy(b+0x80,&fraction,4);std::memcpy(b+0x114,&marker,4);std::memcpy(b+0x6c,normal.data(),12);}
#else
static std::array<float,3>normalFor(uint32_t mode){switch(mode&3){case 0:return {0,0,1};case 1:return {0,0,-1};case 2:return {1,0,0};default:return {-1,0,0};}}
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_LEAF_DISP_POLICY
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");std::memcpy(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes));std::memcpy(base+0x14e790,nativeDispBoundsBytes,sizeof(nativeDispBoundsBytes));std::memcpy(base+0x14cfa0,nativeDispPostBytes,sizeof(nativeDispPostBytes));for(const auto&c:dispBoundsConstants)std::memcpy(base+c.offset,c.bytes,c.size);const float epsilon=.03125f;std::memcpy(base+0x33fa60,&epsilon,4);const uint32_t callback=reinterpret_cast<uint32_t>(&controlledDisplacement);uint8_t jump[7]={0xb8,0,0,0,0,0xff,0xe0};std::memcpy(jump+1,&callback,4);std::memcpy(base+0x14d010,jump,7);DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x6a0000);using Function=void(__fastcall*)(void*,uint32_t,float,float);auto native=reinterpret_cast<Function>(base+0x148270);
#endif
 std::array<uint32_t,51>q{};size_t cases=0;while(in.read(reinterpret_cast<char*>(q.data()),204)){
  std::vector<uint32_t>ordered,visited(4);std::vector<SourceLeafDisplacementCullCandidate>culls(4);std::vector<uint32_t>observed;SourceLeafDisplacementStateCandidate state;std::memcpy(&state.fraction,&q[7],4);state.startSolid=q[2]!=0;state.allSolid=q[3]!=0;state.displacementHit=q[4]!=0;state.normal=normalFor(q[10]);
  for(size_t i=0;i<8;++i){const uint32_t id=(q[11+i/4]>>(8*(i%4)))&255;if(id==255)break;if(id>3)throw std::runtime_error("fixture displacement reference");ordered.push_back(id);}
  for(size_t i=0;i<4;++i){std::memcpy(culls[i].mins.data(),q.data()+19+8*i,12);std::memcpy(culls[i].maxs.data(),q.data()+22+8*i,12);culls[i].contents=q[25+8*i];culls[i].visitIndex=q[26+8*i];if(culls[i].visitIndex>=4)throw std::runtime_error("fixture generation index");visited[i]=(q[1]&(1u<<i))?q[6]:(q[6]^0x12345u);}
  std::array<float,3>start{},delta{},inverse{};std::memcpy(start.data(),q.data()+13,12);std::memcpy(delta.data(),q.data()+16,12);for(size_t a=0;a<3;++a)inverse[a]=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];
#ifdef SFM64_NATIVE_LEAF_DISP_POLICY
  std::array<uint8_t,0x220>world{},work{};std::array<uint16_t,8>leaf{},refs{};std::array<std::array<uint8_t,48>,4>nativeCulls{};std::array<std::array<uint8_t,244>,4>runtimes{};std::array<uint32_t,2>dummyBrush{};uint16_t dummyBrushRef=0;uint32_t dummyBrushVisit=q[6]^0x12345u;
  auto word=[](auto&b,size_t at,uint32_t value){std::memcpy(b.data()+at,&value,4);};auto pointer=[&](auto&b,size_t at,const void*p){word(b,at,reinterpret_cast<uint32_t>(p));};leaf[7]=uint16_t(ordered.size());for(size_t i=0;i<ordered.size();++i)refs[i]=uint16_t(ordered[i]);
  for(size_t i=0;i<4;++i){std::memcpy(nativeCulls[i].data(),culls[i].mins.data(),12);std::memcpy(nativeCulls[i].data()+16,culls[i].maxs.data(),12);word(nativeCulls[i],32,culls[i].visitIndex);word(nativeCulls[i],36,culls[i].contents);actions[i]=(q[8]>>(2*i))&3;modes[i]=(q[9]>>(2*i))&3;}
  if(q[5]&2u)leaf[5]=1;pointer(world,0x98,&dummyBrushRef);pointer(world,0xa8,dummyBrush.data());pointer(work,0x128,&dummyBrushVisit);pointer(world,0x88,leaf.data());pointer(world,0xb0,refs.data());pointer(work,0x104,world.data());pointer(work,0x150,visited.data());word(work,0x120,q[6]);word(work,0xfc,q[0]);word(work,0x114,q[4]);word(work,0x80,q[7]);work[0x8b]=uint8_t(state.startSolid);work[0x8a]=uint8_t(state.allSolid);work[0x101]=uint8_t((q[5]&1u)!=0);std::memcpy(work.data(),start.data(),12);std::memcpy(work.data()+0x3c,delta.data(),12);std::memcpy(work.data()+0x48,inverse.data(),12);std::memcpy(work.data()+0x6c,state.normal.data(),12);
  const uint32_t cullPointer=reinterpret_cast<uint32_t>(nativeCulls.data()),runtimePointer=reinterpret_cast<uint32_t>(runtimes.data());std::memcpy(base+0x637824,&cullPointer,4);std::memcpy(base+0x637820,&runtimePointer,4);activeRuntime=reinterpret_cast<uintptr_t>(runtimes.data());nativeCalls.clear();native(work.data(),0,0,1);observed=nativeCalls;std::memcpy(&state.fraction,work.data()+0x80,4);state.startSolid=work[0x8b]!=0;state.allSolid=work[0x8a]!=0;uint32_t marker=0;std::memcpy(&marker,work.data()+0x114,4);state.displacementHit=marker!=0;std::memcpy(state.normal.data(),work.data()+0x6c,12);
#else
  sourceLeafDisplacementPolicyCandidate(ordered,culls,visited,q[6],q[0],(q[5]&1u)!=0,(q[5]&2u)!=0,state,start,delta,inverse,[&](uint32_t id,auto&s){observed.push_back(id);const uint32_t action=(q[8]>>(2*id))&3;if(!action)return;s.fraction=action==1?.5f:action==2?0.f:.25f;s.displacementHit=true;s.normal=normalFor((q[9]>>(2*id))&3);});
#endif
  std::array<uint32_t,20>result;result.fill(UINT32_MAX);result[0]=uint32_t(observed.size());if(observed.size()>8)throw std::runtime_error("callback capacity");for(size_t i=0;i<observed.size();++i)result[i+1]=observed[i];for(size_t i=0;i<4;++i)result[9+i]=visited[i];std::memcpy(&result[13],&state.fraction,4);result[14]=state.startSolid;result[15]=state.allSolid;result[16]=state.displacementHit;std::memcpy(result.data()+17,state.normal.data(),12);out.write(reinterpret_cast<const char*>(result.data()),80);++cases;
 }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_LEAF_DISP_POLICY
 if(std::memcmp(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes))||std::memcmp(base+0x14e790,nativeDispBoundsBytes,sizeof(nativeDispBoundsBytes))||std::memcmp(base+0x14cfa0,nativeDispPostBytes,sizeof(nativeDispPostBytes)))throw std::runtime_error("original policy code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS leaf displacement cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
