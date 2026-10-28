// Whole original material slot71/direct+forwarded vs existing owned adapter.
// Original instructions remain RX/unmodified; services are explicit observers.
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
#include "studio_material_draw_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t index,shader,parameters,forward,mutations,flags,alpha,compression,mode,cookie,encoded;};
static_assert(sizeof(Query)==44,"reference query wire");
struct Event{std::array<uint32_t,9>words;};
struct Shader{uintptr_t table;uint32_t id;};
static Query q;
static std::array<Shader,2>shaders;
static std::vector<Event>events;
static uint32_t missing=0,callbacks=0,draws=0,checks=0;
static uintptr_t cookie=0;
static StudioMaterialDrawStateCandidate state;
static std::array<uint32_t,3>block;
static uint32_t shaderId(uintptr_t p){if(!p)return 0;for(const auto&s:shaders)if(reinterpret_cast<uintptr_t>(&s)==p)return s.id;throw std::runtime_error("invalid shader identity");}
#ifdef _WIN64
struct Param{StudioMaterialDrawParameterCandidate parameter;uint32_t value;};
static std::array<Param,2>parameters;
static std::array<StudioMaterialDrawParameterCandidate*,2>lists;
#else
struct Param{uint32_t pad0,pad1,flags,value;};
static std::array<Param,2>parameters;
static std::array<Param*,2>lists;
static std::array<uint8_t,0x90>material;
static uint8_t*imageBase=nullptr;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void put(size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(material.data()+at,&w,4);}
static uint32_t get(size_t at){uint32_t w;std::memcpy(&w,material.data()+at,4);return w;}
#endif
static uintptr_t list(uint32_t id){return id?reinterpret_cast<uintptr_t>(&lists.at(id-1)):0;}
static uint32_t listId(uintptr_t p){if(!p)return 0;for(uint32_t i=0;i<2;++i)if(list(i+1)==p)return i+1;throw std::runtime_error("invalid parameter list");}
static uint32_t parameterFlags(uint32_t i){
#ifdef _WIN64
 return parameters[i].parameter.flags;
#else
 return parameters[i].flags;
#endif
}
static void parameterFlags(uint32_t i,uint32_t flags){
#ifdef _WIN64
 parameters[i].parameter.flags=flags;
#else
 parameters[i].flags=flags;
#endif
}
static uintptr_t liveShader(){
#ifdef _WIN64
 return state.shader;
#else
 return get(0x14);
#endif
}
static void liveShader(uintptr_t p){
#ifdef _WIN64
 state.shader=p;
#else
 put(0x14,p);
#endif
}
static uintptr_t liveList(){
#ifdef _WIN64
 return reinterpret_cast<uintptr_t>(state.parameters);
#else
 return get(0x24);
#endif
}
static void liveList(uintptr_t p){
#ifdef _WIN64
 state.parameters=reinterpret_cast<StudioMaterialDrawParameterCandidate**>(p);
#else
 put(0x24,p);
#endif
}
static uintptr_t encoded(){
#ifdef _WIN64
 return state.encodedContext;
#else
 return get(0x78);
#endif
}
static void encoded(uintptr_t p){
#ifdef _WIN64
 state.encodedContext=p;
#else
 put(0x78,p);
#endif
}
static uint32_t blockWord(size_t i){
#ifdef _WIN64
 return block[i];
#else
 return get(0x30+i*4);
#endif
}
static void blockWord(size_t i,uint32_t v){
#ifdef _WIN64
 block[i]=v;
#else
 put(0x30+i*4,v);
#endif
}
static void missingShader(){++missing;events.push_back({{0,0,0,0,0,0,0,0,0}});}
static void shaderCallback(uintptr_t p){need(shaderId(p)==1,"original pre-callback shader receiver");++callbacks;events.push_back({{1,1,0,0,0,0,0,0,0}});if(q.mutations&1u)liveShader(reinterpret_cast<uintptr_t>(&shaders[1]));if(q.mutations&2u)liveList(list(2));if(q.mutations&4u){cookie^=0xf1e2d3c4u;encoded(encoded()^0x12345678u);
#ifndef _WIN64
 uint32_t key=uint32_t(cookie);std::memcpy(imageBase+0x1059f8,&key,4);
#endif
 }if(q.mutations&8u){parameterFlags(0,parameterFlags(0)^4u);parameterFlags(1,parameterFlags(1)^4u);parameters[0].value+=7;parameters[1].value+=11;}if(q.mutations&16u)for(size_t i=0;i<3;++i)blockWord(i,blockWord(i)^0xdeadbeef);if(q.mutations&32u)liveShader(0);}
static void draw(uintptr_t shader,uintptr_t params,uintptr_t shaderState,uint32_t compression,uintptr_t token,uint32_t alpha,uint32_t mode){
#ifdef _WIN64
 need(shaderState==reinterpret_cast<uintptr_t>(block.data()),"stable owned state block address");
#else
 need(shaderState==reinterpret_cast<uintptr_t>(material.data()+0x30),"original embedded state address");
#endif
 const auto lid=listId(params);uint32_t flags=0,value=0;if(lid&&lists[lid-1]){flags=parameterFlags(lid-1);value=parameters[lid-1].value;}
 need(shader==liveShader()&&params==liveList()&&token==(encoded()^cookie)&&compression==q.compression&&alpha==uint32_t(uint8_t(q.alpha)!=0)&&mode==q.mode,"post-callback live values and raw mode/low-byte alpha");++checks;++draws;events.push_back({{2,shaderId(shader),lid,flags,value,compression,uint32_t(token),alpha,mode}});
}
#ifdef _WIN64
struct Ops{void missingShader(){::missingShader();}void shaderCallback(uintptr_t p){::shaderCallback(p);}uintptr_t cookie(){return ::cookie;}void draw(uintptr_t s,uintptr_t p,uintptr_t st,uint32_t c,uintptr_t t,uint32_t a,uint32_t m){::draw(s,p,st,c,t,a,m);}};
#else
static void __cdecl missingNative(const char*p){need(reinterpret_cast<uintptr_t>(p)==0x10095a10,"original missing shader diagnostic");missingShader();}
static void __fastcall callbackNative(void*p,void*){shaderCallback(reinterpret_cast<uintptr_t>(p));}
static void __fastcall drawNative(void*,void*,void*s,void*p,void*st,uint32_t c,uint32_t t,uint32_t a,uint32_t m){draw(reinterpret_cast<uintptr_t>(s),reinterpret_cast<uintptr_t>(p),reinterpret_cast<uintptr_t>(st),c,t,a,m);}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==3&&!std::ifstream(argv[2]).good(),"query and fresh owned trace");const char*inputPath=argv[1],*outputPath=argv[2];
#else
 need(argc==4&&!std::ifstream(argv[3]).good(),"read-only material image/query/fresh trace");const char*inputPath=argv[2],*outputPath=argv[3];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0x100000);imageBase=image.base;std::array<uintptr_t,19>systemMethods{};systemMethods[18]=reinterpret_cast<uintptr_t>(&drawNative);uintptr_t system=reinterpret_cast<uintptr_t>(systemMethods.data());uint32_t receiver=uint32_t(reinterpret_cast<uintptr_t>(&system));std::memcpy(image.base+0x1017f8,&receiver,4);auto warning=reinterpret_cast<uintptr_t>(&missingNative);std::memcpy(image.base+0x951e4,&warning,4);image.seal();std::array<uintptr_t,1>shaderMethods{reinterpret_cast<uintptr_t>(&callbackNative)};
#endif
 std::ifstream input(inputPath,std::ios::binary);std::ofstream output(outputPath,std::ios::binary);need(bool(input)&&bool(output),"query/trace IO");uint32_t cases=0,eventCount=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.index==cases&&q.shader<=1&&q.parameters<3&&q.forward<=1&&q.mutations<64,"query domain");missing=callbacks=draws=0;events.clear();cookie=q.cookie;for(uint32_t i=0;i<2;++i){shaders[i]={0,i+1};parameterFlags(i,q.flags^(i?4u:0u));parameters[i].value=(i?0x22220000u:0x11110000u)+q.index;}
#ifdef _WIN64
  lists[0]=q.parameters==2?&parameters[0].parameter:nullptr;lists[1]=&parameters[1].parameter;state={};state.shaderState=reinterpret_cast<uintptr_t>(block.data());
#else
  for(auto&s:shaders)s.table=reinterpret_cast<uintptr_t>(shaderMethods.data());lists[0]=q.parameters==2?&parameters[0]:nullptr;lists[1]=&parameters[1];material.fill(0);put(0,0x10095d74);uint32_t key=uint32_t(cookie);std::memcpy(image.base+0x1059f8,&key,4);
#endif
  liveShader(q.shader?reinterpret_cast<uintptr_t>(&shaders[0]):0);liveList(q.parameters?list(1):0);encoded(q.encoded);blockWord(0,q.index);blockWord(1,q.compression);blockWord(2,q.mode);
#ifdef _WIN64
  Ops ops;studioMaterialDrawCandidate(state,q.compression,q.alpha,q.mode,ops);
#else
  std::array<uintptr_t,2>forward{0x10095a9c,reinterpret_cast<uintptr_t>(material.data())};auto*target=q.forward?static_cast<void*>(forward.data()):static_cast<void*>(material.data());auto*table=*reinterpret_cast<uintptr_t**>(target);using Call=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);reinterpret_cast<Call>(table[71])(target,q.compression,q.alpha,q.mode);
#endif
  const bool allowed=q.shader&&!(q.parameters==2&&(q.flags&4u));need(missing==uint32_t(!q.shader)&&callbacks==uint32_t(allowed)&&draws==uint32_t(allowed),"independent gates and no recheck after callback");++checks;
  const uint32_t words[16]{cases,missing,callbacks,draws,shaderId(liveShader()),listId(liveList()),parameterFlags(0),parameters[0].value,uint32_t(cookie),uint32_t(encoded()),blockWord(0),blockWord(1),blockWord(2),parameterFlags(1),parameters[1].value,uint32_t(events.size())};output.write(reinterpret_cast<const char*>(words),sizeof(words));for(const auto&e:events)output.write(reinterpret_cast<const char*>(e.words.data()),sizeof(e.words));eventCount+=uint32_t(events.size());++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"complete query framing");output.flush();need(bool(output),"complete trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS material_draw_reference_cases="<<cases<<" events="<<eventCount<<" independent_checks="<<checks<<" live_parameters=PASS callback_gate_not_rechecked=PASS shader_system_body=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
