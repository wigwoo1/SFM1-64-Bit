#include "source_swept_world_trace_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef WORLD_SURFACE_FLAGS
#include "source_bsp_trace_surface_flags_candidate.h"
#endif
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
#include "source_displacement_trace_surface_properties_candidate.h"
#endif
#ifdef FULL_WORLD_TRACE
#include "source_world_ray_trace_candidate.h"
#endif
#ifdef CACHE_WORLD_POSITION
#include "source_bsp_light_cache_world_candidate.h"
#ifdef CACHE_NEW_RECORD_ANCHOR
#include "source_light_cache_new_record_anchor_candidate.h"
#endif
using CachePoint=std::array<float,3>;
static std::vector<std::array<uint32_t,9>>cacheTraceEvents;
static void recordCacheTrace(const CachePoint&start,const CachePoint&delta,uint32_t mask,float fraction,bool solid){std::array<uint32_t,9>event{};std::memcpy(event.data(),start.data(),12);std::memcpy(event.data()+3,delta.data(),12);event[6]=mask;std::memcpy(&event[7],&fraction,4);event[8]=solid;cacheTraceEvents.push_back(event);}
#endif
#ifdef STATIONARY_BRUSH_WORLD
#include "source_stationary_brush_only_world_candidate.h"
#endif
#ifdef NATIVE_BRUSH_WORLD
#define NOMINMAX
#include <windows.h>
#ifdef FULL_WORLD_TRACE
#include "native_private_world_image_fixture.h"
#endif
#ifdef CACHE_NEW_RECORD_ANCHOR
#include "native_new_cache_record_anchor_fixture.h"
#endif
#include "native_convex_brush_ray_bytes.h"
#include "native_optimized_box_bytes.h"
#include "native_leaf_policy_bytes.h"
#include "native_line_split_bytes.h"
#include "native_swept_world_trace_bytes.h"
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
static uint8_t*workSlot=nullptr;static unsigned releases=0;
static void* __cdecl acquireWork(){return workSlot;}
static void __fastcall releaseWork(void*,void*,void*record){if(reinterpret_cast<uintptr_t>(record)!=reinterpret_cast<uintptr_t>(workSlot)-4)ExitProcess(93);++releases;}
static void* __cdecl clearTrace(void*p,int value,size_t n){return std::memset(p,value,n);}
static size_t activeQuery=0;
static LONG CALLBACK reportFault(EXCEPTION_POINTERS*p){std::cerr<<"native fault query="<<activeQuery<<" eip="<<std::hex<<p->ContextRecord->Eip<<" ecx="<<p->ContextRecord->Ecx<<" edx="<<p->ContextRecord->Edx<<" eax="<<p->ContextRecord->Eax<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::endl;ExitProcess(90);}
#ifdef CACHE_WORLD_POSITION
static uint8_t*cacheNativeBase=nullptr;
static void __fastcall cacheWorldTraceBridge(void*,void*,const uint8_t*ray,uint32_t mask,void*filter,uint8_t*out){
 CachePoint start{},delta{};std::memcpy(start.data(),ray,12);std::memcpy(delta.data(),ray+16,12);if(mask!=0x4081||!ray[68])ExitProcess(94);for(size_t at:{size_t(32),size_t(48)})for(size_t a=0;a<3;++a){float value;std::memcpy(&value,ray+at+4*a,4);if(value!=0)ExitProcess(95);}
 // Confirm the cache policy requested the original world-only filter type.
 using FilterType=int(__thiscall*)(void*);auto table=*reinterpret_cast<uintptr_t**>(filter);if(reinterpret_cast<FilterType>(table[1])(filter)!=1)ExitProcess(96);
 uint32_t depth=0,generation;std::memcpy(workSlot+0x11c,&depth,4);std::memcpy(&generation,workSlot+0x120,4);++generation;std::memcpy(workSlot+0x120,&generation,4);
 using Wrapper=void(__cdecl*)(const void*,int32_t,uint32_t,bool,void*);reinterpret_cast<Wrapper>(cacheNativeBase+0x149eb0)(ray,0,mask,true,out);float fraction;std::memcpy(&fraction,out+0x2c,4);recordCacheTrace(start,delta,mask,fraction,out[0x37]!=0);
}
#endif
#endif
int main(int argc,char**argv){try{
 if(argc!=5
#ifdef FULL_WORLD_TRACE
 &&argc!=6
#endif
 )throw std::runtime_error("map prepared-meshes queries output [engine] required");std::ifstream map(argv[1],std::ios::binary),in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!map||!in||!out)throw std::runtime_error("workspace files");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});
#ifdef WORLD_SURFACE_FLAGS
 if(b.size()<1036)throw std::runtime_error("surface fixture BSP header");uint32_t texBytes;std::memcpy(&texBytes,b.data()+8+6*16+4,4);if(texBytes%72||texBytes/72>=65535)throw std::runtime_error("surface fixture texinfo count");std::vector<uint32_t>surfaceBinding(texBytes/72);for(size_t i=0;i<surfaceBinding.size();++i)surfaceBinding[i]=uint32_t(surfaceBinding.size()-1-i);SourceBspTraceSurfaceFlagsCandidate surfaceCatalog(b,surfaceBinding);SourceBspBrushOnlyTraceCandidate owned(b,surfaceBinding);
#else
 SourceBspBrushOnlyTraceCandidate owned(b,std::vector<uint32_t>(32768));
#endif
#if defined(NATIVE_BRUSH_WORLD) || defined(CONTROLLED_WORLD_GEOMETRY)
 std::ifstream meshInput(argv[2],std::ios::binary);if(!meshInput)throw std::runtime_error("prepared mesh file");std::vector<SourceDisplacementRuntimeMeshCandidate>meshes;std::array<uint32_t,3>header{};
 while(meshInput.read(reinterpret_cast<char*>(header.data()),12)){
  auto power=header[0];if(power<1||power>4)throw std::runtime_error("mesh power");size_t cells=size_t(1)<<power,vc=(cells+1)*(cells+1),tc=2*cells*cells;
  std::array<std::array<float,3>,4>corners{};std::vector<std::array<float,3>>positions(vc);std::vector<float>alphas(vc);std::vector<SourceDisplacementGridTriangleCandidate>triangles(tc);
  if(!meshInput.read(reinterpret_cast<char*>(corners.data()),48)||!meshInput.read(reinterpret_cast<char*>(positions.data()),vc*12)||!meshInput.read(reinterpret_cast<char*>(alphas.data()),vc*4)||!meshInput.read(reinterpret_cast<char*>(triangles.data()),tc*8))throw std::runtime_error("truncated prepared mesh");
  meshes.push_back(sourceDisplacementRuntimeMeshCandidate(power,header[1],header[2],corners,positions,alphas,triangles));
 }if(!meshInput.eof()||meshInput.gcount()!=0
#ifdef CONTROLLED_WORLD_GEOMETRY
 ||meshes.empty()||meshes.size()>16
#else
 ||meshes.size()!=43
#endif
 )throw std::runtime_error("mesh fixture count");
#else
 SourceBspDisplacementCollisionCandidate loaded(b,0);auto&meshes=loaded.meshes;
#endif
 SourcePreparedWorldRayCandidate combined(owned,meshes);
#ifdef WORLD_PLANE_TAGS
 std::ofstream planeTagsOut(std::string(argv[4])+".plane-tags",std::ios::binary);if(!planeTagsOut)throw std::runtime_error("plane tags sidecar");
#endif
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
 std::vector<std::array<uint16_t,2>>displacementProperties;for(size_t i=0;i<meshes.size();++i)displacementProperties.push_back({uint16_t(2*i+1),uint16_t(2*i+2)});
 const SourceDisplacementTraceSurfacePropertiesCandidate displacementBinding(displacementProperties);
 std::ofstream displacementOut(std::string(argv[4])+".displacement-surface",std::ios::binary);if(!displacementOut)throw std::runtime_error("displacement property sidecar");
#endif
#ifdef CACHE_WORLD_POSITION
 SourceBspLightCacheWorldCandidate cacheWorld(b,owned,combined);
 struct ObservedCacheWorld{
  const SourceBspLightCacheWorldCandidate&world;
  int32_t leafAt(const CachePoint&p){return world.leafAt(p);}uint32_t contents(int32_t l){return world.contents(l);}
  void constrainToStartLeaf(const CachePoint&s,float tolerance,CachePoint&e){world.constrainToStartLeaf(s,tolerance,e);}
  SourceLightCacheTraceCandidate trace(const CachePoint&s,const CachePoint&e,uint32_t mask){CachePoint delta{};for(size_t a=0;a<3;++a)delta[a]=e[a]-s[a];const auto result=world.trace(s,e,mask);recordCacheTrace(s,delta,mask,result.fraction,result.startSolid);return result;}
 }observedCache{cacheWorld};
#endif

#ifdef NATIVE_BRUSH_WORLD
 AddVectoredExceptionHandler(1,reportFault);
#ifdef FULL_WORLD_TRACE
 if(argc!=6)throw std::runtime_error("full native world oracle requires pinned engine path");NativePrivateWorldImageFixture privateImage(argv[5]);auto base=privateImage.base;
#else
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");
#endif
 std::memcpy(base+0x147690,nativeBrushBytes,sizeof(nativeBrushBytes));std::memcpy(base+0xe9720,nativePlaneCopyBytes,sizeof(nativePlaneCopyBytes));std::memcpy(base+0x146440,nativeSurfaceBytes,sizeof(nativeSurfaceBytes));std::memcpy(base+0x146850,nativeBoxBytes,sizeof(nativeBoxBytes));std::memcpy(base+0x148270,nativeLeafPolicyBytes,sizeof(nativeLeafPolicyBytes));std::memcpy(base+0x148950,nativeLineSplitBytes,sizeof(nativeLineSplitBytes));for(const auto&c:nativeBoxConstants)std::memcpy(base+c.offset,c.bytes,16);float one=1,epsilon=.03125f,sentinel=-99999;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x33fa60,&epsilon,4);std::memcpy(base+0x3423d4,&sentinel,4);
#ifdef STATIONARY_BRUSH_WORLD
 for(const auto&fragment:stationaryWorldFragments)std::memcpy(base+fragment.offset,fragment.bytes,fragment.size);
#endif
 std::vector<std::array<uint8_t,20>>planes(owned.planes.size());for(size_t i=0;i<planes.size();++i){std::memcpy(planes[i].data(),owned.planes[i].normal.data(),12);std::memcpy(planes[i].data()+12,&owned.planes[i].distance,4);planes[i][16]=owned.planes[i].type;
#ifdef WORLD_PLANE_TAGS
 const auto tags=owned.data.planes[i].tags();std::memcpy(planes[i].data()+16,tags.data(),4);
#endif
}
 struct Side{const uint8_t*plane;uint16_t surface;uint8_t bevel,pad;};static_assert(sizeof(Side)==8,"x86 side");std::vector<Side>sides;for(const auto&s:owned.data.sides)sides.push_back({planes[s.plane].data(),
#ifdef WORLD_SURFACE_FLAGS
 uint16_t(surfaceCatalog.forTexture(s.texture)),
#else
 uint16_t(s.texture),
#endif
 uint8_t(s.bevel),0});
 std::vector<std::array<uint32_t,3>>nodes;for(const auto&n:owned.nodes)nodes.push_back({reinterpret_cast<uint32_t>(planes[n.plane].data()),uint32_t(n.children[0]),uint32_t(n.children[1])});
 struct alignas(16) Box{std::array<uint8_t,48>bytes;};std::vector<Box>boxes;std::vector<std::array<uint32_t,2>>brushes;for(size_t i=0;i<owned.data.brushes.size();++i){const auto&brush=owned.data.brushes[i];uint32_t encoded=brush.count|(brush.first<<16);if(owned.records[i].optimized){encoded=65535|(uint32_t(boxes.size())<<16);boxes.push_back({owned.records[i].box});}brushes.push_back({brush.contents,encoded});}
 std::vector<std::array<uint16_t,8>>leaves(owned.data.leafBrushes.size());std::vector<uint16_t>refs;for(size_t i=0;i<leaves.size();++i){
#if defined(STATIONARY_BRUSH_WORLD) || defined(FULL_WORLD_TRACE)
 std::memcpy(leaves[i].data(),&owned.data.leafContents.at(i),4);
#endif
if(refs.size()>65535)throw std::runtime_error("leaf reference index overflow");leaves[i][4]=uint16_t(refs.size());leaves[i][5]=uint16_t(owned.data.leafBrushes[i].size());for(auto index:owned.data.leafBrushes[i])refs.push_back(uint16_t(index));}
 std::vector<uint8_t>world(0x220);std::vector<uint64_t>surfaces(65536);auto put=[](auto&v,size_t offset,const void*p,size_t size){std::memcpy(v.data()+offset,p,size);};auto pointer=[&](auto&v,size_t offset,const void*p){uint32_t value=reinterpret_cast<uint32_t>(p);put(v,offset,&value,4);};pointer(world,0,nodes.data());pointer(world,0x68,sides.data());pointer(world,0x70,boxes.data());pointer(world,0x88,leaves.data());pointer(world,0x98,refs.data());pointer(world,0xa8,brushes.data());pointer(world,0x218,surfaces.data());
#ifdef WORLD_SURFACE_FLAGS
 for(size_t i=0;i<surfaceBinding.size();++i){const auto id=surfaceBinding[i];surfaces[id]=uint64_t(0x70000000u+id+1)|(uint64_t(surfaceCatalog.textureFlags[i])<<48);}
#endif
#ifdef FULL_WORLD_TRACE
 pointer(world,0x78,planes.data());uint32_t boxCount=uint32_t(boxes.size()),leafCount=uint32_t(leaves.size()),brushCount=uint32_t(brushes.size());put(world,0x6c,&boxCount,4);put(world,0x84,&leafCount,4);put(world,0xa4,&brushCount,4);
#endif

 std::vector<uint16_t>dispRefs;for(size_t i=0;i<leaves.size();++i){const auto&lr=combined.leafRefs()[i];if(dispRefs.size()>65535||lr.size()>65535)throw std::runtime_error("displacement leaf span");leaves[i][6]=uint16_t(dispRefs.size());leaves[i][7]=uint16_t(lr.size());for(auto id:lr)dispRefs.push_back(uint16_t(id));}pointer(world,0xb0,dispRefs.data());
 std::vector<std::array<uint8_t,0xf4>>runtime(meshes.size());struct alignas(16) Cull{std::array<uint8_t,48>bytes;};std::vector<Cull>culls(meshes.size());
 for(size_t i=0;i<meshes.size();++i){const auto&m=meshes[i];pointer(runtime[i],0x70,m.vertices.data());pointer(runtime[i],0x80,m.triangles.data());pointer(runtime[i],0x90,m.tree.nodes.data());pointer(runtime[i],0xa0,m.tree.leafPairs.data());put(runtime[i],0x20,&m.contents,4);put(runtime[i],0x2c,&m.runtimeFlags,4);put(runtime[i],0x28,&m.power,4);uint32_t nc=uint32_t(m.tree.nodes.size());put(runtime[i],0x98,&nc,4);put(culls[i].bytes,0,m.tree.expandedBounds.data(),12);put(culls[i].bytes,0x10,m.tree.expandedBounds.data()+3,12);uint32_t id=uint32_t(i);put(culls[i].bytes,0x20,&id,4);put(culls[i].bytes,0x24,&m.contents,4);}
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
 for(size_t i=0;i<meshes.size();++i)put(runtime[i],0x6c,displacementProperties[i].data(),4);
#endif
#ifdef FULL_WORLD_TRACE
 for(size_t i=0;i<meshes.size();++i){const auto&m=meshes[i];put(runtime[i],4,m.tree.expandedBounds.data(),12);put(runtime[i],0x14,m.tree.expandedBounds.data()+3,12);put(runtime[i],0x30,m.corners.data(),48);put(runtime[i],0x60,m.baseNormal.data(),12);uint32_t id=uint32_t(i);put(runtime[i],0x10,&id,4);}
#endif
 {uint32_t a=reinterpret_cast<uint32_t>(runtime.data()),c=reinterpret_cast<uint32_t>(culls.data());std::memcpy(base+0x637820,&a,4);std::memcpy(base+0x637824,&c,4);}
 std::memcpy(base+0x14d010,nativeDispWorldRayBytes,sizeof(nativeDispWorldRayBytes));std::memcpy(base+0x14e790,nativeDispBoundsBytes,sizeof(nativeDispBoundsBytes));std::memcpy(base+0x14cfa0,nativeDispPostBytes,sizeof(nativeDispPostBytes));
 for(const auto&f:treeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:rayTriangleFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:rayTreeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&c:rayTriangleConstants)std::memcpy(base+c.offset,c.bytes,c.size);for(const auto&c:rayTreeConstants)std::memcpy(base+c.offset,c.bytes,c.size);for(const auto&c:dispBoundsConstants)std::memcpy(base+c.offset,c.bytes,c.size);
 for(const auto&f:sweptWorldFragments)std::memcpy(base+f.offset,f.bytes,f.size);const float high=std::numeric_limits<float>::max();std::memcpy(base+0x31debc,&high,4);
 auto jump=[&](size_t at,uintptr_t target){base[at]=0xe9;int32_t d=int32_t(target-(reinterpret_cast<uintptr_t>(base)+at+5));std::memcpy(base+at+1,&d,4);};jump(0x149230,reinterpret_cast<uintptr_t>(&acquireWork));jump(0xe020,reinterpret_cast<uintptr_t>(&releaseWork));jump(0x2de530,reinterpret_cast<uintptr_t>(&clearTrace));
#ifdef CACHE_WORLD_POSITION
 // Spatial globals consumed by original point-leaf/contents/clip wrappers.
 uint32_t nodeCount=uint32_t(nodes.size());put(world,0x74,&nodeCount,4);std::memcpy(base+0x637568,world.data(),world.size());uint32_t sse=1;std::memcpy(base+0x6c1228,&sse,4);
 std::array<uintptr_t,6>cacheTraceTable{};cacheTraceTable[5]=reinterpret_cast<uintptr_t>(&cacheWorldTraceBridge);uintptr_t cacheTraceObject=reinterpret_cast<uintptr_t>(cacheTraceTable.data());uintptr_t objectPointer=reinterpret_cast<uintptr_t>(&cacheTraceObject);std::memcpy(base+0x407e90,&objectPointer,4);cacheNativeBase=base;
#ifdef CACHE_NEW_RECORD_ANCHOR
 // Terminate immediately after the unmodified selection fragment. Its next
 // original block computes additional lighting/cache data outside this test.
 base[0x1979c6]=0xc3;std::array<uint8_t,64>cacheCenterVariable{};pointer(cacheCenterVariable,0,cacheCenterVariable.data());uintptr_t variablePointer=reinterpret_cast<uintptr_t>(cacheCenterVariable.data());std::memcpy(base+0x662634,&variablePointer,4);
#endif
#endif
#ifdef FULL_WORLD_TRACE
 privateImage.seal();
#else
 DWORD old;for(size_t offset:{size_t(0),size_t(0x140000),size_t(0x150000),size_t(0xe0000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x6a0000);
#endif
#endif

 std::array<uint32_t,9>q{};size_t count=0;
#if defined(FULL_WORLD_TRACE) && defined(NATIVE_BRUSH_WORLD)
 uint64_t stationaryProbeCount=0,stationaryProbedQueries=0;
#endif
 while(in.read(reinterpret_cast<char*>(q.data()),36)){
  std::array<float,3>start,delta;std::memcpy(start.data(),q.data(),12);std::memcpy(delta.data(),q.data()+3,12);if(q[7]>1||q[8]>1)throw std::runtime_error("fixture bool");SourceSweptWorldTraceCandidate result;
#ifdef WORLD_SURFACE_FLAGS
  std::array<uint32_t,2>surfaceResult{};
#endif
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
 std::array<uint32_t,2>displacementSurface{UINT32_MAX,0};
#endif
#ifdef CACHE_WORLD_POSITION
  cacheTraceEvents.clear();CachePoint anchor{};int32_t inputLeaf=cacheWorld.leafAt(start);
#endif
#ifdef NATIVE_BRUSH_WORLD
  activeQuery=count;std::vector<uint8_t>work(0x220);std::vector<uint32_t>visited(brushes.size()),dispVisited(meshes.size());pointer(work,0x104,world.data());pointer(work,0x128,visited.data());pointer(work,0x150,dispVisited.data());uint32_t generation=1;put(work,0x120,&generation,4);uint32_t available=q[8];put(world,0x7c,&available,4);workSlot=work.data();releases=0;
#ifdef FULL_WORLD_TRACE
  std::vector<uint32_t>nestedVisited(brushes.size()),nestedDispVisited(meshes.size());pointer(work,0x13c,nestedVisited.data());pointer(work,0x164,nestedDispVisited.data());uint32_t bc=uint32_t(visited.size()),dc=uint32_t(dispVisited.size());put(work,0x134,&bc,4);put(work,0x148,&bc,4);put(work,0x15c,&dc,4);put(work,0x170,&dc,4);
#endif
  struct alignas(16) Ray{std::array<uint8_t,80>bytes{};};Ray ray;std::memcpy(ray.bytes.data(),start.data(),12);std::memcpy(ray.bytes.data()+16,delta.data(),12);ray.bytes[68]=1;
#ifdef WORLD_SURFACE_FLAGS
  const float dx=delta[0]*delta[0],dy=delta[1]*delta[1],dz=delta[2]*delta[2];const float dxy=dx+dy;ray.bytes[69]=(dxy+dz)!=0;
#else
  ray.bytes[69]=delta!=std::array<float,3>{0,0,0};
#endif
  std::array<uint8_t,84>trace;trace.fill(0xcd);
#ifdef CACHE_WORLD_POSITION
  using Leaf=int32_t(__cdecl*)(const float*);inputLeaf=reinterpret_cast<Leaf>(base+0x145660)(start.data());std::array<uint8_t,0x1e0>record;record.fill(0xa5);const auto originalStart=start;
#ifdef CACHE_NEW_RECORD_ANCHOR
  put(cacheCenterVariable,0x30,&q[6],4);const std::array<int32_t,3>cell{17,-29,47};invokeNewCacheRecordAnchor(base+0x19791b,record.data(),start.data(),inputLeaf,cell.data());auto recordWord=[&](size_t at){uint32_t v;std::memcpy(&v,record.data()+at,4);return v;};if(recordWord(0x18c)!=uint32_t(inputLeaf)||recordWord(0xc8)!=UINT32_MAX||recordWord(0x128)!=UINT32_MAX||recordWord(0x1d4)!=17||recordWord(0x1d8)!=uint32_t(-29)||recordWord(0x1dc)!=47)throw std::runtime_error("new cache-record frame/field setup differs");
#else
  using Position=void(__cdecl*)(void*,const float*,int32_t);reinterpret_cast<Position>(base+0x1947b0)(record.data(),start.data(),inputLeaf);
#endif
  std::memcpy(anchor.data(),record.data()+0x198,12);for(size_t i=0;i<record.size();++i){bool changed=i>=0x198&&i<0x1a4;
#ifdef CACHE_NEW_RECORD_ANCHOR
   changed=changed||(i>=0xc8&&i<0xcc)||(i>=0x128&&i<0x12c)||(i>=0x18c&&i<0x190)||(i>=0x1d4&&i<0x1e0);
#endif
   if(!changed&&record[i]!=0xa5)throw std::runtime_error("cache policy changed unrelated record bytes");}if(start!=originalStart)throw std::runtime_error("cache policy changed query input");if(releases!=cacheTraceEvents.size()||cacheTraceEvents.size()>2)throw std::runtime_error("cache trace work release count");
#else
  using Wrapper=void(__cdecl*)(const void*,int32_t,uint32_t,bool,void*);reinterpret_cast<Wrapper>(base+0x149eb0)(ray.bytes.data(),owned.root,q[6],q[7]!=0,trace.data());if(releases!=1)throw std::runtime_error("work release not called exactly once");
#endif
#ifdef FULL_WORLD_TRACE
  if(delta==std::array<float,3>{0,0,0}){uint32_t probes=0;std::memcpy(&probes,work.data()+0x124,4);stationaryProbeCount+=probes;stationaryProbedQueries+=probes!=0;}
#endif
  std::memcpy(&result.state.trace.fraction,trace.data()+0x2c,4);std::memcpy(&result.state.trace.leftSolid,trace.data()+0x38,4);std::memcpy(&result.state.trace.contents,trace.data()+0x30,4);result.state.trace.allSolid=trace[0x36]!=0;result.state.trace.startSolid=trace[0x37]!=0;std::memcpy(result.state.trace.plane.data(),trace.data()+0x18,16);
#ifdef WORLD_PLANE_TAGS
 std::memcpy(result.state.trace.planeTags.data(),trace.data()+0x28,4);
#endif
std::memcpy(&result.state.displacementFlags,trace.data()+0x34,2);std::memcpy(&result.state.displacementHit,work.data()+0x114,4);std::memcpy(result.startPosition.data(),trace.data(),12);std::memcpy(result.endPosition.data(),trace.data()+12,12);
#ifdef WORLD_SURFACE_FLAGS
  uint32_t name,defaultName;uint16_t flags;std::memcpy(&name,trace.data()+0x3c,4);std::memcpy(&defaultName,base+0x402fe8,4);std::memcpy(&flags,trace.data()+0x42,2);if(name==defaultName)surfaceResult[0]=SourceBspTraceSurfaceFlagsCandidate::defaultId;else if(name==0x10350ee0)surfaceResult[0]=SourceBspTraceSurfaceFlagsCandidate::displacementId;else if(name>0x70000000u&&name<=0x70000000u+surfaceBinding.size())surfaceResult[0]=name-0x70000001u;else throw std::runtime_error("unrecognized native surface name token");surfaceResult[1]=flags;
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
  if(name==0x10350ee0){uint16_t property=0;std::memcpy(&property,trace.data()+0x40,2);if(property<1||property>meshes.size()*2)throw std::runtime_error("unrecognized controlled native displacement property");const uint32_t index=uint32_t(property-1)/2;if(property!=displacementProperties[index][(result.state.displacementFlags&16u)?1:0])throw std::runtime_error("native displacement property selector mismatch");displacementSurface={index,property};}
#endif
#endif
#else
#ifdef CACHE_WORLD_POSITION
#ifdef CACHE_NEW_RECORD_ANCHOR
  int32_t centerSetting;std::memcpy(&centerSetting,&q[6],4);anchor=sourceLightCacheNewRecordAnchorCandidate(start,inputLeaf,centerSetting,observedCache);
#else
  anchor=sourceLightCachePositionCandidate(start,inputLeaf,observedCache);if(anchor!=cacheWorld.position(start))throw std::runtime_error("observed versus direct cache adapter differs");
#endif
#else
#ifdef FULL_WORLD_TRACE
#ifdef WORLD_SURFACE_FLAGS
  const float dx=delta[0]*delta[0],dy=delta[1]*delta[1],dz=delta[2]*delta[2];const float dxy=dx+dy;result=sourceWorldRayTraceWithSweptFlagCandidate(combined,start,delta,q[6],(dxy+dz)!=0,q[7]!=0,q[8]!=0);
#else
  result=sourceWorldRayTraceCandidate(combined,start,delta,q[6],q[7]!=0,q[8]!=0);
#endif
#else
  result=sourceSweptWorldTraceCandidate(combined,start,delta,q[6],q[7]!=0,q[8]!=0);
#endif
#endif
#endif
#ifdef CACHE_WORLD_POSITION
  if(cacheTraceEvents.size()>2)throw std::runtime_error("cache trace event capacity");
#ifndef CACHE_NEW_RECORD_ANCHOR
  if(cacheTraceEvents.empty())throw std::runtime_error("cache trace missing");
#endif
  std::array<uint32_t,23>cacheResult{};cacheResult[0]=uint32_t(inputLeaf);cacheResult[1]=uint32_t(cacheTraceEvents.size());for(size_t i=0;i<cacheTraceEvents.size();++i)std::copy(cacheTraceEvents[i].begin(),cacheTraceEvents[i].end(),cacheResult.begin()+2+9*i);std::memcpy(cacheResult.data()+20,anchor.data(),12);out.write(reinterpret_cast<const char*>(cacheResult.data()),92);
#else
#ifdef WORLD_PLANE_TAGS
  planeTagsOut.write(reinterpret_cast<const char*>(result.state.trace.planeTags.data()),4);if(!planeTagsOut)throw std::runtime_error("plane tags write");
#endif
  const auto&s=result.state;std::array<uint32_t,17>r{};std::memcpy(r.data(),&s.trace.fraction,4);std::memcpy(r.data()+1,&s.trace.leftSolid,4);r[2]=s.trace.startSolid;r[3]=s.trace.allSolid;r[4]=s.trace.contents;std::memcpy(r.data()+5,s.trace.plane.data(),16);r[9]=s.displacementFlags;r[10]=s.displacementHit;std::memcpy(r.data()+11,result.startPosition.data(),12);std::memcpy(r.data()+14,result.endPosition.data(),12);out.write(reinterpret_cast<const char*>(r.data()),68);++count;
#ifdef WORLD_SURFACE_FLAGS
#ifndef NATIVE_BRUSH_WORLD
  surfaceResult={surfaceCatalog.identity(s.trace.surface),surfaceCatalog.flags(s.trace.surface)};
#endif
  out.write(reinterpret_cast<const char*>(surfaceResult.data()),8);
#endif
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
#ifndef NATIVE_BRUSH_WORLD
  if(s.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Displacement)displacementSurface={s.trace.surface.index,displacementBinding.property(s.trace.surface,s.displacementFlags)};
#endif
  displacementOut.write(reinterpret_cast<const char*>(displacementSurface.data()),8);
#endif
#endif
#ifdef CACHE_WORLD_POSITION
  ++count;
#endif
 }
 if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef WORLD_PLANE_TAGS
 planeTagsOut.flush();if(!planeTagsOut)throw std::runtime_error("plane tags flush");
#endif
#ifdef WORLD_DISPLACEMENT_SURFACE_IDENTITY
 displacementOut.flush();if(!displacementOut)throw std::runtime_error("displacement property sidecar output failure");
#endif
#ifdef NATIVE_BRUSH_WORLD
#ifdef FULL_WORLD_TRACE
 privateImage.verify();
 std::cout<<"stationary_probe_count="<<stationaryProbeCount<<" stationary_probed_queries="<<stationaryProbedQueries<<'\n';
#endif
 for(const auto&f:sweptWorldFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original world wrapper changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS world ray wrapper cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
