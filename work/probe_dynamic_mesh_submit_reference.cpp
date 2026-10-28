// Whole original mesh submit + instance builder/default matrix transpose,
// compared with existing owned x64 adapters. Explicit interface observers;
// original RX instructions unchanged, no DllMain/import resolution/SFM/GPU.
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
#include "studio_dynamic_mesh_submit_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t index,supplied,mutations,pending,batch,count,formatLow,formatHigh,offsetA,offsetB;std::array<uint32_t,4>color,modulation;};
static_assert(sizeof(Query)==72,"explicit reference wire");
struct Event{uint32_t kind,id,data;};
struct Material{uintptr_t table;uint32_t id;std::array<uint32_t,4>color;};
static Query q;
static std::array<Material,2>materials;
static std::array<uint32_t,16>sourceMatrix;
static std::array<float,4>modulation;
static StudioShaderInstanceStateCandidate instanceState;
static std::vector<Event>events;
static std::array<uint32_t,49>record;
static uintptr_t meshIdentity=0,currentMaterial=0;
static uint32_t formatLow=0,publishedCount=0,checks=0,submitted=0;
#ifndef _WIN64
static std::array<uint8_t,0x3000>owner;
static uintptr_t ownerIdentity=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void word(size_t at,uintptr_t value){uint32_t v=uint32_t(value);std::memcpy(owner.data()+at,&v,4);}
static uint32_t word(size_t at){uint32_t v;std::memcpy(&v,owner.data()+at,4);return v;}
#endif
static void event(uint32_t kind,uint32_t id=0,uint32_t data=0){events.push_back({kind,id,data});}
static float floating(uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;}
static uint32_t bits(float v){uint32_t b;std::memcpy(&b,&v,4);return b;}
static Material&mat(uintptr_t p){for(auto&m:materials)if(reinterpret_cast<uintptr_t>(&m)==p)return m;throw std::runtime_error("invalid material");}
static const uint32_t*matrix(){event(1);return sourceMatrix.data();}
static void offsets(uintptr_t receiver,uint32_t&a,uint32_t&b){need(receiver==meshIdentity,"mesh offsets receiver");event(2);a=q.offsetA;b=q.offsetB;if(q.mutations&1u)for(auto&v:modulation)v=floating(bits(v)^0x80000000u);}
static uintptr_t material(){event(3);const auto result=currentMaterial;if(q.mutations&2u)modulation.fill(99.f);return result;}
static void rgb(uintptr_t receiver,std::array<float,3>&out){auto&m=mat(receiver);need(m.id==1,"captured RGB material");event(4,m.id);for(unsigned i=0;i<3;++i)out[i]=floating(m.color[i]);if(q.mutations&4u){m.color[3]^=0x80000000u;currentMaterial=reinterpret_cast<uintptr_t>(&materials[1]);}}
static float alpha(uintptr_t receiver){auto&m=mat(receiver);need(m.id==1,"alpha uses same material even after current binding changes");event(5,m.id);if(q.mutations&8u)formatLow^=0x400u;return floating(m.color[3]);}
static uint32_t count(uintptr_t receiver){need(receiver==meshIdentity,"mesh count receiver");event(6);if(q.mutations&16u)for(unsigned i=0;i<3;++i)materials[0].color[i]^=0x80000000u;return q.count;}
static std::array<uint32_t,2>format(uintptr_t receiver){need(receiver==meshIdentity,"mesh format receiver");event(8);return {formatLow,q.formatHigh};}
static void observe(uintptr_t receiver,uint32_t n,const std::array<uint32_t,19>&scalars,uintptr_t matrices,uintptr_t lighting,bool compressed,uintptr_t metadata1,uintptr_t metadata2){
 need(receiver==meshIdentity&&n==1,"synchronous single instance submission");event(9);++submitted;need(submitted==1,"one submit");
#ifdef _WIN64
 need(matrices==reinterpret_cast<uintptr_t>(instanceState.currentMatrix.data())&&lighting==instanceState.lighting&&metadata1==instanceState.metadata1&&metadata2==instanceState.metadata2,"full-width retained instance pointers");const auto remaining=instanceState.pendingMatrices;
#else
 need(matrices==ownerIdentity+0x3f0&&lighting==ownerIdentity+0x13dc&&metadata1==ownerIdentity+0x11b8&&metadata2==ownerIdentity+0x11b6,"original interface-relative retained pointers");const auto remaining=word(0xde0);publishedCount=word(0x1d38);
#endif
 checks+=2;const auto expectedCompression=((q.formatLow^((q.mutations&8u)?0x400u:0u))&0x400u)!=0;need(compressed==expectedCompression&&publishedCount==q.count&&scalars[11]==q.offsetA&&scalars[12]==q.offsetB,"later live format/count/offset transport");++checks;
 for(unsigned i=0;i<4;++i){auto color=q.color[i];if(i==3&&(q.mutations&4u))color^=0x80000000u;uint32_t expected=color;if(q.supplied){auto factor=q.modulation[i]^((q.mutations&1u)?0x80000000u:0u);expected=bits(_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(floating(color)),_mm_set_ss(floating(factor)))));}need(scalars[15+i]==expected,"captured modulation and material RGBA timing");++checks;}
 record.fill(0);record[0]=q.index;record[1]=remaining;record[2]=publishedCount;record[3]=compressed;record[4]=mat(currentMaterial).id;for(unsigned i=0;i<4;++i){record[5+i]=materials[0].color[i];record[9+i]=bits(modulation[i]);}std::memcpy(record.data()+13,scalars.data(),sizeof(scalars));std::memcpy(record.data()+32,reinterpret_cast<const void*>(matrices),48);for(unsigned i=0;i<4;++i)record[44+i]=i+1;
}
#ifdef _WIN64
struct Ops{
 StudioShaderInstanceResultCandidate instance(){return studioShaderInstanceStateCandidate(instanceState,[](auto&converted){auto p=matrix();std::array<uint32_t,16>src;std::memcpy(src.data(),p,sizeof(src));converted=studioShaderInstanceMatrixCandidate(src);});}
 void offsets(uintptr_t m,uint32_t&a,uint32_t&b){::offsets(m,a,b);}uintptr_t material(){return ::material();}void rgb(uintptr_t m,std::array<float,3>&out){::rgb(m,out);}float alpha(uintptr_t m){return ::alpha(m);}uint32_t count(uintptr_t m){return ::count(m);}void setCount(uint32_t v){publishedCount=v;}std::array<uint32_t,2>format(uintptr_t m){return ::format(m);}
 void submit(uintptr_t m,uint32_t n,const StudioShaderInstanceCandidate&s,bool compressed,uintptr_t a,uintptr_t b){need(s.scalars[4]==0&&s.scalars[6]==0,"owned reserved scalar pointer slots");observe(m,n,s.scalars,s.matrices,s.lighting,compressed,a,b);}
};
#else
// The matrix service uses COM-style stack-this (__stdcall), unlike the
// surrounding Source __thiscall virtuals: native code PUSHes its receiver.
static const uint32_t*__stdcall matrixNative(void*){return matrix();}
static void __fastcall offsetsNative(void*p,void*,uint32_t*a,uint32_t*b){offsets(reinterpret_cast<uintptr_t>(p),*a,*b);}
static uintptr_t __fastcall materialNative(void*,void*){return material();}
static void __fastcall rgbNative(void*p,void*,float*r,float*g,float*b){std::array<float,3>out;rgb(reinterpret_cast<uintptr_t>(p),out);*r=out[0];*g=out[1];*b=out[2];}
static float __fastcall alphaNative(void*p,void*){return alpha(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall countNative(void*p,void*){return count(reinterpret_cast<uintptr_t>(p));}
static uint64_t __fastcall formatNative(void*p,void*){auto value=format(reinterpret_cast<uintptr_t>(p));return value[0]|(uint64_t(value[1])<<32);}
static void __fastcall submitNative(void*,void*,void*m,uint32_t n,const uint32_t*instance,uint32_t compressed,void*a,void*b){std::array<uint32_t,19>scalars;std::memcpy(scalars.data(),instance,sizeof(scalars));const auto matrices=scalars[4],lighting=scalars[6];scalars[4]=scalars[6]=0;observe(reinterpret_cast<uintptr_t>(m),n,scalars,matrices,lighting,compressed!=0,reinterpret_cast<uintptr_t>(a),reinterpret_cast<uintptr_t>(b));}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==3&&!std::ifstream(argv[2]).good(),"query and fresh owned trace");const char*queryPath=argv[1],*outputPath=argv[2];
#else
 need(argc==4&&!std::ifstream(argv[3]).good(),"read-only original, query, fresh reference trace");const char*queryPath=argv[2],*outputPath=argv[3];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0x100000);
 // Pin only the mutable CPU-dispatch table entry to its actual default x87
 // implementation. No instruction is patched, initializer/import is executed,
 // or automatic CPU selection claimed by this controlled fixture.
 uint32_t transpose=0x10057cc0;std::memcpy(image.base+0x16f8c8,&transpose,4);image.seal();
 std::array<uintptr_t,278>apiMethods{};apiMethods[274]=0x1000ffb0;apiMethods[277]=0x1000ea60;apiMethods[260]=reinterpret_cast<uintptr_t>(&materialNative);apiMethods[253]=reinterpret_cast<uintptr_t>(&submitNative);
 std::array<uintptr_t,38>meshMethods{};meshMethods[37]=reinterpret_cast<uintptr_t>(&offsetsNative);meshMethods[32]=reinterpret_cast<uintptr_t>(&countNative);meshMethods[1]=reinterpret_cast<uintptr_t>(&formatNative);uintptr_t meshObject=reinterpret_cast<uintptr_t>(meshMethods.data());meshIdentity=reinterpret_cast<uintptr_t>(&meshObject);
 std::array<uintptr_t,47>materialMethods{};materialMethods[46]=reinterpret_cast<uintptr_t>(&rgbNative);materialMethods[44]=reinterpret_cast<uintptr_t>(&alphaNative);std::array<uintptr_t,18>matrixMethods{};matrixMethods[17]=reinterpret_cast<uintptr_t>(&matrixNative);uintptr_t matrixObject=reinterpret_cast<uintptr_t>(matrixMethods.data());ownerIdentity=reinterpret_cast<uintptr_t>(owner.data());uint32_t api=uint32_t(ownerIdentity);std::memcpy(image.base+0x16dd08,&api,4);
#endif
 std::ifstream input(queryPath,std::ios::binary);std::ofstream output(outputPath,std::ios::binary);need(bool(input)&&bool(output),"query/trace IO");uint32_t cases=0,eventsTotal=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.index==cases&&q.supplied<=1&&q.batch<=1&&q.mutations<32,"query domain");events.clear();submitted=0;for(unsigned i=0;i<4;++i)modulation[i]=floating(q.modulation[i]);formatLow=q.formatLow;materials[0]={0,1,q.color};materials[1]={0,2,{0x3f800000,0x3f800000,0x3f800000,0x3f800000}};currentMaterial=reinterpret_cast<uintptr_t>(&materials[0]);for(unsigned i=0;i<16;++i)sourceMatrix[i]=0x3f000000u+((q.index*31u+i*17u)&0x7fffffu);instanceState={};instanceState.pendingMatrices=q.pending;instanceState.matrixBatchEnabled=q.batch!=0;for(unsigned i=0;i<12;++i)instanceState.currentMatrix[i]=q.index*31u+i;publishedCount=0xcccccccc;
#ifdef _WIN64
  meshIdentity=reinterpret_cast<uintptr_t>(&instanceState);instanceState.lighting=reinterpret_cast<uintptr_t>(&materials);instanceState.metadata1=reinterpret_cast<uintptr_t>(&q.color);instanceState.metadata2=reinterpret_cast<uintptr_t>(&q.modulation);Ops ops;studioDynamicMeshSubmitCandidate(meshIdentity,q.supplied?&modulation:nullptr,ops);
#else
  owner.fill(0);word(0,reinterpret_cast<uintptr_t>(apiMethods.data()));word(0x3ec,reinterpret_cast<uintptr_t>(&matrixObject));word(0xde0,q.pending);word(0x170c,q.batch);std::memcpy(owner.data()+0x3f0,instanceState.currentMatrix.data(),48);for(auto&m:materials)m.table=reinterpret_cast<uintptr_t>(materialMethods.data());using Submit=void(__thiscall*)(void*,const float*);reinterpret_cast<Submit>(0x100093f0)(reinterpret_cast<void*>(meshIdentity),q.supplied?modulation.data():nullptr);image.verify();
#endif
  const bool batch=q.pending&&q.batch;need(record[1]==(batch?0u:q.pending)&&record[15]==(batch?q.pending+1u:1u),"whole instance builder pending-count mutation");++checks;std::vector<uint32_t>order;for(auto&e:events)order.push_back(e.kind);need(order==(batch?std::vector<uint32_t>{2,3,4,5,6,8,9}:std::vector<uint32_t>{1,2,3,4,5,6,8,9}),"whole callback order");++checks;record[48]=uint32_t(events.size());output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));for(const auto&e:events)output.write(reinterpret_cast<const char*>(&e),sizeof(e));eventsTotal+=uint32_t(events.size());++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"complete query framing");output.flush();need(bool(output),"complete output");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS dynamic_mesh_submit_cases="<<cases<<" events="<<eventsTotal<<" independent_checks="<<checks<<" captured_modulation=PASS same_material_alpha=PASS late_format=PASS shader_draw=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX cpu_matrix_path=PINNED_DEFAULT\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
