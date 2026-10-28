// Extended real generic preparation; stop before static/dynamic render bodies.
// Connected execution stops after the real blend classification, NOT after a
// complete generic render. The later generic/GPU body does not execute.
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
#include "generic_material_blend_class_candidate.h"
#include "vertexlit_generic_post_blend_candidate.h"
#include <xmmintrin.h>
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t serial,integrated,first,second,alpha,snapshot,flags,flags2,baseType,detailType,envType,alpha0,alpha1,alpha2,mutation;};
struct Var{uintptr_t table;uint32_t id,integer;std::array<uint32_t,4>vec;uint32_t type,parent;};
struct Resource{uintptr_t table;uint32_t id;};struct Object{uintptr_t table;};
static Query q;static std::array<std::array<Var,125>,2>vars;static std::array<std::array<Var*,125>,2>lists;
static std::array<Resource,3>resources;static Object shader,api;static uint32_t current=0,snapshot=0,result=0,prefix=0;
static std::array<uint32_t,14> prepared;static uint32_t shadowStorage=0;
static uint32_t context=0;static std::array<uint32_t,110>baseInfo;
static std::vector<std::array<uint32_t,8>>events;
#ifndef _WIN64
static uint8_t*imageBase;
static void word(size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(imageBase+at,&w,4);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT case="<<q.serial<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static_assert(offsetof(Var,integer)==8&&offsetof(Var,type)==28,"native variable fields");
#endif
static void globals(){
#ifndef _WIN64
 word(0xdb944,reinterpret_cast<uintptr_t>(lists[current].data()));word(0xdb94c,snapshot);
#endif
}
static void event(uint32_t kind,uint32_t a,uint32_t b,uint32_t c){events.push_back({kind,a,b,c,current,vars[0][0].integer,vars[1][0].integer,snapshot});}
static uint32_t resourceIndex(uint32_t variable){if(variable==6)return 0;if(variable==17)return 1;if(variable==22)return 2;throw std::runtime_error("texture variable domain");}
static uintptr_t texture(uintptr_t p){auto&v=*reinterpret_cast<Var*>(p);need(v.parent<2&&v.id<125&&p==reinterpret_cast<uintptr_t>(&vars[v.parent][v.id]),"captured live variable");auto r=(resourceIndex(v.id)+v.parent+uint32_t(bool(q.mutation&4)))%3;event(1,v.id,v.parent,r);if(q.mutation&1){current^=1;globals();}return reinterpret_cast<uintptr_t>(&resources[r]);}
static uint32_t alpha(uintptr_t p){const auto&r=*reinterpret_cast<Resource*>(p);need(r.id<3&&p==reinterpret_cast<uintptr_t>(&resources[r.id]),"captured returned resource");const uint32_t values[]{q.alpha0,q.alpha1,q.alpha2};const auto value=values[r.id];event(2,r.id,value,0);if(q.mutation&2){vars[0][0].integer^=0x180;vars[1][0].integer^=0x180;}if(q.mutation&8)current^=1;if(q.mutation&16)snapshot^=1;globals();return value;}
struct PostOps{
 uint32_t primaryFlags(){return vars[0][0].integer;}
 bool initialAlphaTest(){return (q.flags&0x100u)!=0;}
 int32_t integer(uint32_t offset){auto i=baseInfo.at(offset/4);if(i==0xffffffffu)return 0;int32_t v;std::memcpy(&v,&vars[0].at(i).integer,4);return v;}
 float scalar(uint32_t offset){auto i=baseInfo.at(offset/4);if(i==0xffffffffu)return 0;float v;std::memcpy(&v,&vars[0].at(i).vec[0],4);return v;}
};
#ifdef _WIN64
static uint32_t listId(uintptr_t p){for(uint32_t i=0;i<2;++i)if(p==reinterpret_cast<uintptr_t>(lists[i].data()))return i;throw std::runtime_error("list identity");}
struct Ops{
 uint32_t snapshotWord(){return snapshot;}uintptr_t currentParameters(){return reinterpret_cast<uintptr_t>(lists[current].data());}
 uint32_t typeByte(uintptr_t p,uint32_t i){return vars.at(listId(p)).at(i).type;}
 uint32_t primaryFlags(uintptr_t p){return vars.at(listId(p))[0].integer;}uint32_t secondaryFlags(uintptr_t p){return vars.at(listId(p))[2].integer;}
 uintptr_t variable(uintptr_t p,uint32_t i){return reinterpret_cast<uintptr_t>(&vars.at(listId(p)).at(i));}
 uintptr_t texture(uintptr_t p){return ::texture(p);}uint32_t textureAlpha(uintptr_t p){return ::alpha(p);}
};
#else
static void* __fastcall textureNative(void*p,void*){return reinterpret_cast<void*>(texture(reinterpret_cast<uintptr_t>(p)));}
static uint32_t __fastcall alphaNative(void*p,void*){return alpha(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall api17(void*,void*){return 0;}
static void __cdecl unused(){throw std::runtime_error("unexpected observer");}
static void checkpoint(unsigned index,CONTEXT&c){
 need(index==0&&q.integrated&&c.Eip==0x100557b0,"declared generic-prefix checkpoint");
 const auto frame=c.Ebp;auto read=[&](int32_t offset){uint32_t v;std::memcpy(&v,reinterpret_cast<const void*>(frame+offset),4);return v;};
 need(read(8)==reinterpret_cast<uintptr_t>(&shader)&&read(12)==reinterpret_cast<uintptr_t>(lists[0].data())&&c.Ebx==reinterpret_cast<uintptr_t>(lists[0].data())&&read(0x28)==reinterpret_cast<uintptr_t>(&context),"generic original list remains captured while classifier uses live globals");
 result=read(-0x49c);prefix=1;
 const int offsets[]{-0x4d8,-0x471,-0x494,-0x483,-0x482,-0x474,-0x4a0,-0x477,-0x478,-0x48a,-0x46e,-0x473,-0x48e};
 prepared[0]=result;for(unsigned i=0;i<13;++i)prepared[i+1]=(i==6)?read(offsets[i]):*reinterpret_cast<const uint8_t*>(frame+offsets[i]);
 need(c.Edi==(q.serial&1?reinterpret_cast<uintptr_t>(&shadowStorage):0),"live shadow route boundary");
 // Test-only stop at the verified prefix boundary. Restore the callee-saved
 // registers from this exact original prologue and return to its caller.
 // This bypasses the remaining renderer and its cookie/cleanup, proving neither.
 c.Ebx=read(-0x4ec);c.Esi=read(-0x4f0);c.Edi=read(-0x4f4);c.Eip=read(4);c.Esp=frame+8;c.Ebp=read(0);
}
#endif
static int32_t signedWord(uint32_t v){int32_t x;std::memcpy(&x,&v,4);return x;}
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==4&&!std::ifstream(argv[3]).good(),"mapping query fresh owned trace");const char*mapping=argv[1],*input=argv[2],*output=argv[3];
#else
 need(argc==5&&!std::ifstream(argv[4]).good(),"private shader mapping query fresh trace");const char*mapping=argv[2],*input=argv[3],*output=argv[4];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(0,&fault);NativePrivateWorldImageFixture image(argv[1],0xf0000);imageBase=image.base;
#endif
 std::ifstream map(mapping,std::ios::binary);uint32_t count;map.read(reinterpret_cast<char*>(&count),4);need(count==112,"registered bindings");for(uint32_t i=0;i<count;++i){uint32_t p[2];map.read(reinterpret_cast<char*>(p),8);need(p[0]>=0xc0000&&p[0]<0xdb000&&p[1]>=13&&p[1]<125,"data-only binding");
#ifndef _WIN64
 word(p[0],p[1]);
#endif
 }uint32_t length;map.read(reinterpret_cast<char*>(&length),4);need(length==110,"full generic info record");map.read(reinterpret_cast<char*>(baseInfo.data()),sizeof(baseInfo));need(bool(map)&&baseInfo[0]==6&&baseInfo[0x18/4]==17&&baseInfo[0x2c/4]==22,"exact base/detail/env index mapping");
#ifndef _WIN64
 std::array<uintptr_t,1>variableMethods{reinterpret_cast<uintptr_t>(&textureNative)};for(auto&v:vars)for(auto&p:v)p.table=reinterpret_cast<uintptr_t>(variableMethods.data());std::array<uintptr_t,7>textureMethods{};textureMethods[6]=reinterpret_cast<uintptr_t>(&alphaNative);for(auto&r:resources)r.table=reinterpret_cast<uintptr_t>(textureMethods.data());
 std::array<uintptr_t,18>apiMethods{};apiMethods[17]=reinterpret_cast<uintptr_t>(&api17);api.table=reinterpret_cast<uintptr_t>(apiMethods.data());shader.table=0x1009c39c;
 std::array<uint32_t,32>phong{};word(0xcaff4,reinterpret_cast<uintptr_t>(phong.data()));word(0xdb940,0);word(0xdb948,reinterpret_cast<uintptr_t>(&api));image.seal();
 PrivateX86BoundaryObserverFixture traps({0x100557b0,uint32_t(reinterpret_cast<uintptr_t>(&unused)),uint32_t(reinterpret_cast<uintptr_t>(&unused)),uint32_t(reinterpret_cast<uintptr_t>(&unused))},&checkpoint);traps.configure(true);
#endif
 for(uint32_t a=0;a<2;++a)for(uint32_t i=0;i<125;++i){vars[a][i].id=i;vars[a][i].parent=a;lists[a][i]=&vars[a][i];}for(uint32_t i=0;i<3;++i)resources[i].id=i;
 std::ifstream in(input,std::ios::binary);std::ofstream out(output,std::ios::binary);need(bool(in)&&bool(out),"query trace IO");uint32_t cases=0,eventCount=0,prefixCount=0;
 while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&q.integrated<2&&q.mutation<32,"query domain");for(uint32_t v:{q.first,q.second})need(signedWord(v)<0||v==6||v==17||v==22,"safe classifier index domain");events.clear();prefix=0;current=0;snapshot=q.snapshot;
  for(uint32_t a=0;a<2;++a){for(auto&v:vars[a]){v.integer=0;v.vec={};v.type=4;}vars[a][0].integer=q.flags^(a?0x180u:0u);vars[a][2].integer=q.flags2^(a?0x400000u:0u);vars[a][6].type=q.baseType;vars[a][17].type=q.detailType;vars[a][22].type=q.envType;vars[a][55].vec[0]=0x3f800000;}
  for(auto&list:vars){
   // Distinct values after callbacks expose accidental use of the shared list.
   for(uint32_t at:{0x138u,0x17cu,0xd4u,0xd8u}){auto i=baseInfo[at/4];if(i!=0xffffffffu)list[i].integer=q.alpha^(list[0].parent?0x80000001u:0u);}
   for(uint32_t at:{0x9cu,0x160u}){auto i=baseInfo[at/4];if(i!=0xffffffffu)list[i].vec[0]=(q.serial%5==0?0x7fc00001u:q.serial%5==1?0x80000000u:q.serial%5==2?0xbf800000u:q.serial%5==3?0x3f800000u:0x7f800000u)^(list[0].parent?0x80000000u:0u);}
  }
  _mm_setcsr(0x1f80);globals();
#ifndef _WIN64
  word(0xdb940,q.serial&1?reinterpret_cast<uintptr_t>(&shadowStorage):0);
#endif
#ifdef _WIN64
  Ops ops;if(q.integrated){const bool base=(q.baseType&15)==3;result=genericMaterialBlendClassCandidate(base?6:22,base?1u:0u,-1,ops);prefix=1;PostOps post;prepared=vertexlitGenericPostBlendCandidate(result,post);}else result=genericMaterialBlendClassCandidate(signedWord(q.first),q.alpha,signedWord(q.second),ops);
#else
  if(q.integrated){auto*table=*reinterpret_cast<uintptr_t**>(&shader);using Draw=void(__thiscall*)(void*,void*,void*,void*,uint32_t,void*);reinterpret_cast<Draw>(table[14])(&shader,lists[0].data(),nullptr,&api,0,&context);}
  else{using Blend=uint32_t(__thiscall*)(void*,int32_t,uint32_t,int32_t);result=reinterpret_cast<Blend>(image.base+0x2d20)(&shader,signedWord(q.first),q.alpha,signedWord(q.second));}
#endif
  need(result<4&&prefix==q.integrated,"classification/prefix return domain");const uint32_t header[]{cases,result,current,snapshot,vars[0][0].integer,vars[1][0].integer,prefix,uint32_t(events.size())};out.write(reinterpret_cast<const char*>(header),sizeof(header));if(q.integrated)out.write(reinterpret_cast<const char*>(prepared.data()),sizeof(prepared));for(const auto&e:events)out.write(reinterpret_cast<const char*>(e.data()),sizeof(e));eventCount+=uint32_t(events.size());prefixCount+=prefix;++cases;
 }
 need(in.eof()&&in.gcount()==0&&cases,"query framing");out.flush();need(bool(out),"complete trace");
#ifndef _WIN64
 traps.configure(false);need(traps.hits==prefixCount,"exact prefix stops only");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS extended_preparation_cases="<<cases<<" events="<<eventCount<<" generic_prefix_cases="<<prefixCount<<" complete_generic_renderer=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
