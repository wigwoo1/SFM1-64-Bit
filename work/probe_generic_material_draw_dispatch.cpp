// Real routing1005A820/predicate/query/skip, optionally through concrete14.
// Generic10055320 and Phong100380F0 rendering bodies remain declared observers.
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
#include "generic_material_draw_dispatch_candidate.h"
#include "shader_skin_material_predicate_candidate.h"
#include "studio_vertex_lit_passes_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t serial,integrated,shadow,sharedShadow,vertexLit,field14c,field150,query17,query59,matPhong,phong,warp,mask,bump,hardware,mutation,compression;};
struct Var{uintptr_t table;uint32_t id,integer;std::array<uint32_t,4>vec;uint32_t type;};
struct Object{uintptr_t table;uint32_t id;};
static Query q;static std::array<Var,125>vars;static std::array<Var*,125>params;
static std::array<uint32_t,110>baseInfo,infoWords;static std::array<uint32_t,32>phong;
static Object shader,shadowObject,service,hardwareObject;static std::array<Object,2>apis;
static uint32_t contextWord=0,ownedPass=0,sharedShadow=0,sharedApi=2,leaves=0;
static std::vector<std::array<uint32_t,12>>events;
#ifndef _WIN64
static uint8_t*imageBase;
static void word(size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(imageBase+at,&w,4);}
static uint32_t word(size_t at){uint32_t v;std::memcpy(&v,imageBase+at,4);return v;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT case="<<q.serial<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static_assert(offsetof(Var,integer)==8&&offsetof(Var,type)==28,"native variable fields");
#endif
static uint32_t pass(){
#ifdef _WIN64
 return ownedPass;
#else
 return word(0xdb958);
#endif
}
static void pass(uint32_t v){
#ifdef _WIN64
 ownedPass=v;
#else
 word(0xdb958,v);
#endif
}
static uint32_t vertexLit(){return q.integrated?1:q.vertexLit;}
static void globals(){
#ifndef _WIN64
 word(0xdb940,sharedShadow?reinterpret_cast<uintptr_t>(&shadowObject):0);word(0xdb948,reinterpret_cast<uintptr_t>(&apis[sharedApi-1]));
#endif
}
static void record(uint32_t kind,uint32_t a=0,uint32_t b=0,uint32_t receiver=0){events.push_back({kind,a,b,receiver,vertexLit(),pass(),sharedShadow,sharedApi,vars[100].integer,vars[101].integer,phong[12],q.shadow});}
static uint32_t defined(){record(3,q.bump,0,26);if(q.mutation&1){phong[12]=0;vars[100].integer^=1;}return q.bump;}
static int32_t level(){record(4,q.hardware,0,3);if(q.mutation&2){vars[100].integer^=1;vars[101].integer^=1;}int32_t n;std::memcpy(&n,&q.hardware,4);return n;}
static uint32_t query17(uint32_t receiver){record(5,q.query17,0,receiver);if(q.mutation&4)vars[101].integer^=1;return q.query17;}
static uint32_t query59(uint32_t receiver){record(6,q.query59,0,receiver);if(q.mutation&8){sharedShadow^=1;globals();}return q.query59;}
static void leaf(uint32_t kind,uint32_t flash,uint32_t mode){record(kind,flash,mode,1);++leaves;if(q.mutation&16){vars[100].integer^=1;vars[101].integer^=1;sharedShadow^=1;sharedApi=1;globals();}pass(pass()+1);}
static void skipped(){record(0);}
#ifdef _WIN64
static void checkArguments(uintptr_t s,uintptr_t p,uintptr_t a,uintptr_t sh,uintptr_t i,uint32_t c,uintptr_t ctx){need(s==reinterpret_cast<uintptr_t>(&shader)&&p==reinterpret_cast<uintptr_t>(params.data())&&a==reinterpret_cast<uintptr_t>(&apis[0])&&sh==(q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0)&&i==reinterpret_cast<uintptr_t>(infoWords.data())&&c==q.compression&&ctx==reinterpret_cast<uintptr_t>(&contextWord),"complete owned pointer-width supplied arguments");}
struct Ops{
 bool wantsSkin(uintptr_t,uintptr_t){ShaderSkinMaterialInputsCandidate inputs;inputs.matPhong=int32_t(phong[12]);inputs.phong=int32_t(vars[48].integer);inputs.diffuseWarpTypeByte=uint8_t(vars[43].type);inputs.baseAlphaPhongMask=int32_t(vars[49].integer);const bool bump=inputs.matPhong&&*inputs.phong&&((*inputs.diffuseWarpTypeByte&15)!=3)&&*inputs.baseAlphaPhongMask!=1;inputs.bumpDefined=bump?uint8_t(::defined())!=0:false;return shaderSkinMaterialPredicateCandidate(inputs).wantsSkin;}
 int32_t hardwareLevel(){return level();}uint32_t infoIndex(uintptr_t p,uint32_t offset){return reinterpret_cast<const uint32_t*>(p)[offset/4];}
 uint32_t integer(uintptr_t,uint32_t i){return vars.at(i).integer;}
 uint32_t flashlight(uintptr_t){return sharedShadow?((vars[2].integer>>10)&1): ::query17(sharedApi);}
 uint32_t query59(uintptr_t a){need(a==reinterpret_cast<uintptr_t>(&apis[0]),"owned supplied query59 API");return ::query59(1);}
 void skin(uintptr_t s,uintptr_t p,uintptr_t a,uintptr_t sh,uintptr_t i,uint32_t c,uintptr_t ctx){checkArguments(s,p,a,sh,i,c,ctx);leaf(2,0,0);}
 void generic(uintptr_t s,uintptr_t p,uintptr_t a,uintptr_t sh,uint32_t v,bool f,uint32_t m,uintptr_t i,uint32_t c,uintptr_t ctx){checkArguments(s,p,a,sh,i,c,ctx);need(v==vertexLit(),"owned raw vertexlit word");leaf(1,uint32_t(f),m);}
 void skip(uintptr_t,uint32_t n){need(n==0,"owned skip0");skipped();pass(pass()+1);}
};
static void ownedMain(){Ops ops;genericMaterialDrawDispatchCandidate(reinterpret_cast<uintptr_t>(&shader),reinterpret_cast<uintptr_t>(params.data()),reinterpret_cast<uintptr_t>(&apis[0]),q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0,vertexLit(),reinterpret_cast<uintptr_t>(infoWords.data()),q.compression,reinterpret_cast<uintptr_t>(&contextWord),ops);}
struct OuterOps{bool cloakEnabled(){return false;}bool emissiveEnabled(){return false;}bool fleshEnabled(){return false;}bool cloakOpaque(){return false;}float cloakFactor(){return 0;}float emissiveStrength(){return 0;}void skip(uint32_t){throw std::runtime_error("unexpected outer skip");}void mainLighting(){ownedMain();}void cloak(){throw 1;}void emissive(){throw 1;}void flesh(){throw 1;}};
#else
static uint32_t __fastcall definedNative(void*p,void*){need(p==&vars[26],"bump callback receiver");return defined();}
static int32_t __fastcall hardwareNative(void*,void*){return level();}
static uint32_t __fastcall api17Native(void*p,void*){auto&o=*static_cast<Object*>(p);return query17(o.id);}
static uint32_t __fastcall api59Native(void*p,void*){need(p==&apis[0],"query59 supplied API");return query59(1);}
static void __fastcall skipShadow(void*,void*){need(sharedShadow==1,"live shadow skip");skipped();}
static void __fastcall skipRuntime(void*,void*,uint32_t data,uint32_t draw){need(!sharedShadow&&data==0xaabbccdd&&draw==0,"runtime skip data/draw");skipped();}
static void boundary(unsigned index,CONTEXT&c){
 auto*a=reinterpret_cast<const uint32_t*>(c.Esp);need(index<2&&a[1]==reinterpret_cast<uintptr_t>(&shader)&&a[2]==reinterpret_cast<uintptr_t>(params.data()),"draw shader/parameter args");const uint32_t*actual=nullptr;
 if(index==0){need(c.Edx==reinterpret_cast<uintptr_t>(&apis[0])&&a[3]==(q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0)&&a[4]==vertexLit()&&a[6]<2&&a[8]==q.compression&&a[9]==reinterpret_cast<uintptr_t>(&contextWord),"generic custom EDX/stack ABI");actual=reinterpret_cast<const uint32_t*>(a[7]);}
 else{need(a[3]==reinterpret_cast<uintptr_t>(&apis[0])&&a[4]==(q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0)&&a[6]==q.compression&&a[7]==reinterpret_cast<uintptr_t>(&contextWord),"Phong cdecl ABI");actual=reinterpret_cast<const uint32_t*>(a[5]);}
 need(std::memcmp(actual,infoWords.data(),sizeof(infoWords))==0,"retained complete info record");leaf(index?2u:1u,index?0u:uint8_t(a[5]),index?0u:a[6]);PrivateX86BoundaryObserverFixture::returnCdecl(c);
}
static void __cdecl unusedBoundary(){throw std::runtime_error("unexpected unused boundary");}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==4&&!std::ifstream(argv[3]).good(),"mapping query fresh trace");const char*mapping=argv[1],*inputPath=argv[2],*outputPath=argv[3];
#else
 need(argc==5&&!std::ifstream(argv[4]).good(),"private shader mapping query fresh trace");const char*mapping=argv[2],*inputPath=argv[3],*outputPath=argv[4];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(0,&fault);NativePrivateWorldImageFixture image(argv[1],0xf0000);imageBase=image.base;
#endif
 std::ifstream map(mapping,std::ios::binary);uint32_t count;map.read(reinterpret_cast<char*>(&count),4);need(count==112,"schema bindings");for(uint32_t i=0;i<count;++i){uint32_t p[2];map.read(reinterpret_cast<char*>(p),8);need(p[0]>=0xc0000&&p[0]<0xdb000&&p[1]>=13&&p[1]<125,"data-only binding");
#ifndef _WIN64
 word(p[0],p[1]);
#endif
 }uint32_t length;map.read(reinterpret_cast<char*>(&length),4);need(length==110,"base info extent");map.read(reinterpret_cast<char*>(baseInfo.data()),sizeof(baseInfo));need(bool(map),"mapping framing");
#ifndef _WIN64
 std::array<uintptr_t,18>variableMethods{};variableMethods[17]=reinterpret_cast<uintptr_t>(&definedNative);for(auto&v:vars)v.table=reinterpret_cast<uintptr_t>(variableMethods.data());
 std::array<uintptr_t,60>apiMethods{};apiMethods[17]=reinterpret_cast<uintptr_t>(&api17Native);apiMethods[59]=reinterpret_cast<uintptr_t>(&api59Native);for(auto&o:apis)o.table=reinterpret_cast<uintptr_t>(apiMethods.data());
 std::array<uintptr_t,18>hardwareMethods{};hardwareMethods[17]=reinterpret_cast<uintptr_t>(&hardwareNative);hardwareObject.table=reinterpret_cast<uintptr_t>(hardwareMethods.data());
 std::array<uintptr_t,5>serviceMethods{};serviceMethods[3]=reinterpret_cast<uintptr_t>(&skipShadow);serviceMethods[4]=reinterpret_cast<uintptr_t>(&skipRuntime);service.table=reinterpret_cast<uintptr_t>(serviceMethods.data());shader.table=0x1009c39c;
 std::array<uint8_t,128>configuration{};std::array<uint32_t,2>instance{0,0xaabbccdd};std::array<uintptr_t,8>instanceRecords{};instanceRecords.fill(reinterpret_cast<uintptr_t>(instance.data()));
 word(0xdb9dc,reinterpret_cast<uintptr_t>(configuration.data()));word(0xdb9e0,reinterpret_cast<uintptr_t>(&service));word(0xdb95c,reinterpret_cast<uintptr_t>(instanceRecords.data()));word(0xdb9d8,reinterpret_cast<uintptr_t>(&hardwareObject));word(0xcaff4,reinterpret_cast<uintptr_t>(phong.data()));word(0xdb944,reinterpret_cast<uintptr_t>(params.data()));image.seal();
 PrivateX86BoundaryObserverFixture traps({0x10055320,0x100380f0,uint32_t(reinterpret_cast<uintptr_t>(&unusedBoundary)),uint32_t(reinterpret_cast<uintptr_t>(&unusedBoundary))},&boundary);traps.configure(true);
#endif
 for(uint32_t i=0;i<125;++i){vars[i].id=i;params[i]=&vars[i];}apis[0].id=1;apis[1].id=2;
 std::ifstream input(inputPath,std::ios::binary);std::ofstream output(outputPath,std::ios::binary);need(bool(input)&&bool(output),"query trace IO");uint32_t cases=0,eventCount=0,leafCount=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&q.integrated<2&&q.shadow<2&&q.sharedShadow<2&&q.field14c<3&&q.field150<3&&q.mutation<32,"query domain");events.clear();leaves=0;pass(0);sharedShadow=q.sharedShadow;sharedApi=2;globals();for(auto&v:vars){v.integer=0;v.type=4;}
  phong.fill(0);phong[12]=q.matPhong;vars[48].integer=q.phong;vars[43].type=q.warp;vars[49].integer=q.mask;vars[100].integer=uint32_t(q.field14c==2);vars[101].integer=uint32_t(q.field150==2);vars[2].integer=uint8_t(q.query17)?0x400:0;
  infoWords=baseInfo;if(!q.integrated){infoWords[0x14c/4]=q.field14c?100:0xffffffff;infoWords[0x150/4]=q.field150?101:0xffffffff;}
#ifdef _WIN64
  if(q.integrated){OuterOps outer;studioVertexLitPassesCandidate(q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0,outer);}else ownedMain();
#else
  if(q.integrated){auto*table=*reinterpret_cast<uintptr_t**>(&shader);using Fn=void(__thiscall*)(void*,void*,void*,void*,uint32_t,void*);reinterpret_cast<Fn>(table[14])(&shader,params.data(),q.shadow?&shadowObject:nullptr,&apis[0],q.compression,&contextWord);}
  else{using Fn=void(__cdecl*)(void*,void*,void*,void*,uint32_t,void*,uint32_t,void*);reinterpret_cast<Fn>(image.base+0x5a820)(&shader,params.data(),&apis[0],q.shadow?&shadowObject:nullptr,q.vertexLit,infoWords.data(),q.compression,&contextWord);}
#endif
  need(leaves&&leaves<=2&&pass()<=2,"bounded generic/Phong consumption profile");const uint32_t header[]{cases,uint32_t(events.size()),leaves,pass(),sharedShadow,sharedApi,vars[100].integer,vars[101].integer,phong[12]};output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(e.data()),sizeof(e));eventCount+=uint32_t(events.size());leafCount+=leaves;++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"complete query framing");output.flush();need(bool(output),"complete trace");
#ifndef _WIN64
 traps.configure(false);need(traps.hits==leafCount,"two declared rendering boundaries only");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS generic_dispatch_cases="<<cases<<" events="<<eventCount<<" renderer_boundaries="<<leafCount<<" generic_phong_bodies=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
