#include "source_stationary_leaf_selection_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef DISPLACEMENT_LEAF_ASSIGNMENT
#include "source_displacement_leaf_assignment_candidate.h"
#endif
#ifdef NATIVE_STATIONARY_LEAVES
#define NOMINMAX
#include <windows.h>
#include "native_stationary_leaf_selection_bytes.h"
#ifdef DISPLACEMENT_LEAF_ASSIGNMENT
#include "native_displacement_leaf_assignment_bytes.h"
#include "native_bounds_plane_side_bytes.h"
#endif
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("map queries output required");std::ifstream map(argv[1],std::ios::binary),in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!map||!in||!out)throw std::runtime_error("workspace files");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});SourceBspBrushOnlyTraceCandidate world(b,std::vector<uint32_t>(32768));
#ifdef NATIVE_STATIONARY_LEAVES
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");std::memcpy(base+0x145810,nativeSelectionBytes,sizeof(nativeSelectionBytes));std::memcpy(base+0x2df090,nativeStackProbeBytes,sizeof(nativeStackProbeBytes));std::memcpy(base+0x2de362,nativeCookieCheckBytes,sizeof(nativeCookieCheckBytes));uint32_t sign=0x80000000;std::memcpy(base+0x31cbb0,&sign,4);
#ifdef DISPLACEMENT_LEAF_ASSIGNMENT
 std::memcpy(base+0x14d610,nativeAssignmentBytes,sizeof(nativeAssignmentBytes));std::memcpy(base+0x2446e0,nativeBoundsSideBytes,sizeof(nativeBoundsSideBytes));
#endif
 std::vector<std::array<uint8_t,20>>planes(world.planes.size());for(size_t i=0;i<planes.size();++i){std::memcpy(planes[i].data(),world.planes[i].normal.data(),12);std::memcpy(planes[i].data()+12,&world.planes[i].distance,4);planes[i][16]=world.planes[i].type;for(unsigned axis=0;axis<3;++axis)if(world.planes[i].normal[axis]<0)planes[i][17]|=uint8_t(1u<<axis);}
 std::vector<std::array<uint32_t,3>>nodes;for(const auto&n:world.nodes)nodes.push_back({reinterpret_cast<uint32_t>(planes[n.plane].data()),uint32_t(n.children[0]),uint32_t(n.children[1])});uint32_t nodePtr=reinterpret_cast<uint32_t>(nodes.data());DWORD old;for(size_t offset:{size_t(0x140000),size_t(0x2d0000),size_t(0x240000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=int(__cdecl*)(void*,const float*,const float*,int32_t);auto native=reinterpret_cast<Function>(base+0x145810);(void)native;
#endif
 std::array<float,6>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),24)){std::array<uint32_t,1026>result;result.fill(UINT32_MAX);
#ifdef NATIVE_STATIONARY_LEAVES
#ifdef DISPLACEMENT_LEAF_ASSIGNMENT
 std::array<uint8_t,0xf4>displacement{};std::memcpy(displacement.data()+4,q.data(),12);std::memcpy(displacement.data()+20,q.data()+3,12);uint32_t active=1;std::memcpy(displacement.data()+40,&active,4);uint32_t displacementPtr=reinterpret_cast<uint32_t>(displacement.data());std::memcpy(base+0x637820,&displacementPtr,4);
 std::array<uint8_t,0xa4>runtimeWorld{};std::array<uint8_t,40>model{};std::memcpy(model.data()+36,&world.root,4);std::memcpy(runtimeWorld.data(),&nodePtr,4);uint32_t modelPtr=reinterpret_cast<uint32_t>(model.data());std::memcpy(runtimeWorld.data()+0xa0,&modelPtr,4);
 std::vector<uint16_t>assigned(65536);uint32_t first=0;uint16_t assignedCount=0;std::array<uint32_t,16>context{};context[0]=reinterpret_cast<uint32_t>(runtimeWorld.data());context[1]=reinterpret_cast<uint32_t>(assigned.data());context[2]=65536;context[5]=context[1];context[6]=reinterpret_cast<uint32_t>(&first);context[11]=reinterpret_cast<uint32_t>(&assignedCount);
 using Assign=void(__thiscall*)(void*,uint32_t);auto assign=reinterpret_cast<Assign>(base+0x14d610);assign(context.data(),0);if(context[4]>1024)throw std::runtime_error("test output capacity exceeded");result[0]=context[4];result[1]=assignedCount;for(size_t i=0;i<context[4];++i)result[2+i]=assigned[i];
#else
  struct Record{int32_t top,capacity;uint32_t*leaves;void*world;};Record record{-1,1024,result.data()+2,&nodePtr};int found=native(&record,q.data(),q.data()+3,world.root);if(found<0||found>1024)throw std::runtime_error("native leaf count");result[0]=uint32_t(found);result[1]=uint32_t(record.top);
#endif
#else
#ifdef DISPLACEMENT_LEAF_ASSIGNMENT
 auto assigned=sourceDisplacementLeafAssignmentCandidate(world,{q[0],q[1],q[2]},{q[3],q[4],q[5]});if(assigned.size()>1024)throw std::runtime_error("test output capacity exceeded");result[0]=uint32_t(assigned.size());result[1]=uint32_t(assigned.size());for(size_t i=0;i<assigned.size();++i)result[2+i]=assigned[i];
#else
  auto selected=sourceStationaryLeafSelectionCandidate(world,{q[0],q[1],q[2]},{q[3],q[4],q[5]});result[0]=uint32_t(selected.leaves.size());result[1]=uint32_t(selected.topNode);for(size_t i=0;i<selected.leaves.size();++i)result[2+i]=selected.leaves[i];
#endif
 #endif
  out.write(reinterpret_cast<char*>(result.data()),sizeof(result));++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_STATIONARY_LEAVES
 if(std::memcmp(base+0x145810,nativeSelectionBytes,sizeof(nativeSelectionBytes)))throw std::runtime_error("original body changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
