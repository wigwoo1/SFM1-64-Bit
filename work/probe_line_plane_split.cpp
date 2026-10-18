#include "source_line_plane_split_candidate.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#ifdef BENCHMARK_LINE_WALK
#include "source_bsp_point_leaf_candidate.h"
#include <iterator>
#endif
#ifdef MULTILEVEL_LINE_SPLIT
#include "source_bsp_line_walk_candidate.h"
#endif
#ifdef NATIVE_LINE_SPLIT
#define NOMINMAX
#include <windows.h>
#include "native_line_split_bytes.h"
#endif
static std::vector<uint32_t>events;
static bool updateHit=false;static float nextHit=1;
#ifdef NATIVE_LINE_SPLIT
static void __fastcall leaf(void*work,uint32_t index,float from,float to){uint32_t a,b;std::memcpy(&a,&from,4);std::memcpy(&b,&to,4);events.insert(events.end(),{index,a,b});if(updateHit)std::memcpy(static_cast<uint8_t*>(work)+0x80,&nextHit,4);}
#endif
int main(int argc,char**argv){try{
#ifdef BENCHMARK_LINE_WALK
 if(argc!=4)throw std::runtime_error("input output map required");
 std::ifstream map(argv[3],std::ios::binary);if(!map)throw std::runtime_error("map unavailable");std::vector<uint8_t>rawMap((std::istreambuf_iterator<char>(map)),{});SourceBspPointLeafCandidate checked(rawMap);int32_t treeRoot=checked.worldRoot();uint32_t treeLeaves=checked.leafCount();
 auto word=[&](size_t p){uint32_t v;std::memcpy(&v,rawMap.data()+p,4);return v;};auto scalar=[&](size_t p){float v;std::memcpy(&v,rawMap.data()+p,4);return v;};auto signedWord=[&](size_t p){int32_t v;std::memcpy(&v,rawMap.data()+p,4);return v;};
 std::vector<SourceLineWalkPlaneCandidate>treePlanes;std::vector<SourceLineWalkNodeCandidate>treeNodes;
 for(size_t i=0;i<word(28)/20;++i){size_t p=word(24)+20*i;treePlanes.push_back({{scalar(p),scalar(p+4),scalar(p+8)},scalar(p+12),uint8_t(word(p+16))});}
 for(size_t i=0;i<word(8+5*16+4)/32;++i){size_t p=word(8+5*16)+32*i;treeNodes.push_back({word(p),{signedWord(p+4),signedWord(p+8)}});}
#else
 if(argc!=3)throw std::runtime_error("input output required");
#endif
std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_LINE_SPLIT
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");std::memcpy(base+0x148950,nativeLineSplitBytes,sizeof(nativeLineSplitBytes));uint8_t jump[7]={0xb8,0,0,0,0,0xff,0xe0};uint32_t callback=reinterpret_cast<uint32_t>(&leaf);std::memcpy(jump+1,&callback,4);std::memcpy(base+0x148270,jump,7);float one=1,epsilon=.03125f;uint32_t sign=0x80000000;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x33fa60,&epsilon,4);std::memcpy(base+0x31cbb0,&sign,4);DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=void(__fastcall*)(void*,int32_t,float,float,const float*,const float*);auto native=reinterpret_cast<Function>(base+0x148950);
#endif
#ifdef EXTENDED_LINE_SPLIT
 std::array<float,16>q{};
#else
 std::array<float,2>q{};
#endif
 size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){events.clear();
#ifdef EXTENDED_LINE_SPLIT
 uint32_t type,flags;std::memcpy(&type,&q[14],4);std::memcpy(&flags,&q[15],4);if(type>5)throw std::runtime_error("invalid plane type");updateHit=flags!=0;nextHit=q[13];float hit=q[12];
#endif
#if defined(MULTILEVEL_LINE_SPLIT) && !defined(BENCHMARK_LINE_WALK)
 int32_t treeRoot=0;uint32_t treeLeaves=8;
 std::vector<SourceLineWalkPlaneCandidate>treePlanes;std::vector<SourceLineWalkNodeCandidate>treeNodes;
 for(unsigned i=0;i<7;++i){SourceLineWalkPlaneCandidate p;p.type=uint8_t((type+i)%6);p.distance=q[9]+float(int(i)-3)*.125f;if(p.type<3)p.normal[p.type]=1;else p.normal={q[6],q[7],q[8]};treePlanes.push_back(p);SourceLineWalkNodeCandidate n;n.plane=i;for(unsigned j=0;j<2;++j){int child=int(2*i+1+j);n.children[j]=child<7?child:6-child;}treeNodes.push_back(n);}
#endif
#ifdef NATIVE_LINE_SPLIT
  std::array<uint8_t,0x220>work{},world{};std::array<uint8_t,20>plane{};std::array<uint32_t,3>node{reinterpret_cast<uint32_t>(plane.data()),uint32_t(-1),uint32_t(-2)};float normal=1;std::memcpy(plane.data(),&normal,4);uint32_t nodePtr=reinterpret_cast<uint32_t>(node.data()),worldPtr=reinterpret_cast<uint32_t>(world.data());std::memcpy(world.data(),&nodePtr,4);std::memcpy(work.data()+0x104,&worldPtr,4);float fraction=1;std::memcpy(work.data()+0x80,&fraction,4);std::array<float,3>start{q[0],0,0},end{q[1],0,0};
#ifndef EXTENDED_LINE_SPLIT
 native(work.data(),0,0,1,start.data(),end.data());
#endif
#ifdef EXTENDED_LINE_SPLIT
 std::memcpy(plane.data(),q.data()+6,16);plane[16]=uint8_t(type);start={q[0],q[1],q[2]};end={q[3],q[4],q[5]};std::memcpy(work.data()+0x80,&hit,4);events.clear();
#ifdef MULTILEVEL_LINE_SPLIT
 std::vector<std::array<uint8_t,20>>nativePlanes(treePlanes.size());std::vector<std::array<uint32_t,3>>nativeNodes(treeNodes.size());
 for(size_t i=0;i<treePlanes.size();++i){std::memcpy(nativePlanes[i].data(),treePlanes[i].normal.data(),12);std::memcpy(nativePlanes[i].data()+12,&treePlanes[i].distance,4);nativePlanes[i][16]=treePlanes[i].type;}for(size_t i=0;i<treeNodes.size();++i){nativeNodes[i]={reinterpret_cast<uint32_t>(nativePlanes[treeNodes[i].plane].data()),uint32_t(treeNodes[i].children[0]),uint32_t(treeNodes[i].children[1])};}nodePtr=reinterpret_cast<uint32_t>(nativeNodes.data());std::memcpy(world.data(),&nodePtr,4);
#endif
#ifdef MULTILEVEL_LINE_SPLIT
 native(work.data(),treeRoot,q[10],q[11],start.data(),end.data());
#else
 native(work.data(),0,q[10],q[11],start.data(),end.data());
#endif
std::memcpy(&hit,work.data()+0x80,4);
#endif
#else
#ifdef EXTENDED_LINE_SPLIT
#ifdef MULTILEVEL_LINE_SPLIT
 sourceBspLineWalkCandidate(treePlanes,treeNodes,treeRoot,treeLeaves,{q[0],q[1],q[2]},{q[3],q[4],q[5]},q[10],q[11],hit,[&](uint32_t index,float from,float to){uint32_t a,b;std::memcpy(&a,&from,4);std::memcpy(&b,&to,4);events.insert(events.end(),{index,a,b});if(updateHit)hit=nextHit;});
#else
 auto distance=[&](size_t offset){if(type<3)return q[offset+type]-q[9];const float y=q[offset+1]*q[7],x=q[offset]*q[6],z=q[offset+2]*q[8];const float xy=y+x;return (xy+z)-q[9];};
 if(q[10]<hit){auto split=sourceLinePlaneSplitCandidate(distance(0),distance(3));auto emit=[&](unsigned index,float from,float to){if(from>=hit)return;uint32_t a,b;std::memcpy(&a,&from,4);std::memcpy(&b,&to,4);events.insert(events.end(),{index,a,b});if(updateHit)hit=nextHit;};if(split.split){const float span=q[11]-q[10];emit(split.side,q[10],span*split.nearEnd+q[10]);emit(split.side^1,span*split.farStart+q[10],q[11]);}else emit(split.side,q[10],q[11]);}
#endif
#else
  auto split=sourceLinePlaneSplitCandidate(q[0],q[1]);auto emit=[](unsigned index,float from,float to){if(from>=1)return;uint32_t a,b;std::memcpy(&a,&from,4);std::memcpy(&b,&to,4);events.insert(events.end(),{index,a,b});};if(split.split){emit(split.side,0,(1.0f*split.nearEnd)+0.0f);emit(split.side^1,(1.0f*split.farStart)+0.0f,1);}else emit(split.side,0,1);
#endif
#endif
#ifdef BENCHMARK_LINE_WALK
  const uint32_t eventCount=uint32_t(events.size()/3);out.write(reinterpret_cast<const char*>(&eventCount),4);out.write(reinterpret_cast<const char*>(events.data()),std::streamsize(events.size()*4));++count;
#else
#ifdef MULTILEVEL_LINE_SPLIT
  std::array<uint32_t,25>result;
#else
  std::array<uint32_t,7>result;
#endif
result.fill(UINT32_MAX);result[0]=uint32_t(events.size()/3);for(size_t i=0;i<events.size();++i)result[1+i]=events[i];out.write(reinterpret_cast<char*>(result.data()),sizeof(result));++count;
#endif
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failed");
#ifdef NATIVE_LINE_SPLIT
 if(std::memcmp(base+0x148950,nativeLineSplitBytes,sizeof(nativeLineSplitBytes)))throw std::runtime_error("body changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
