#include "source_convex_brush_ray_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef SFM64_STATIONARY_BRUSH
#include "source_stationary_brush_candidate.h"
#ifdef SFM64_NATIVE_BRUSH_ORACLE
#include "native_stationary_brush_bytes.h"
#endif
#endif
#ifdef SFM64_AUTO_BRUSH_DISPATCH
#include "source_brush_line_dispatch_candidate.h"
#endif
#ifdef SFM64_OWNED_BOX_CANDIDATE
#include "source_optimized_box_line_ray_candidate.h"
#endif
#ifdef SFM64_NATIVE_BRUSH_ORACLE
#define NOMINMAX
#include <windows.h>
#include "native_convex_brush_ray_bytes.h"
#if defined(SFM64_NATIVE_BOX_ORACLE) || defined(SFM64_AUTO_BRUSH_DISPATCH)
#include "native_optimized_box_bytes.h"
#endif
#endif
#ifdef SFM64_NATIVE_BRUSH_ORACLE
static void put(std::vector<uint8_t>&b,size_t p,const void*value,size_t size){std::memcpy(b.data()+p,value,size);}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("usage: probe map queries.bin output.bin");std::ifstream map(argv[1],std::ios::binary),input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);if(!map||!input||!output)throw std::runtime_error("workspace query files");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});SourceBspWorldCollisionDataCandidate data(b);
#ifdef WORLD_PLANE_TAGS
 // Deliberately distinguish all-four-byte plane copies from the incoming
 // trace padding. Production map import keeps these reserved bytes zero.
 for(auto&plane:data.planes)plane.reserved={0x35,0x79};
#endif

#ifdef SFM64_AUTO_BRUSH_DISPATCH
 const auto collisionRecords=sourceBspCollisionRecordsCandidate(data,std::vector<uint32_t>(32768));
#endif
#ifdef SFM64_NATIVE_BRUSH_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6a0000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("oracle base unavailable");
#ifdef SFM64_STATIONARY_BRUSH
 std::memcpy(base+0x146c30,nativeStationaryBrushBytes,sizeof(nativeStationaryBrushBytes));
#endif
 std::memcpy(base+0x147690,nativeBrushBytes,sizeof(nativeBrushBytes));std::memcpy(base+0xe9720,nativePlaneCopyBytes,sizeof(nativePlaneCopyBytes));std::memcpy(base+0x146440,nativeSurfaceBytes,sizeof(nativeSurfaceBytes));
 const float one=1,epsilon=0.03125f,sentinel=-99999.0f;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x33fa60,&epsilon,4);std::memcpy(base+0x3423d4,&sentinel,4);
#if defined(SFM64_NATIVE_BOX_ORACLE) || defined(SFM64_AUTO_BRUSH_DISPATCH)
 std::memcpy(base+0x146850,nativeBoxBytes,sizeof(nativeBoxBytes));
 for(const auto&fragment:nativeBoxConstants)std::memcpy(base+fragment.offset,fragment.bytes,16);
#endif
 std::vector<std::array<float,5>>planes(data.planes.size());for(size_t i=0;i<planes.size();++i){std::memcpy(planes[i].data(),data.planes[i].normal.data(),12);planes[i][3]=data.planes[i].distance;
#ifdef WORLD_PLANE_TAGS
 const auto tags=data.planes[i].tags();std::memcpy(reinterpret_cast<uint8_t*>(planes[i].data())+16,tags.data(),4);
#endif
 }
 struct Side{const float*plane;uint16_t texture;uint8_t bevel,pad;};static_assert(sizeof(Side)==8,"native side layout");std::vector<Side>sides;for(const auto&s:data.sides)sides.push_back({planes[s.plane].data(),uint16_t(s.texture),uint8_t(s.bevel),0});
 std::vector<uint8_t>world(0x220);std::vector<uint64_t>surfaces(65536);auto sidePtr=reinterpret_cast<uintptr_t>(sides.data()),surfacePtr=reinterpret_cast<uintptr_t>(surfaces.data());put(world,0x68,&sidePtr,4);put(world,0x218,&surfacePtr,4);
 DWORD old;for(size_t offset:{size_t(0x140000),size_t(0xe0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("oracle protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);
#ifdef SFM64_STATIONARY_BRUSH
 using Function=void(__thiscall*)(void*,void*);const auto native=reinterpret_cast<Function>(base+0x146c30);
#else
 using Function=void(__fastcall*)(void*,void*);const auto native=reinterpret_cast<Function>(base+0x147690);
#endif
#if defined(SFM64_NATIVE_BOX_ORACLE) || defined(SFM64_AUTO_BRUSH_DISPATCH)
 using BoxFunction=int(__cdecl*)(void*,void*,void*);const auto nativeBox=reinterpret_cast<BoxFunction>(base+0x146850);
 (void)native;
 (void)nativeBox;
#endif
#endif
#ifdef WORLD_PLANE_TAGS
 std::ofstream tagsOut(std::string(argv[3])+".plane-tags",std::ios::binary);if(!tagsOut)throw std::runtime_error("brush tags sidecar");
#endif
 std::array<uint32_t,12>query{};size_t count=0;
 while(input.read(reinterpret_cast<char*>(query.data()),sizeof(query))){const uint32_t index=query[0];const auto&brush=data.brushes.at(index);std::array<float,3>start,end;std::memcpy(start.data(),query.data()+1,12);std::memcpy(end.data(),query.data()+4,12);SourceConvexBrushRayStateCandidate state;std::memcpy(&state.fraction,query.data()+7,4);std::memcpy(&state.leftSolid,query.data()+8,4);state.startSolid=(query[9]&1)!=0;state.allSolid=(query[9]&2)!=0;state.contents=query[10];state.side=query[11];
#ifdef WORLD_PLANE_TAGS
  std::array<uint8_t,4>planeTags{0xa1,0xb2,0xc3,0xd4};
#endif
  const std::array<float,4>fallback{99,98,97,96};std::array<float,4>plane=fallback;
  if(state.side!=UINT32_MAX){const auto&p=data.planes.at(data.sides.at(state.side).plane);std::memcpy(plane.data(),p.normal.data(),12);plane[3]=p.distance;}
#ifdef SFM64_NATIVE_BRUSH_ORACLE
  if(brush.first>65535||brush.count>=65535)throw std::runtime_error("general native brush encoding exceeded");std::array<uint32_t,2>record{brush.contents,uint32_t(brush.count)|(brush.first<<16)};std::vector<uint8_t>work(0x220);put(work,0,start.data(),12);put(work,12,end.data(),12);put(work,0x6c,plane.data(),16);
#ifdef WORLD_PLANE_TAGS
 put(work,0x7c,planeTags.data(),4);
#endif
put(work,0x80,&state.fraction,4);put(work,0x84,&state.contents,4);work[0x8b]=uint8_t(state.startSolid);work[0x8a]=uint8_t(state.allSolid);put(work,0x8c,&state.leftSolid,4);const auto worldPtr=reinterpret_cast<uintptr_t>(world.data());put(work,0x104,&worldPtr,4);
#if defined(SFM64_NATIVE_BOX_ORACLE) || defined(SFM64_AUTO_BRUSH_DISPATCH)
#ifdef SFM64_AUTO_BRUSH_DISPATCH
  if(collisionRecords.at(index).optimized){
#endif
  if(brush.count!=6)throw std::runtime_error("controlled six-plane box required");alignas(16) std::array<unsigned char,48>box{};std::array<float,4>mins{},maxs{};unsigned faces=0;
  for(size_t i=0;i<6;++i){const auto&p=data.planes[data.sides[brush.first+i].plane];bool found=false;for(size_t axis=0;axis<3;++axis){bool axial=true;for(size_t j=0;j<3;++j)if(j!=axis&&p.normal[j]!=0)axial=false;if(axial&&(p.normal[axis]==1||p.normal[axis]==-1)){const unsigned face=unsigned(axis)+(p.normal[axis]>0?3u:0u);if(faces&(1u<<face))throw std::runtime_error("duplicate box plane");faces|=1u<<face;if(p.normal[axis]>0)maxs[axis]=p.distance;else mins[axis]=-p.distance;found=true;break;}}if(!found)throw std::runtime_error("nonaxial controlled box");}
  if(faces!=63)throw std::runtime_error("incomplete box");std::memcpy(box.data(),mins.data(),16);std::memcpy(box.data()+16,maxs.data(),16);std::memset(box.data()+32,0xff,12);
  for(size_t axis=0;axis<3;++axis){const float delta=end[axis]-start[axis],inverse=delta==0?std::numeric_limits<float>::max():1.0f/delta;put(work,0x3c+axis*4,&delta,4);put(work,0x48+axis*4,&inverse,4);}work[0x100]=1;
#ifdef SFM64_AUTO_BRUSH_DISPATCH
  std::memcpy(box.data(),collisionRecords.at(index).box.data(),48);auto boxPtr=reinterpret_cast<uintptr_t>(box.data());put(world,0x70,&boxPtr,4);record[1]=65535;native(work.data(),record.data());
  }else native(work.data(),record.data());
#else
  (void)nativeBox(work.data(),record.data(),box.data());
#endif
#else
  native(work.data(),record.data());
#endif
  std::memcpy(&state.fraction,work.data()+0x80,4);std::memcpy(&state.leftSolid,work.data()+0x8c,4);state.contents=*reinterpret_cast<uint32_t*>(work.data()+0x84);state.startSolid=work[0x8b]!=0;state.allSolid=work[0x8a]!=0;std::memcpy(plane.data(),work.data()+0x6c,16);
#ifdef WORLD_PLANE_TAGS
 std::memcpy(planeTags.data(),work.data()+0x7c,4);
#endif
#else
#ifdef SFM64_AUTO_BRUSH_DISPATCH
  (void)brush;SourceOptimizedBoxLineStateCandidate dispatched;dispatched.fraction=state.fraction;dispatched.leftSolid=state.leftSolid;dispatched.startSolid=state.startSolid;dispatched.allSolid=state.allSolid;dispatched.contents=state.contents;dispatched.plane=plane;
#ifdef WORLD_PLANE_TAGS
 dispatched.planeTags=planeTags;
#endif
#ifdef SFM64_STATIONARY_BRUSH
 sourceStationaryBrushCandidate(data,collisionRecords,index,start,dispatched);
#else
 sourceBrushLineDispatchCandidate(data,collisionRecords,index,start,end,dispatched);
#endif
state.fraction=dispatched.fraction;state.leftSolid=dispatched.leftSolid;state.startSolid=dispatched.startSolid;state.allSolid=dispatched.allSolid;state.contents=dispatched.contents;plane=dispatched.plane;
#ifdef WORLD_PLANE_TAGS
 planeTags=dispatched.planeTags;
#endif
#else
#ifdef SFM64_OWNED_BOX_CANDIDATE
  if(brush.count!=6)throw std::runtime_error("controlled six-plane box required");std::array<float,3>mins{},maxs{};unsigned faces=0;
  for(size_t i=0;i<6;++i){const auto&p=data.planes[data.sides[brush.first+i].plane];bool found=false;for(size_t axis=0;axis<3;++axis){bool axial=true;for(size_t j=0;j<3;++j)if(j!=axis&&p.normal[j]!=0)axial=false;if(axial&&(p.normal[axis]==1||p.normal[axis]==-1)){const unsigned face=unsigned(axis)+(p.normal[axis]>0?3u:0u);if(faces&(1u<<face))throw std::runtime_error("duplicate box plane");faces|=1u<<face;if(p.normal[axis]>0)maxs[axis]=p.distance;else mins[axis]=-p.distance;found=true;break;}}if(!found)throw std::runtime_error("nonaxial controlled box");}
  if(faces!=63)throw std::runtime_error("incomplete box");SourceOptimizedBoxLineStateCandidate boxState;boxState.fraction=state.fraction;boxState.leftSolid=state.leftSolid;boxState.startSolid=state.startSolid;boxState.allSolid=state.allSolid;boxState.contents=state.contents;boxState.plane=plane;sourceOptimizedBoxLineRayCandidate(mins,maxs,start,end,brush.contents,boxState);state.fraction=boxState.fraction;state.leftSolid=boxState.leftSolid;state.startSolid=boxState.startSolid;state.allSolid=boxState.allSolid;state.contents=boxState.contents;plane=boxState.plane;
#else
  (void)brush;sourceConvexBrushRayCandidate(data,index,start,end,state);plane=fallback;if(state.side!=UINT32_MAX){const auto&p=data.planes.at(data.sides.at(state.side).plane);std::memcpy(plane.data(),p.normal.data(),12);plane[3]=p.distance;}
#endif
#endif
#endif
#ifdef WORLD_PLANE_TAGS
  tagsOut.write(reinterpret_cast<const char*>(planeTags.data()),4);if(!tagsOut)throw std::runtime_error("brush tags write");
#endif
  std::array<uint32_t,9>result{};std::memcpy(result.data(),&state.fraction,4);std::memcpy(result.data()+1,&state.leftSolid,4);result[2]=uint32_t(state.startSolid);result[3]=uint32_t(state.allSolid);result[4]=state.contents;std::memcpy(result.data()+5,plane.data(),16);output.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++count;
 }
 if(!input.eof()||input.gcount()!=0||!count)throw std::runtime_error("query length invalid");output.flush();if(!output)throw std::runtime_error("output failed");
#ifdef WORLD_PLANE_TAGS
 tagsOut.flush();if(!tagsOut)throw std::runtime_error("brush tags flush");
#endif
#ifdef SFM64_NATIVE_BRUSH_ORACLE
#if defined(SFM64_NATIVE_BOX_ORACLE) || defined(SFM64_AUTO_BRUSH_DISPATCH)
 if(std::memcmp(base+0x146850,nativeBoxBytes,sizeof(nativeBoxBytes)))throw std::runtime_error("native box body modified");
#endif
#ifdef SFM64_STATIONARY_BRUSH
 if(std::memcmp(base+0x146c30,nativeStationaryBrushBytes,sizeof(nativeStationaryBrushBytes)))throw std::runtime_error("stationary body modified");
#endif
 if(std::memcmp(base+0x147690,nativeBrushBytes,sizeof(nativeBrushBytes)))throw std::runtime_error("native body modified");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
