// Whole original ShaderApi030 material bind vs owned x64 callback protocol.
// No original instruction payloads, original imports, DllMain, live SFM or GPU.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "shader_api_material_binding_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Material{uintptr_t table;uint32_t id,value;};
struct Event{uint32_t kind,id,data;};
static std::array<Material,3>materials;
static std::vector<Event>events;
static uint32_t predicateMask=0,predicateCalls=0,keyCalls=0,keyValue=0,keysEqual=0,mutations=0,flushes=0,checks=0;
static ShaderApiMaterialBindingStateCandidate state;
static uintptr_t identity(uint32_t id){return id?reinterpret_cast<uintptr_t>(&materials.at(id-1)):0;}
static uint32_t id(uintptr_t p){if(!p)return 0;for(const auto&m:materials)if(reinterpret_cast<uintptr_t>(&m)==p)return m.id;throw std::runtime_error("invalid material identity");}
#ifndef _WIN64
static std::array<uint8_t,0x2c24>owner;
static uintptr_t ownerIdentity=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void word(size_t offset,uintptr_t value){uint32_t bits=uint32_t(value);std::memcpy(owner.data()+offset,&bits,4);}
static uintptr_t word(size_t offset){uint32_t bits;std::memcpy(&bits,owner.data()+offset,4);return bits;}
#endif
static uintptr_t current(){
#ifdef _WIN64
 return state.material;
#else
 return word(0x2c20);
#endif
}
static void setCurrent(uintptr_t p){
#ifdef _WIN64
 state.material=p;
#else
 word(0x2c20,p);
#endif
}
static bool predicate(uintptr_t p){const auto result=(predicateMask>>predicateCalls++)&1u;events.push_back({1,id(p),result});if(mutations&1u)setCurrent(identity(3));return result!=0;}
static uint32_t key(uintptr_t p){const auto result=keyValue^((keyCalls++&& !keysEqual)?1u:0u);events.push_back({2,id(p),result});if(mutations&2u)setCurrent(identity(2));return result;}
static void flush(){events.push_back({3,id(current()),0});++flushes;if(mutations&4u)setCurrent(identity(1));}
#ifdef _WIN64
struct Ops{bool predicate(uintptr_t p){return ::predicate(p);}uint32_t key(uintptr_t p){return ::key(p);}void flush(){::flush();}};
static void wideCheck(){struct Wide{ShaderApiMaterialBindingStateCandidate&s;uintptr_t before=0;unsigned calls=0;bool predicate(uintptr_t){return false;}uint32_t key(uintptr_t){return 0;}void flush(){before=s.material;s.material=0xfedcba9876543210ull;++calls;}};ShaderApiMaterialBindingStateCandidate s{0xabcdef0123456789ull};Wide ops{s};shaderApiMaterialBindingCandidate(s,uintptr_t(0x123456789abcdef0ull),ops);need(ops.calls==1&&ops.before==0xabcdef0123456789ull&&s.material==0x123456789abcdef0ull,"full-width IDs and post-flush assignment");}
#else
static bool __fastcall predicateNative(void*p,void*){return predicate(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall keyNative(void*p,void*){return key(reinterpret_cast<uintptr_t>(p));}
static void __fastcall flushNative(void*p,void*){need(reinterpret_cast<uintptr_t>(p)==ownerIdentity,"actual shader API receiver");flush();}
#endif
static uint32_t randomWord(uint32_t&s){s=s*1664525u+1013904223u;return s;}
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==2&&!std::ifstream(argv[1]).good(),"fresh owned trace output");const char*outputPath=argv[1];wideCheck();
#else
 need(argc==3&&!std::ifstream(argv[2]).good(),"read-only ShaderAPI image and fresh trace output");const char*outputPath=argv[2];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0x100000);image.seal();
 std::array<uintptr_t,11>materialMethods{};materialMethods[7]=reinterpret_cast<uintptr_t>(&predicateNative);materialMethods[10]=reinterpret_cast<uintptr_t>(&keyNative);std::array<uintptr_t,85>methods{};methods[83]=0x10013f30;methods[84]=reinterpret_cast<uintptr_t>(&flushNative);ownerIdentity=reinterpret_cast<uintptr_t>(owner.data());
#endif
 std::ofstream output(outputPath,std::ios::binary);need(bool(output),"trace output");uint32_t cases=0,eventCount=0,totalFlushes=0,retainedDifferent=0,random=0x20261029;
 for(uint32_t initial=0;initial<4;++initial)for(uint32_t desired=0;desired<4;++desired)for(uint32_t mask=0;mask<16;++mask)for(uint32_t equal=0;equal<2;++equal)for(uint32_t mutation=0;mutation<8;++mutation){
  predicateMask=mask;predicateCalls=keyCalls=flushes=0;keysEqual=equal;mutations=mutation;keyValue=randomWord(random);events.clear();for(uint32_t i=0;i<3;++i)materials[i]={0,i+1,randomWord(random)};
#ifndef _WIN64
  for(auto&m:materials)m.table=reinterpret_cast<uintptr_t>(materialMethods.data());owner.fill(0xa5);word(0,reinterpret_cast<uintptr_t>(methods.data()));
#endif
  setCurrent(identity(initial));const auto beforeMaterials=materials;
#ifdef _WIN64
  Ops ops;shaderApiMaterialBindingCandidate(state,identity(desired),ops);const auto after=current();
#else
  const auto beforeOwner=owner;using Bind=void(__thiscall*)(void*,void*);using Get=uintptr_t(__thiscall*)(void*);reinterpret_cast<Bind>(0x10013f30)(owner.data(),reinterpret_cast<void*>(identity(desired)));const auto after=reinterpret_cast<Get>(0x10013fd0)(owner.data());need(after==current()&&std::memcmp(owner.data(),beforeOwner.data(),0x2c20)==0,"only cached pointer owner field changes");image.verify();
#endif
  need(std::memcmp(materials.data(),beforeMaterials.data(),sizeof(materials))==0&&flushes<=1&&predicateCalls<=4&&keyCalls<=2,"unchanged material objects and bounded callback visits");++checks;
  // Changing a synthetic pointee after bind remains visible through the saved
  // identity. This is NOT a GPU shader parameter consumption observation.
  uint32_t live=0;if(after){auto&material=materials.at(id(after)-1);material.value+=101;live=reinterpret_cast<const Material*>(after)->value;need(live==material.value,"saved identity retains live pointee");++checks;}
  if(initial&&desired&&initial!=desired&&!mutation&&(mask&3u)==3u&&equal){need(after==identity(initial)&&flushes==0,"distinct same-key material identity retained");++retainedDifferent;++checks;}
  const uint32_t header[7]{cases,id(after),predicateCalls,keyCalls,flushes,live,uint32_t(events.size())};output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(&e),sizeof(e));++cases;eventCount+=uint32_t(events.size());totalFlushes+=flushes;
 }
 output.flush();need(bool(output),"complete binding trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image released");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS shader_api_material_binding_cases="<<cases<<" events="<<eventCount<<" flushes="<<totalFlushes<<" retained_distinct_same_key="<<retainedDifferent<<" independent_checks="<<checks<<" material_snapshot=NONE_AT_THIS_BOUNDARY full_draw_snapshot=UNVERIFIED";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64 wide_ids=PASS\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
