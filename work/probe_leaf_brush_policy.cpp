#include "source_leaf_brush_policy_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef NATIVE_LEAF_POLICY
#define NOMINMAX
#include <windows.h>
#include "native_leaf_policy_bytes.h"
static uint32_t*activeBrushes;static std::array<uint32_t,4>actions;static std::vector<uint32_t>calls;
static void __fastcall controlledBrush(void*work,void*brush){const size_t index=(static_cast<uint32_t*>(brush)-activeBrushes)/2;calls.push_back(uint32_t(index));auto b=static_cast<uint8_t*>(work);if(actions.at(index)==1){float zero=0;std::memcpy(b+0x80,&zero,4);}if(actions.at(index)==2)b[0x8b]=1;if(actions.at(index)==3){float half=.5f;std::memcpy(b+0x80,&half,4);}}
#endif
struct State{float fraction;bool startSolid;};
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_LEAF_POLICY
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");std::memcpy(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes));uint8_t jump[7]={0xb8,0,0,0,0,0xff,0xe0};uint32_t callback=reinterpret_cast<uint32_t>(&controlledBrush);std::memcpy(jump+1,&callback,4);std::memcpy(base+0x147690,jump,7);DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=void(__fastcall*)(void*,uint32_t,float,float);auto native=reinterpret_cast<Function>(base+0x148270);
#endif
 std::array<uint32_t,16>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),64)){
  std::vector<uint32_t>ordered,contents(4),visited(4);std::vector<uint32_t>observed;State state;std::memcpy(&state.fraction,&q[5],4);state.startSolid=q[4]!=0;
  for(size_t i=0;i<4;++i){contents[i]=q[6+i];visited[i]=(q[1]&(1u<<i))?7u:3u;}
  for(size_t i=0;i<8;++i){uint32_t index=(q[14+i/4]>>(8*(i%4)))&255;if(index==255)break;if(index>3)throw std::runtime_error("bad fixture index");ordered.push_back(index);}
#ifdef NATIVE_LEAF_POLICY
  std::array<uint8_t,0x220>world{},work{};std::array<uint8_t,16>leaf{};std::array<uint16_t,8>refs{};std::array<uint32_t,8>brushes{};std::array<uint8_t,32>sides{},surfaces{};std::array<uint8_t,192>boxes{};
  auto put=[](auto&b,size_t offset,const void*value,size_t size){std::memcpy(b.data()+offset,value,size);};auto pointer=[&](auto&b,size_t offset,const void*value){uint32_t p=reinterpret_cast<uint32_t>(value);put(b,offset,&p,4);};
  uint16_t refCount=uint16_t(ordered.size());put(leaf,10,&refCount,2);for(size_t i=0;i<ordered.size();++i)refs[i]=uint16_t(ordered[i]);
  for(size_t i=0;i<4;++i){brushes[2*i]=contents[i];brushes[2*i+1]=(q[3]&(1u<<i))?uint32_t(65535|(i<<16)):uint32_t(1|(i<<16));uint16_t surface=uint16_t(i);put(sides,8*i+4,&surface,2);surfaces[8*i+6]=(q[2]&(1u<<i))?0x80:0;for(size_t j=0;j<6;++j)put(boxes,48*i+32+2*j,&surface,2);actions[i]=q[10+i];}
  pointer(world,0x88,leaf.data());pointer(world,0x98,refs.data());pointer(world,0xa8,brushes.data());pointer(world,0x68,sides.data());pointer(world,0x70,boxes.data());pointer(world,0x218,surfaces.data());pointer(work,0x104,world.data());pointer(work,0x128,visited.data());uint32_t generation=7;put(work,0x120,&generation,4);put(work,0xfc,&q[0],4);put(work,0x80,&state.fraction,4);work[0x8b]=uint8_t(state.startSolid);activeBrushes=brushes.data();calls.clear();native(work.data(),0,0,1);observed=calls;std::memcpy(&state.fraction,work.data()+0x80,4);state.startSolid=work[0x8b]!=0;
#else
  sourceLeafBrushPolicyCandidate(ordered,contents,visited,7,q[0],state,[&](uint32_t i){return(q[2]&(1u<<i))!=0;},[&](uint32_t i,State&s){observed.push_back(i);if(q[10+i]==1)s.fraction=0;if(q[10+i]==2)s.startSolid=true;if(q[10+i]==3)s.fraction=.5f;});
#endif
  std::array<uint32_t,15>result;result.fill(UINT32_MAX);result[0]=uint32_t(observed.size());for(size_t i=0;i<observed.size();++i)result[1+i]=observed[i];for(size_t i=0;i<4;++i)result[9+i]=visited[i];std::memcpy(&result[13],&state.fraction,4);result[14]=uint32_t(state.startSolid);out.write(reinterpret_cast<char*>(result.data()),60);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_LEAF_POLICY
 if(std::memcmp(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes)))throw std::runtime_error("original body changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
