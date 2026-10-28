// Real original Bind->Flush->manager thunk->pending-dispatch chain, separately
// compared with owned x64 composition. Draw/finish/gate/material leaf services
// are explicit observers. No original instruction arrays, SFM or GPU access.
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
#include "shader_api_material_flush_candidate.h"
#include "pending_draw_dispatch_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Material{uintptr_t table;uint32_t id,value;};
struct Mesh{uintptr_t table;uint32_t id;};
struct Event{uint32_t kind,id,a,b;};
static std::array<Material,3>materials;
static std::array<Mesh,2>meshes;
static std::array<uint32_t,5>metadata{11,22,33,44,55};
static std::vector<Event>events;
static uint32_t mode=0,gateMode=0,mutations=0,draws=0,finishes=0,gates=0,checks=0,depth=0;
static ShaderApiMaterialBindingStateCandidate binding;
static ShaderApiMaterialFlushStateCandidate saved;
static PendingDrawDispatchCandidate pending;
static uintptr_t material(uint32_t id){return reinterpret_cast<uintptr_t>(&materials.at(id-1));}
static uint32_t materialId(uintptr_t p){for(const auto&m:materials)if(reinterpret_cast<uintptr_t>(&m)==p)return m.id;throw std::runtime_error("invalid material");}
static uintptr_t mesh(uint32_t id){return id?reinterpret_cast<uintptr_t>(&meshes.at(id-1)):0;}
static uint32_t meshId(uintptr_t p){if(!p)return 0;for(const auto&m:meshes)if(reinterpret_cast<uintptr_t>(&m)==p)return m.id;throw std::runtime_error("invalid mesh");}
#ifndef _WIN64
static std::array<uint8_t,0x2c24>owner;
static uint8_t*nativePending=nullptr;
static uintptr_t ownerIdentity=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static uint32_t read(const uint8_t*p,size_t at){uint32_t w;std::memcpy(&w,p+at,4);return w;}
static void write(uint8_t*p,size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(p+at,&w,4);}
#endif
static uintptr_t current(){
#ifdef _WIN64
 return binding.material;
#else
 return read(owner.data(),0x2c20);
#endif
}
static uint32_t first(){
#ifdef _WIN64
 return saved.first;
#else
 return read(owner.data(),0xde4);
#endif
}
static uint32_t second(){
#ifdef _WIN64
 return saved.second;
#else
 return read(owner.data(),0xde8);
#endif
}
static void controls(uint32_t a,uint32_t b){
#ifdef _WIN64
 saved={a,b};
#else
 write(owner.data(),0xde4,a);write(owner.data(),0xde8,b);
#endif
}
static uintptr_t endpoint(){
#ifdef _WIN64
 return pending.mesh;
#else
 return read(nativePending,0x18);
#endif
}
static void endpoint(uintptr_t p){
#ifdef _WIN64
 pending.mesh=p;
#else
 write(nativePending,0x18,p);
#endif
}
static uint32_t flags(){
#ifdef _WIN64
 return uint32_t(pending.guard)|(uint32_t(pending.pending)<<8)|(uint32_t(pending.alternate)<<16);
#else
 return nativePending[0x30]|(uint32_t(nativePending[0x32])<<8)|(uint32_t(nativePending[0x33])<<16);
#endif
}
static void flags(uint8_t g,uint8_t p,uint8_t a){
#ifdef _WIN64
 pending.guard=g;pending.pending=p;pending.alternate=a;
#else
 nativePending[0x30]=g;nativePending[0x32]=p;nativePending[0x33]=a;
#endif
}
static void bind(uintptr_t);
static bool predicate(uintptr_t p){events.push_back({1,materialId(p),mode!=0,depth});return mode!=0;}
static uint32_t key(uintptr_t p){auto value=mode==1?42u:materialId(p);events.push_back({2,materialId(p),value,depth});return value;}
static bool allow(){++gates;events.push_back({3,materialId(current()),first(),second()});if(mutations&1u)controls(first()+1,second()^0xffu);return gateMode==2;}
static void draw(uintptr_t target,bool alternate,uintptr_t data){
 need(target==mesh(1)&&(flags()&255u)==1,"original draw endpoint and guard");
 if(alternate){
#ifdef _WIN64
  need(data==reinterpret_cast<uintptr_t>(metadata.data()),"owned metadata pointer");
#else
  need(data==reinterpret_cast<uintptr_t>(nativePending+0x1c),"original inline metadata pointer");
#endif
  need(std::memcmp(reinterpret_cast<void*>(data),metadata.data(),sizeof(metadata))==0,"alternate metadata preserved");
 }
 auto&m=materials.at(materialId(current())-1);events.push_back({alternate?5u:4u,meshId(target),m.id,m.value});events.push_back({6,flags(),first(),second()});++draws;need(draws==1&&m.id==1,"old binding still active at pending draw");checks+=2;m.value+=31;
 if(mutations&2u){endpoint(mesh(2));flags(7,8,9);}controls(0x11223344,0x55667788);
 if(mutations&4u){++depth;bind(material(3));--depth;need(draws==1,"pending guard suppresses nested draw");++checks;}
}
static void finish(uintptr_t target){need(target==endpoint()&&target==mesh(mutations&2u?2:1)&&flags()==0,"fresh endpoint and cleared flags at cleanup");auto&m=materials.at(materialId(current())-1);events.push_back({8,meshId(target),m.id,m.value});events.push_back({9,flags(),first(),second()});++finishes;++checks;if(mutations&8u)flags(5,17,19);}
#ifdef _WIN64
struct PendingOps{void draw(uintptr_t p,bool alt,uintptr_t data){::draw(p,alt,data);}void finish(uintptr_t p){::finish(p);}};
struct FlushOps{uintptr_t gate(){return gateMode?uintptr_t(1):0;}bool allowFlush(uintptr_t p){need(p==1,"gate identity");return allow();}void flushMeshManager(){PendingOps ops;pending.flush(ops);}};
struct BindOps{bool predicate(uintptr_t p){return ::predicate(p);}uint32_t key(uintptr_t p){return ::key(p);}void flush(){FlushOps ops;shaderApiMaterialFlushCandidate(saved,ops);}};
static void bind(uintptr_t p){BindOps ops;shaderApiMaterialBindingCandidate(binding,p,ops);}
#else
static bool __fastcall predicateNative(void*p,void*){return predicate(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall keyNative(void*p,void*){return key(reinterpret_cast<uintptr_t>(p));}
static bool __fastcall allowNative(void*,void*){return allow();}
static void __fastcall ordinaryNative(void*p,void*,int32_t start,int32_t count){need(start==-1&&count==0,"ordinary native arguments");draw(reinterpret_cast<uintptr_t>(p),false,0);}
static void __fastcall alternateNative(void*p,void*,void*data,int32_t start,int32_t count){need(start==-1&&count==0,"alternate native arguments");draw(reinterpret_cast<uintptr_t>(p),true,reinterpret_cast<uintptr_t>(data));}
static void __fastcall finishNative(void*p,void*,uint32_t a,uint32_t b){need(!a&&!b,"native cleanup arguments");finish(reinterpret_cast<uintptr_t>(p));}
static void bind(uintptr_t p){using Bind=void(__thiscall*)(void*,void*);reinterpret_cast<Bind>(0x10013f30)(owner.data(),reinterpret_cast<void*>(p));}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==2&&!std::ifstream(argv[1]).good(),"fresh owned trace");const char*outputPath=argv[1];
#else
 need(argc==3&&!std::ifstream(argv[2]).good(),"read-only shader image/fresh trace");const char*outputPath=argv[2];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0x100000);nativePending=image.base+0x17f7c8+0x290;write(image.base+0x17f7c8,0,0x100fbce4);image.seal();
 std::array<uintptr_t,11>materialMethods{};materialMethods[7]=reinterpret_cast<uintptr_t>(&predicateNative);materialMethods[10]=reinterpret_cast<uintptr_t>(&keyNative);std::array<uintptr_t,28>gateMethods{};gateMethods[27]=reinterpret_cast<uintptr_t>(&allowNative);uintptr_t gate=reinterpret_cast<uintptr_t>(gateMethods.data());std::array<uintptr_t,26>meshMethods{};meshMethods[12]=reinterpret_cast<uintptr_t>(&ordinaryNative);meshMethods[25]=reinterpret_cast<uintptr_t>(&alternateNative);meshMethods[22]=reinterpret_cast<uintptr_t>(&finishNative);ownerIdentity=reinterpret_cast<uintptr_t>(owner.data());need(ownerIdentity!=0,"reference owner");
#endif
 std::ofstream output(outputPath,std::ios::binary);need(bool(output),"trace output");uint32_t cases=0,eventCount=0,totalDraws=0,totalGates=0,reentries=0;
 for(uint32_t desired:{1u,2u})for(uint32_t special=0;special<3;++special)for(uint32_t gateKind=0;gateKind<3;++gateKind)for(uint32_t present=0;present<2;++present)for(uint32_t guard=0;guard<2;++guard)for(uint32_t todo:{0u,1u,255u})for(uint32_t alternate=0;alternate<2;++alternate)for(uint32_t mutation=0;mutation<16;++mutation){
  mode=special;gateMode=gateKind;mutations=mutation;draws=finishes=gates=depth=0;events.clear();for(uint32_t i=0;i<3;++i)materials[i]={0,i+1,0xaaa00001u+i};for(uint32_t i=0;i<2;++i)meshes[i]={0,i+1};
#ifdef _WIN64
  binding.material=material(1);pending.metadata=reinterpret_cast<uintptr_t>(metadata.data());
#else
  for(auto&m:materials)m.table=reinterpret_cast<uintptr_t>(materialMethods.data());for(auto&m:meshes)m.table=reinterpret_cast<uintptr_t>(meshMethods.data());owner.fill(0xa5);write(owner.data(),0,0x100fcedc);write(owner.data(),0x2c20,material(1));std::memcpy(nativePending+0x1c,metadata.data(),sizeof(metadata));write(image.base+0x183b58,0,gateKind?reinterpret_cast<uintptr_t>(&gate):0);
#endif
  controls(0x87654321,0xabcdef00);endpoint(present?mesh(1):0);flags(uint8_t(guard),uint8_t(todo),uint8_t(alternate));bind(material(desired));
  const bool triggered=desired==2&&special!=1;const bool performed=triggered&&gateKind!=1&&present&&!guard&&todo;need(draws==uint32_t(performed)&&finishes==draws,"independent whole-chain draw/finish eligibility");need(current()==material(triggered?desired:1),"outer bind assignment happens after flush");checks+=2;
  const uint32_t expectedFirst=0x87654321u+(triggered&&gateKind&&(mutation&1u)?1u:0u),expectedSecond=0xabcdef00u^(triggered&&gateKind&&(mutation&1u)?0xffu:0u);need(first()==expectedFirst&&second()==expectedSecond,"restore values saved after optional gate callback");++checks;
  const uint32_t expectedFlags=performed?(mutation&8u?5u|(17u<<8)|(19u<<16):0u):guard|(todo<<8)|(alternate<<16);need(flags()==expectedFlags&&meshId(endpoint())==(performed&&(mutation&2u)?2u:present),"callback replacement/reset/cleanup state preserved");++checks;
  const uint32_t header[11]{cases,materialId(current()),first(),second(),meshId(endpoint()),flags(),draws,finishes,gates,materials[0].value,uint32_t(events.size())};output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(&e),sizeof(e));++cases;eventCount+=uint32_t(events.size());totalDraws+=draws;totalGates+=gates;if(performed&&(mutation&4u))++reentries;
#ifndef _WIN64
  image.verify();
#endif
 }
 output.flush();need(bool(output),"complete composition trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image released");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS bind_flush_composition_cases="<<cases<<" events="<<eventCount<<" pending_draws="<<totalDraws<<" gate_calls="<<totalGates<<" reentrant_binds="<<reentries<<" independent_checks="<<checks<<" draw_observes_previous_material=PASS endpoint_replacement=PASS state_restore=PASS GPU=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
