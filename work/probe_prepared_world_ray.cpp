#include "source_prepared_world_ray_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef STATIONARY_BRUSH_WORLD
#include "source_stationary_brush_only_world_candidate.h"
#endif
#ifdef NATIVE_BRUSH_WORLD
#define NOMINMAX
#include <windows.h>
#include "native_convex_brush_ray_bytes.h"
#include "native_optimized_box_bytes.h"
#include "native_leaf_policy_bytes.h"
#include "native_line_split_bytes.h"
#include "native_displacement_tree_bytes.h"
#include "native_displacement_ray_triangle_bytes.h"
#include "native_displacement_ray_tree_bytes.h"
#include "native_displacement_world_ray_bytes.h"
#include "native_displacement_bounds_gate_bytes.h"
#include "native_leaf_displacement_policy_bytes.h"
#ifdef STATIONARY_BRUSH_WORLD
#include "native_stationary_world_bytes.h"
#endif
#endif
#if defined(STATIONARY_BRUSH_WORLD) && defined(NATIVE_BRUSH_WORLD)
__declspec(naked) static void invokeStationary(void*,void*,void*,int32_t){__asm{
 push ebp
 mov ebp,esp
 push esi
 mov esi,dword ptr [ebp+8]
 push dword ptr [ebp+14h]
 push dword ptr [ebp+10h]
 call dword ptr [ebp+0ch]
 add esp,8
 pop esi
 pop ebp
 ret
}}
#endif
#ifdef NATIVE_BRUSH_WORLD
static size_t activeQuery=0;
static LONG CALLBACK reportFault(EXCEPTION_POINTERS*p){std::cerr<<"native fault query="<<activeQuery<<" eip="<<std::hex<<p->ContextRecord->Eip<<" ecx="<<p->ContextRecord->Ecx<<" edx="<<p->ContextRecord->Edx<<" eax="<<p->ContextRecord->Eax<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
 if(argc!=5)throw std::runtime_error("map prepared-meshes queries output required");std::ifstream map(argv[1],std::ios::binary),in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!map||!in||!out)throw std::runtime_error("workspace files");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});SourceBspBrushOnlyTraceCandidate owned(b,std::vector<uint32_t>(32768));
 std::ifstream meshInput(argv[2],std::ios::binary);if(!meshInput)throw std::runtime_error("prepared mesh file");std::vector<SourceDisplacementRuntimeMeshCandidate>meshes;std::array<uint32_t,3>header{};
 while(meshInput.read(reinterpret_cast<char*>(header.data()),12)){
  auto power=header[0];if(power<1||power>4)throw std::runtime_error("mesh power");size_t cells=size_t(1)<<power,vc=(cells+1)*(cells+1),tc=2*cells*cells;
  std::array<std::array<float,3>,4>corners{};std::vector<std::array<float,3>>positions(vc);std::vector<float>alphas(vc);std::vector<SourceDisplacementGridTriangleCandidate>triangles(tc);
  if(!meshInput.read(reinterpret_cast<char*>(corners.data()),48)||!meshInput.read(reinterpret_cast<char*>(positions.data()),vc*12)||!meshInput.read(reinterpret_cast<char*>(alphas.data()),vc*4)||!meshInput.read(reinterpret_cast<char*>(triangles.data()),tc*8))throw std::runtime_error("truncated prepared mesh");
  meshes.push_back(sourceDisplacementRuntimeMeshCandidate(power,header[1],header[2],corners,positions,alphas,triangles));
 }if(!meshInput.eof()||meshInput.gcount()!=0||meshes.size()!=43)throw std::runtime_error("mesh fixture count");
 SourcePreparedWorldRayCandidate combined(owned,meshes);

#ifdef NATIVE_BRUSH_WORLD
 AddVectoredExceptionHandler(1,reportFault);
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");
 std::memcpy(base+0x147690,nativeBrushBytes,sizeof(nativeBrushBytes));std::memcpy(base+0xe9720,nativePlaneCopyBytes,sizeof(nativePlaneCopyBytes));std::memcpy(base+0x146440,nativeSurfaceBytes,sizeof(nativeSurfaceBytes));std::memcpy(base+0x146850,nativeBoxBytes,sizeof(nativeBoxBytes));std::memcpy(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes));std::memcpy(base+0x148950,nativeLineSplitBytes,sizeof(nativeLineSplitBytes));for(const auto&c:nativeBoxConstants)std::memcpy(base+c.offset,c.bytes,16);float one=1,epsilon=.03125f,sentinel=-99999;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x33fa60,&epsilon,4);std::memcpy(base+0x3423d4,&sentinel,4);
#ifdef STATIONARY_BRUSH_WORLD
 for(const auto&fragment:stationaryWorldFragments)std::memcpy(base+fragment.offset,fragment.bytes,fragment.size);
#endif
 std::vector<std::array<uint8_t,20>>planes(owned.planes.size());for(size_t i=0;i<planes.size();++i){std::memcpy(planes[i].data(),owned.planes[i].normal.data(),12);std::memcpy(planes[i].data()+12,&owned.planes[i].distance,4);planes[i][16]=owned.planes[i].type;}
 struct Side{const uint8_t*plane;uint16_t surface;uint8_t bevel,pad;};static_assert(sizeof(Side)==8,"x86 side");std::vector<Side>sides;for(const auto&s:owned.data.sides)sides.push_back({planes[s.plane].data(),uint16_t(s.texture),uint8_t(s.bevel),0});
 std::vector<std::array<uint32_t,3>>nodes;for(const auto&n:owned.nodes)nodes.push_back({reinterpret_cast<uint32_t>(planes[n.plane].data()),uint32_t(n.children[0]),uint32_t(n.children[1])});
 struct alignas(16) Box{std::array<uint8_t,48>bytes;};std::vector<Box>boxes;std::vector<std::array<uint32_t,2>>brushes;for(size_t i=0;i<owned.data.brushes.size();++i){const auto&brush=owned.data.brushes[i];uint32_t encoded=brush.count|(brush.first<<16);if(owned.records[i].optimized){encoded=65535|(uint32_t(boxes.size())<<16);boxes.push_back({owned.records[i].box});}brushes.push_back({brush.contents,encoded});}
 std::vector<std::array<uint16_t,8>>leaves(owned.data.leafBrushes.size());std::vector<uint16_t>refs;for(size_t i=0;i<leaves.size();++i){
#ifdef STATIONARY_BRUSH_WORLD
 uint32_t leafOffset;std::memcpy(&leafOffset,b.data()+8+10*16,4);std::memcpy(leaves[i].data(),b.data()+leafOffset+32*i,4);
#endif
if(refs.size()>65535)throw std::runtime_error("leaf reference index overflow");leaves[i][4]=uint16_t(refs.size());leaves[i][5]=uint16_t(owned.data.leafBrushes[i].size());for(auto index:owned.data.leafBrushes[i])refs.push_back(uint16_t(index));}
 std::vector<uint8_t>world(0x220);std::vector<uint64_t>surfaces(65536);auto put=[](auto&v,size_t offset,const void*p,size_t size){std::memcpy(v.data()+offset,p,size);};auto pointer=[&](auto&v,size_t offset,const void*p){uint32_t value=reinterpret_cast<uint32_t>(p);put(v,offset,&value,4);};pointer(world,0,nodes.data());pointer(world,0x68,sides.data());pointer(world,0x70,boxes.data());pointer(world,0x88,leaves.data());pointer(world,0x98,refs.data());pointer(world,0xa8,brushes.data());pointer(world,0x218,surfaces.data());

 std::vector<uint16_t>dispRefs;for(size_t i=0;i<leaves.size();++i){const auto&lr=combined.leafRefs()[i];if(dispRefs.size()>65535||lr.size()>65535)throw std::runtime_error("displacement leaf span");leaves[i][6]=uint16_t(dispRefs.size());leaves[i][7]=uint16_t(lr.size());for(auto id:lr)dispRefs.push_back(uint16_t(id));}pointer(world,0xb0,dispRefs.data());
 std::vector<std::array<uint8_t,0xf4>>runtime(meshes.size());struct alignas(16) Cull{std::array<uint8_t,48>bytes;};std::vector<Cull>culls(meshes.size());
 for(size_t i=0;i<meshes.size();++i){const auto&m=meshes[i];pointer(runtime[i],0x70,m.vertices.data());pointer(runtime[i],0x80,m.triangles.data());pointer(runtime[i],0x90,m.tree.nodes.data());pointer(runtime[i],0xa0,m.tree.leafPairs.data());put(runtime[i],0x20,&m.contents,4);put(runtime[i],0x2c,&m.runtimeFlags,4);put(runtime[i],0x28,&m.power,4);uint32_t nc=uint32_t(m.tree.nodes.size());put(runtime[i],0x98,&nc,4);put(culls[i].bytes,0,m.tree.expandedBounds.data(),12);put(culls[i].bytes,0x10,m.tree.expandedBounds.data()+3,12);uint32_t id=uint32_t(i);put(culls[i].bytes,0x20,&id,4);put(culls[i].bytes,0x24,&m.contents,4);}
 {uint32_t a=reinterpret_cast<uint32_t>(runtime.data()),c=reinterpret_cast<uint32_t>(culls.data());std::memcpy(base+0x637820,&a,4);std::memcpy(base+0x637824,&c,4);}
 std::memcpy(base+0x14d010,nativeDispWorldRayBytes,sizeof(nativeDispWorldRayBytes));std::memcpy(base+0x14e790,nativeDispBoundsBytes,sizeof(nativeDispBoundsBytes));std::memcpy(base+0x14cfa0,nativeDispPostBytes,sizeof(nativeDispPostBytes));
 for(const auto&f:treeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:rayTriangleFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:rayTreeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&c:rayTriangleConstants)std::memcpy(base+c.offset,c.bytes,c.size);for(const auto&c:rayTreeConstants)std::memcpy(base+c.offset,c.bytes,c.size);for(const auto&c:dispBoundsConstants)std::memcpy(base+c.offset,c.bytes,c.size);
 DWORD old;for(size_t offset:{size_t(0x140000),size_t(0x150000),size_t(0xe0000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x6a0000);using Function=void(__fastcall*)(void*,int32_t,float,float,const float*,const float*);auto native=reinterpret_cast<Function>(base+0x148950);(void)native;
#endif
 std::array<uint32_t,7>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),28)){std::array<float,3>start,end;std::memcpy(start.data(),q.data(),12);std::memcpy(end.data(),q.data()+3,12);if(q[6]&0x2000)throw std::runtime_error("unsupported surface mask");SourcePreparedWorldRayStateCandidate combinedState;auto&state=combinedState.trace;state.plane={99,98,97,96};
#ifdef NATIVE_BRUSH_WORLD
  activeQuery=count;std::vector<uint8_t>work(0x220);std::vector<uint32_t>visited(brushes.size()),dispVisited(meshes.size());pointer(work,0x150,dispVisited.data());work[0x101]=1;pointer(work,0x104,world.data());pointer(work,0x128,visited.data());uint32_t generation=1;put(work,0x120,&generation,4);put(work,0xfc,&q[6],4);put(work,0,start.data(),12);put(work,12,end.data(),12);put(work,0x80,&state.fraction,4);put(work,0x6c,state.plane.data(),16);work[0x100]=1;for(size_t axis=0;axis<3;++axis){float delta=end[axis]-start[axis],inverse=delta==0?std::numeric_limits<float>::max():1.0f/delta;put(work,0x3c+4*axis,&delta,4);put(work,0x48+4*axis,&inverse,4);}
#ifdef STATIONARY_BRUSH_WORLD
 std::array<uint8_t,80>ray{};std::memcpy(ray.data(),start.data(),12);invokeStationary(work.data(),base+0x148030,ray.data(),owned.root);
#else
 native(work.data(),owned.root,0,1,start.data(),end.data());
#endif
std::memcpy(&state.fraction,work.data()+0x80,4);std::memcpy(&state.leftSolid,work.data()+0x8c,4);std::memcpy(&state.contents,work.data()+0x84,4);state.startSolid=work[0x8b]!=0;state.allSolid=work[0x8a]!=0;std::memcpy(state.plane.data(),work.data()+0x6c,16);std::memcpy(&combinedState.displacementFlags,work.data()+0x88,2);std::memcpy(&combinedState.displacementHit,work.data()+0x114,4);
#else
#ifdef STATIONARY_BRUSH_WORLD
 sourceStationaryBrushOnlyWorldCandidate(owned,start,q[6],state);
#else
 combined.trace(start,end,q[6],combinedState);
#endif
#endif
  std::array<uint32_t,11>result{};std::memcpy(result.data(),&state.fraction,4);std::memcpy(result.data()+1,&state.leftSolid,4);result[2]=state.startSolid;result[3]=state.allSolid;result[4]=state.contents;std::memcpy(result.data()+5,state.plane.data(),16);result[9]=combinedState.displacementFlags;result[10]=combinedState.displacementHit;out.write(reinterpret_cast<char*>(result.data()),44);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_BRUSH_WORLD
 if(std::memcmp(base+0x148950,nativeLineSplitBytes,sizeof(nativeLineSplitBytes))||std::memcmp(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes))||std::memcmp(base+0x147690,nativeBrushBytes,sizeof(nativeBrushBytes))||std::memcmp(base+0x146850,nativeBoxBytes,sizeof(nativeBoxBytes)))throw std::runtime_error("original body changed");for(const auto&f:rayTreeFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("displacement traversal changed");if(std::memcmp(base+0x14d010,nativeDispWorldRayBytes,sizeof(nativeDispWorldRayBytes)))throw std::runtime_error("displacement adapter changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
