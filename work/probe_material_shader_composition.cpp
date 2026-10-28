// Connected actual material slot71 -> actual ShaderSystem slot18 normal route.
// Owned side composes the two already-verified x64 helpers, without changing them.
// No DllMain/import loading; only data globals/IAT patched; code unchanged RX.
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
#include "shader_system_normal_draw_candidate.h"
#include "studio_material_draw_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t serial,selection,alpha,available,flags,mutations,contextMode,token,compression,materialMutations,present,forward,cookie,mode;};
struct Context{uint32_t prefix,key;uint8_t dirty;};
struct Snapshot{uint32_t available,word,pad;Context*context;uint32_t middle[3];std::array<uint32_t,4>tail;};
struct ShaderState{uint32_t pad[6];Snapshot*array;};
struct Object{uintptr_t table;uint32_t id;};
struct Event{std::array<uint32_t,8>words;};
static Query q;static std::array<Context,4>contexts;
static std::array<std::array<Snapshot,512>,2>arrays;
static std::array<ShaderState*,2>shaderStates;static ShaderState externalState;
struct Parameter{uintptr_t table;uint32_t unused;StudioMaterialDrawParameterCandidate parameter;uint32_t value;};
static std::array<Parameter,2>parameters;
#ifdef _WIN64
struct MaterialOwner{StudioMaterialDrawStateCandidate draw;ShaderState embedded;};
static MaterialOwner owner;
static std::array<std::array<StudioMaterialDrawParameterCandidate*,3>,2>lists;
#else
static std::array<uint8_t,0x90>materialStorage;
static std::array<std::array<Parameter*,3>,2>lists;
static_assert(offsetof(Parameter,parameter)==8,"native parameter flags");
#endif
static uint32_t callbacks=0,missing=0,cookie=0;
static std::array<Object,2>apis,shaders;static Object material;
static uintptr_t api;
static std::vector<Event>events;static ShaderSystemNormalDrawStateCandidate state;
static uint32_t draws=0,checks=0;static std::array<uint8_t,0x70>systemStorage;
static void diagnosticWord(size_t at,uint32_t v){std::memcpy(systemStorage.data()+at,&v,4);}
static uint32_t diagnosticWord(size_t at){uint32_t v;std::memcpy(&v,systemStorage.data()+at,4);return v;}
#ifndef _WIN64
static uint8_t*imageBase;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void put(size_t o,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(systemStorage.data()+o,&w,4);}
static uint32_t get(size_t o){uint32_t w;std::memcpy(&w,systemStorage.data()+o,4);return w;}
static void materialWord(size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(materialStorage.data()+at,&w,4);}
static uint32_t materialWord(size_t at){uint32_t w;std::memcpy(&w,materialStorage.data()+at,4);return w;}
static_assert(sizeof(Snapshot)==44&&offsetof(Snapshot,context)==12&&offsetof(Snapshot,tail)==28,"native record fields");
#endif
static uintptr_t current(){
#ifdef _WIN64
 return state.currentState;
#else
 return get(0x54);
#endif
}
static void current(uintptr_t p){
#ifdef _WIN64
 state.currentState=p;
#else
 put(0x54,p);
#endif
}
static uint8_t indexByte(){
#ifdef _WIN64
 return state.snapshot;
#else
 return systemStorage[0x5a];
#endif
}
static void indexByte(uint8_t x){
#ifdef _WIN64
 state.snapshot=x;
#else
 systemStorage[0x5a]=x;
#endif
}
static uint8_t flagByte(){
#ifdef _WIN64
 return state.flag;
#else
 return systemStorage[0x5b];
#endif
}
static void flagByte(uint8_t x){
#ifdef _WIN64
 state.flag=x;
#else
 systemStorage[0x5b]=x;
#endif
}
static void apiValue(uintptr_t p){api=p;
#ifndef _WIN64
 uint32_t v=uint32_t(p);std::memcpy(imageBase+0x11b098,&v,4);
#endif
}
static uint32_t apiId(uintptr_t p){for(const auto&a:apis)if(reinterpret_cast<uintptr_t>(&a)==p)return a.id;throw std::runtime_error("API identity");}
static uint32_t stateId(uintptr_t p){if(!p)return 0;for(uint32_t i=0;i<2;++i)if(reinterpret_cast<uintptr_t>(shaderStates[i])==p)return i+1;throw std::runtime_error("state identity");}
static uint32_t shaderId(uintptr_t p){if(!p)return 0;for(const auto&v:shaders)if(reinterpret_cast<uintptr_t>(&v)==p)return v.id;throw std::runtime_error("shader identity");}
static uintptr_t list(uint32_t id){return reinterpret_cast<uintptr_t>(lists.at(id-1).data());}
static uint32_t listId(uintptr_t p){for(uint32_t i=1;i<=2;++i)if(list(i)==p)return i;throw std::runtime_error("parameter-list identity");}
static Parameter&parameterFor(uintptr_t p){return parameters.at(listId(p)-1);}
static uintptr_t liveShader(){
#ifdef _WIN64
 return owner.draw.shader;
#else
 return materialWord(0x14);
#endif
}
static void liveShader(uintptr_t p){
#ifdef _WIN64
 owner.draw.shader=p;
#else
 materialWord(0x14,p);
#endif
}
static uintptr_t liveList(){
#ifdef _WIN64
 return reinterpret_cast<uintptr_t>(owner.draw.parameters);
#else
 return materialWord(0x24);
#endif
}
static void liveList(uintptr_t p){
#ifdef _WIN64
 owner.draw.parameters=reinterpret_cast<StudioMaterialDrawParameterCandidate**>(p);
#else
 materialWord(0x24,p);
#endif
}
static uint32_t encoded(){
#ifdef _WIN64
 return uint32_t(owner.draw.encodedContext);
#else
 return materialWord(0x78);
#endif
}
static void encoded(uint32_t v){
#ifdef _WIN64
 owner.draw.encodedContext=v;
#else
 materialWord(0x78,v);
#endif
}
static uint32_t decoded(){return encoded()^cookie;}
static uint32_t effectiveIndex(){return q.selection|uint32_t(uint8_t(q.alpha)!=0);}
static uint32_t contextId(Context*p){if(!p)return 0;for(uint32_t i=0;i<4;++i)if(&contexts[i]==p)return i+1;throw std::runtime_error("context identity");}
static Snapshot&snapshot(uintptr_t p,uint32_t i){need(i<512,"snapshot bounds");stateId(p);return reinterpret_cast<ShaderState*>(p)->array[i];}
static void event(uint32_t kind,uint32_t a=0,uint32_t b=0,uint32_t c=0,uint32_t d=0,uint32_t e=0,uint32_t f=0,uint32_t g=0){events.push_back({{kind,a,b,c,d,e,f,g}});}
static void resetApi(uintptr_t p){event(1,apiId(p));if(q.mutations&1)apiValue(reinterpret_cast<uintptr_t>(&apis[1]));}
static uint32_t select(uintptr_t s,uintptr_t params,uintptr_t ap){need(s==liveShader()&&params==liveList(),"selector receives post-material-callback shader/list");event(2,apiId(ap),shaderId(s),listId(params),parameterFor(params).value);if(q.mutations&2)shaderStates[0]->array=arrays[1].data();return q.selection;}
static uint32_t parameterFlags(uintptr_t pa){need(pa==liveList(),"live parameter list for flags");const auto f=parameterFor(pa).parameter.flags;event(3,f);return f;}
static void beginApi(uintptr_t p){event(4,apiId(p));if(q.mutations&4)parameterFor(liveList()).parameter.flags^=0x1000;}
static void api100(uintptr_t p,uint32_t v){need(v==0,"API100 zero");event(5,apiId(p),v);}
static void pushLogging(uint32_t a,uint32_t b){need(a==1&&b==1,"logging push arguments");event(6,a,b);}
static void registerListener(uintptr_t p){
#ifndef _WIN64
 need(p==reinterpret_cast<uintptr_t>(systemStorage.data()+0x1c),"embedded listener address");
#else
 (void)p;
#endif
 event(7,1);}
static void setPolicy(uintptr_t p){
#ifndef _WIN64
 need(p==reinterpret_cast<uintptr_t>(systemStorage.data()+0x50),"embedded policy address");
#else
 (void)p;
#endif
 event(8,1);}
static void useSnapshot(uintptr_t p,uint32_t word){event(9,apiId(p),word);need(word<=0xffff&&current()==reinterpret_cast<uintptr_t>(shaderStates[0])&&indexByte()==uint8_t(effectiveIndex())&&flagByte()==0,"material-owned embedded state reaches original system body");++checks;
 if(q.mutations&8)current(reinterpret_cast<uintptr_t>(shaderStates[1]));
 if(q.mutations&16)indexByte(uint8_t(indexByte()+1));
 if(q.mutations&32){auto&s=*reinterpret_cast<ShaderState*>(current());s.array=s.array==arrays[0].data()?arrays[1].data():arrays[0].data();}
 if(q.mutations&64)apiValue(reinterpret_cast<uintptr_t>(&apis[apiId(api)==1?1:0]));
}
static void draw(uintptr_t sh,uintptr_t pa,uint32_t full,uint32_t zero,uintptr_t ap,uint32_t compression,Context**cell,uint32_t*tail){
 need(sh==liveShader()&&pa==liveList()&&zero==0&&ap==api&&full==effectiveIndex()&&compression==q.compression,"composed draw arguments");
 auto&rec=snapshot(current(),indexByte());need(cell==&rec.context&&tail==rec.tail.data(),"live context cell and trailing block addresses");
 const auto*c=*cell;need(!c||c->key==decoded(),"live material XOR token reaches context key");event(10,full,compression,contextId(*cell),tail[0],c?c->key:0,c?c->dirty:0,apiId(ap));
 tail[0]+=7;if(q.mutations&128){*cell=&contexts[3];current(reinterpret_cast<uintptr_t>(shaderStates[1]));indexByte(77);flagByte(0xb5);}++draws;++checks;
}
static void popLogging(uint32_t a){need(a==1,"logging pop argument");event(11,a,stateId(current()),indexByte(),flagByte());}
static void resetMaterialMode(uint32_t a){need(a==0&&current()==0,"mode reset after current-state cleanup");event(12,a,stateId(current()),indexByte(),flagByte());++checks;}
static void missingShader(){event(14);++missing;}
static void shaderCallback(uintptr_t p){need(shaderId(p)==1,"material callback initial shader");event(13,1);++callbacks;
 if(q.materialMutations&1)liveShader(reinterpret_cast<uintptr_t>(&shaders[1]));
 if(q.materialMutations&2)liveList(list(2));
 if(q.materialMutations&4){cookie^=0xf1e2d3c4u;encoded(encoded()^0x12345678u);
#ifndef _WIN64
 std::memcpy(imageBase+0x1059f8,&cookie,4);
#endif
 }
 if(q.materialMutations&8){for(uint32_t i=0;i<2;++i){parameters[i].parameter.flags^=0x1004;parameters[i].value+=i?11:7;}}
 if(q.materialMutations&16){shaderStates[0]->array=arrays[1].data();shaderStates[0]->pad[0]^=0xabcdef01;}
}
#ifdef _WIN64
struct Ops{
 void resetApi(){::resetApi(api);}uint32_t select(uintptr_t s,uintptr_t p){return ::select(s,p,api);}
 bool available(uintptr_t s,uint32_t i){return snapshot(s,i).available!=0;}
 uint32_t parameterFlags(uintptr_t p){return ::parameterFlags(p);}void beginApi(){::beginApi(api);}void api100(uint32_t v){::api100(api,v);}
 void pushLogging(uint32_t a,uint32_t b){::pushLogging(a,b);}void registerListener(){::registerListener(0);}void setResponsePolicy(){setPolicy(0);}
 uint32_t snapshotWord(uintptr_t s,uint8_t i){return snapshot(s,i).word;}void useSnapshot(uint16_t w){::useSnapshot(api,w);}
 uintptr_t contextCell(uintptr_t s,uint8_t i){return reinterpret_cast<uintptr_t>(&snapshot(s,i).context);}
 Context*context(uintptr_t c){return *reinterpret_cast<Context**>(c);}uint32_t contextKey(Context*c){return c->key;}
 void dirtyContext(Context*c){c->dirty=1;}void setContextKey(Context*c,uint32_t v){c->key=v;}
 uintptr_t snapshotTail(uintptr_t s,uint8_t i){return reinterpret_cast<uintptr_t>(snapshot(s,i).tail.data());}
 void draw(uintptr_t s,uintptr_t p,uint32_t i,uint32_t c,uintptr_t cell,uintptr_t tail){::draw(s,p,i,0,api,c,reinterpret_cast<Context**>(cell),reinterpret_cast<uint32_t*>(tail));}
 void popLogging(uint32_t a){::popLogging(a);}void clearDiagnosticStorage(){for(size_t at:{0x2cu,0x30u,0x3cu,0x40u})diagnosticWord(at,0);systemStorage[0x34]=0;}void resetMaterialMode(uint32_t a){::resetMaterialMode(a);}
};
struct MaterialOps{
 void missingShader(){::missingShader();}void shaderCallback(uintptr_t p){::shaderCallback(p);}uintptr_t cookie(){return ::cookie;}
 void draw(uintptr_t s,uintptr_t p,uintptr_t st,uint32_t c,uintptr_t token,uint32_t alpha,uint32_t mode){
  need(st==reinterpret_cast<uintptr_t>(&owner.embedded)&&token==decoded()&&uint8_t(mode)==0,"stable embedded owned state and normal-route mode");
  Ops ops;shaderSystemNormalDrawCandidate(state,s,p,st,c,uint32_t(token),alpha,ops);
 }
};
#else
static void __fastcall materialCallbackNative(void*p,void*){shaderCallback(reinterpret_cast<uintptr_t>(p));}
static void __cdecl missingNative(const char*p){need(reinterpret_cast<uintptr_t>(p)==0x10095a10,"missing shader diagnostic");missingShader();}
static void __fastcall resetNative(void*p,void*){resetApi(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall selectNative(void*p,void*,void*pa,void*ap){return select(reinterpret_cast<uintptr_t>(p),reinterpret_cast<uintptr_t>(pa),reinterpret_cast<uintptr_t>(ap));}
static uint32_t __fastcall flagsNative(void*p,void*){need(p==&parameterFor(liveList()),"flags receiver");return parameterFlags(liveList());}
static void __fastcall beginNative(void*p,void*){beginApi(reinterpret_cast<uintptr_t>(p));}
static void __fastcall api100Native(void*p,void*,uint32_t v){api100(reinterpret_cast<uintptr_t>(p),v);}
static void __cdecl pushNative(uint32_t a,uint32_t b){pushLogging(a,b);}
static void __cdecl listenerNative(void*p){registerListener(reinterpret_cast<uintptr_t>(p));}
static void __cdecl policyNative(void*p){setPolicy(reinterpret_cast<uintptr_t>(p));}
static void __fastcall useNative(void*p,void*,uint32_t w){useSnapshot(reinterpret_cast<uintptr_t>(p),w);}
static void __fastcall drawNative(void*s,void*,void*p,uint32_t i,uint32_t z,void*ap,uint32_t c,Context**cell,uint32_t*tail){draw(reinterpret_cast<uintptr_t>(s),reinterpret_cast<uintptr_t>(p),i,z,reinterpret_cast<uintptr_t>(ap),c,cell,tail);}
static void __cdecl popNative(uint32_t a){popLogging(a);}
static void __fastcall modeNative(void*,void*,uint32_t a){resetMaterialMode(a);}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==3&&!std::ifstream(argv[2]).good(),"queries and fresh owned trace");const char*in=argv[1],*out=argv[2];
#else
 need(argc==4&&!std::ifstream(argv[3]).good(),"private image queries fresh trace");const char*in=argv[2],*out=argv[3];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0x100000);imageBase=image.base;
 std::array<uintptr_t,203>apiMethods{};apiMethods[202]=reinterpret_cast<uintptr_t>(&resetNative);apiMethods[6]=reinterpret_cast<uintptr_t>(&beginNative);apiMethods[100]=reinterpret_cast<uintptr_t>(&api100Native);apiMethods[93]=reinterpret_cast<uintptr_t>(&useNative);
 std::array<uintptr_t,8>shaderMethods{};shaderMethods[0]=reinterpret_cast<uintptr_t>(&materialCallbackNative);shaderMethods[7]=reinterpret_cast<uintptr_t>(&selectNative);shaderMethods[6]=reinterpret_cast<uintptr_t>(&drawNative);
 std::array<uintptr_t,27>paramMethods{};paramMethods[26]=reinterpret_cast<uintptr_t>(&flagsNative);
 std::array<uintptr_t,136>materialMethods{};materialMethods[135]=reinterpret_cast<uintptr_t>(&modeNative);
 for(auto&a:apis)a.table=reinterpret_cast<uintptr_t>(apiMethods.data());for(auto&sh:shaders)sh.table=reinterpret_cast<uintptr_t>(shaderMethods.data());for(auto&pa:parameters)pa.table=reinterpret_cast<uintptr_t>(paramMethods.data());material.table=reinterpret_cast<uintptr_t>(materialMethods.data());
 auto patch=[&](size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(image.base+at,&w,4);};
 patch(0x1017f8,reinterpret_cast<uintptr_t>(systemStorage.data()));patch(0x951e4,reinterpret_cast<uintptr_t>(&missingNative));patch(0xfc7b0,reinterpret_cast<uintptr_t>(&material));patch(0x95148,reinterpret_cast<uintptr_t>(&pushNative));patch(0x9514c,reinterpret_cast<uintptr_t>(&listenerNative));patch(0x95150,reinterpret_cast<uintptr_t>(&policyNative));patch(0x95144,reinterpret_cast<uintptr_t>(&popNative));
 for(size_t i=0;i<12;++i)image.base[0xfc800+i]=0;image.seal();
#endif
 apis[0].id=1;apis[1].id=2;shaders[0].id=1;shaders[1].id=2;std::ifstream input(in,std::ios::binary);std::ofstream output(out,std::ios::binary);need(bool(input)&&bool(output),"query trace IO");uint32_t cases=0,eventCount=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&effectiveIndex()<512&&q.available<2&&q.contextMode<4&&q.mutations<256&&q.materialMutations<32&&q.present<2&&q.forward<2&&uint8_t(q.mode)==0,"query domain");events.clear();draws=callbacks=missing=0;cookie=q.cookie;
#ifdef _WIN64
  owner={};shaderStates[0]=&owner.embedded;owner.draw.shaderState=reinterpret_cast<uintptr_t>(&owner.embedded);
#else
  materialStorage.fill(0);shaderStates[0]=reinterpret_cast<ShaderState*>(materialStorage.data()+0x30);materialWord(0,0x10095d74);std::memcpy(imageBase+0x1059f8,&cookie,4);
#endif
  shaderStates[1]=&externalState;*shaderStates[1]={};
  for(uint32_t i=0;i<2;++i){parameters[i].parameter.flags=q.flags^(i?0x1000u:0u);parameters[i].value=(i?0x22220000u:0x11110000u)+q.serial;
#ifdef _WIN64
  lists[i]={&parameters[i].parameter,nullptr,&parameters[i].parameter};
#else
  lists[i]={&parameters[i],nullptr,&parameters[i]};
#endif
  }
  liveShader(q.present?reinterpret_cast<uintptr_t>(&shaders[0]):0);liveList(list(1));encoded(q.token);shaderStates[0]->pad[0]=q.serial;
  const uint32_t anticipatedToken=q.token^q.cookie^((q.materialMutations&4)?(0xf1e2d3c4u^0x12345678u):0u);
  for(uint32_t i=0;i<4;++i)contexts[i]={0x12340000u+i,(q.contextMode&1)?anticipatedToken:anticipatedToken^0xabcddcbau,uint8_t((q.contextMode&2)?0x77:0)};
  for(uint32_t a=0;a<2;++a)for(uint32_t i=0;i<512;++i){auto&r=arrays[a][i];r={};r.available=1;r.word=0xbeef0000u+a*1000u+i;r.context=(i%5==4)?nullptr:&contexts[(i+a)%4];r.tail={a*0x10000u+i,1,2,3};}
  for(auto&a:arrays)a[effectiveIndex()].available=q.available;
  shaderStates[0]->array=arrays[0].data();shaderStates[1]->array=arrays[1].data();systemStorage.fill(0);state={};current(reinterpret_cast<uintptr_t>(shaderStates[1]));indexByte(0xee);flagByte(0xa7);apiValue(reinterpret_cast<uintptr_t>(&apis[0]));
  // Equal positions make the diagnostic stream empty, but nonzero sentinels
  // require its real clear/terminate path to execute. External buffer flag8
  // prevents allocation; no diagnostic messages or heap services are supplied.
  diagnosticWord(0x2c,0x23);diagnosticWord(0x3c,0x23);diagnosticWord(0x30,9);diagnosticWord(0x40,0x80000000);systemStorage[0x34]=0x71;systemStorage[0x35]=8;
#ifdef _WIN64
  MaterialOps materialOps;studioMaterialDrawCandidate(owner.draw,q.compression,q.alpha,q.mode,materialOps);
#else
  put(0,0x1009c9dc);std::array<uintptr_t,2>forward{0x10095a9c,reinterpret_cast<uintptr_t>(materialStorage.data())};
  void*target=q.forward?static_cast<void*>(forward.data()):static_cast<void*>(materialStorage.data());auto*table=*reinterpret_cast<uintptr_t**>(target);
  using Call=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);reinterpret_cast<Call>(table[71])(target,q.compression,q.alpha,q.mode);
#endif
  const bool outer=q.present&&!(q.flags&4u);const bool normal=outer&&q.available;
  if(normal)need(diagnosticWord(0x2c)==0&&diagnosticWord(0x30)==0&&systemStorage[0x34]==0&&diagnosticWord(0x40)==0&&diagnosticWord(0x3c)==0&&systemStorage[0x35]==8,"diagnostic sentinels cleared without allocation");
  else need(diagnosticWord(0x2c)==0x23&&diagnosticWord(0x30)==9&&diagnosticWord(0x3c)==0x23&&diagnosticWord(0x40)==0x80000000&&systemStorage[0x34]==0x71,"missing-snapshot path leaves diagnostics untouched");
  need(draws==uint32_t(normal)&&callbacks==uint32_t(outer)&&missing==uint32_t(!q.present),"independent composed material and snapshot gates");++checks;
  std::array<uint32_t,2>digest{2166136261u,2166136261u};for(uint32_t a=0;a<2;++a)for(const auto&r:arrays[a]){digest[a]=(digest[a]^contextId(r.context))*16777619u;digest[a]=(digest[a]^r.tail[0])*16777619u;}
  const uint32_t header[28]{cases,draws,parameterFor(liveList()).parameter.flags,apiId(api),stateId(current()),indexByte(),flagByte(),contexts[0].key,contexts[0].dirty,contexts[1].key,contexts[1].dirty,contexts[2].key,contexts[2].dirty,contexts[3].key,contexts[3].dirty,uint32_t(events.size()),uint32_t(shaderStates[0]->array==arrays[1].data()),uint32_t(shaderStates[1]->array==arrays[1].data()),digest[0],digest[1],callbacks,missing,shaderId(liveShader()),listId(liveList()),cookie,encoded(),shaderStates[0]->pad[0],parameters[0].value};
  output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(e.words.data()),sizeof(e.words));eventCount+=uint32_t(events.size());++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"query framing");output.flush();need(bool(output),"complete trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS material_shader_composition_cases="<<cases<<" events="<<eventCount<<" independent_checks="<<checks<<" debug_fallback=NOT_RUN diagnostic_buffer=EMPTY";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
