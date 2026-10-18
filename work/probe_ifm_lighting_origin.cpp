#include "ifm_model_lighting_origin_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#ifndef NATIVE_IFM_LIGHTING_ORIGIN
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_model_illumination_dag_candidate.h"
#else
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::array<std::array<float,3>,3>dagPoints{};
static std::array<uint8_t,3>dagKinds{};
static std::array<std::array<uint32_t,2>,3>dagObjects{};
static std::array<std::array<float,3>,2>callbackBounds{};
static std::array<uint32_t,8>attribute{};
static bool attributePresent=false;static void*modelAddress=nullptr;
static uint32_t selectedDag=UINT32_MAX,worldCalls=0,boundsCalls=0;
static void* __fastcall lookup(void*self,void*,const char*name){if(self!=modelAddress||std::strcmp(name,"illumPositionDag"))throw std::runtime_error("unexpected lookup");return attributePresent?attribute.data():nullptr;}
static void* __fastcall resolve(void*,void*,uint32_t handle){if(handle==UINT32_MAX)return nullptr;if(handle<1||handle>3)throw std::runtime_error("unexpected DAG handle");return dagObjects[handle-1].data();}
static bool __fastcall isDag(void*self,void*,uint32_t token){for(uint32_t i=0;i<3;++i)if(self==dagObjects[i].data())return token==0x1357&&dagKinds[i]==1;throw std::runtime_error("unknown type receiver");}
static void __fastcall worldPosition(void*self,void*,float*out){for(uint32_t i=0;i<3;++i)if(self==dagObjects[i].data()){++worldCalls;selectedDag=i;std::memcpy(out,dagPoints[i].data(),12);return;}throw std::runtime_error("unknown world receiver");}
static void __fastcall bounds(void*,void*,float*lo,float*hi){++boundsCalls;std::memcpy(lo,callbackBounds[0].data(),12);std::memcpy(hi,callbackBounds[1].data(),12);}
static void redirect(uint8_t*base,size_t offset,void*target){auto*p=base+offset;p[0]=0xe9;const auto displacement=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&displacement,4);}
// Supply precisely the local frame/register inputs used by this draw fragment.
// A test RET replaces the first following instruction at 10243A57; no rendering
// or unrelated draw-state setup executes.
__declspec(naked) static void invoke(void*,void*,void*,void*,float*,uint32_t*){__asm{
    push ebp
    mov ebp,esp
    sub esp,260h
    push ebx
    push esi
    push edi
    mov edi,dword ptr [ebp+0ch]
    mov esi,dword ptr [ebp+10h]
    mov eax,dword ptr [ebp+14h]
    mov dword ptr [ebp-218h],eax
    call dword ptr [ebp+8]
    mov eax,dword ptr [ebp+1ch]
    xor ecx,ecx
    test ebx,ebx
    setne cl
    mov dword ptr [eax],ecx
    test ebx,ebx
    jz noPoint
    mov eax,dword ptr [ebp+18h]
    mov ecx,dword ptr [ebx]
    mov dword ptr [eax],ecx
    mov ecx,dword ptr [ebx+4]
    mov dword ptr [eax+4],ecx
    mov ecx,dword ptr [ebx+8]
    mov dword ptr [eax+8],ecx
noPoint:
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret
}}
static size_t currentCase=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"IFM selection fault case="<<currentCase<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("ifm.dll queries.bin output.bin required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace fixture IO");
#ifdef NATIVE_IFM_LIGHTING_ORIGIN
    static_assert(sizeof(void*)==4,"x86 original-code probe");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto*base=image.base;
    redirect(base,0x35dd50,reinterpret_cast<void*>(&lookup));redirect(base,0x4c5900,reinterpret_cast<void*>(&worldPosition));base[0x243a57]=0xc3;
    std::array<uintptr_t,19>dmVtable{};dmVtable[18]=reinterpret_cast<uintptr_t>(&resolve);const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmVtable.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);const uint32_t token=0x1357;std::memcpy(base+0xeb131c,&token,4);
    std::array<uintptr_t,5>dagVtable{};dagVtable[4]=reinterpret_cast<uintptr_t>(&isDag);for(uint32_t i=0;i<3;++i)dagObjects[i]={reinterpret_cast<uint32_t>(dagVtable.data()),i};
    std::array<uintptr_t,18>renderVtable{};renderVtable[17]=reinterpret_cast<uintptr_t>(&bounds);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned AMD64 probe");
#endif
    // Nine u32 fields followed by explicit/child0/child1/lo/hi vectors (15 floats).
    std::array<uint32_t,24>q{};size_t count=0;
    while(in.read(reinterpret_cast<char*>(q.data()),96)){
        if(q[0]>1||q[1]>2||q[2]>2||q[3]>2||q[4]>2||q[5]>1||q[7]>255||q[8]>255)throw std::runtime_error("fixture enum/byte range");
        int32_t index;std::memcpy(&index,&q[6],4);std::array<std::array<float,3>,3>points{};std::array<std::array<float,3>,2>bb{};std::memcpy(points.data(),q.data()+9,36);std::memcpy(bb.data(),q.data()+18,24);
        uint32_t has=0,calls=0,boxCalls=0,dag=UINT32_MAX;std::array<float,3>point{123.25f,-456.5f,789.75f};
#ifdef NATIVE_IFM_LIGHTING_ORIGIN
        currentCase=count;attributePresent=q[0]!=0;dagKinds={uint8_t(q[1]),uint8_t(q[3]),uint8_t(q[4])};dagPoints=points;callbackBounds=bb;selectedDag=UINT32_MAX;worldCalls=0;boundsCalls=0;
        std::array<uint8_t,0x110>model{};modelAddress=model.data();std::array<uint32_t,2>children{q[3]?2u:UINT32_MAX,q[4]?3u:UINT32_MAX};auto put=[&](auto&target,size_t at,uint32_t value){std::memcpy(target.data()+at,&value,4);};put(model,0x84,reinterpret_cast<uint32_t>(children.data()));put(model,0x90,q[2]);model[0xfc]=uint8_t(q[7]);
        uint32_t ref=q[1]?1u:UINT32_MAX;attribute={};attribute[1]=reinterpret_cast<uint32_t>(&ref);attribute[4]=1;
        std::array<uint8_t,128>renderable{},context{};put(renderable,0,reinterpret_cast<uint32_t>(renderVtable.data()));renderable[0x7c]=uint8_t(q[8]);
        std::array<uint8_t,0x1b0>header{};if(q[5]){put(header,0x190,0x1a0);put(header,0x1a8,q[6]);}const uint32_t headerAddress=reinterpret_cast<uint32_t>(header.data());put(context,0x70,reinterpret_cast<uint32_t>(&headerAddress));
        const auto modelBefore=model;const auto renderBefore=renderable;const auto contextBefore=context;const auto headerBefore=header;const auto attributeBefore=attribute;
        invoke(base+0x243967,model.data(),renderable.data(),context.data(),point.data(),&has);calls=worldCalls;boxCalls=boundsCalls;dag=selectedDag;
        if(model!=modelBefore||renderable!=renderBefore||context!=contextBefore||header!=headerBefore||attribute!=attributeBefore)throw std::runtime_error("native selection input changed");
#else
        Graph graph;graph.strings={"DmeGameModel","DmeDag","DmElement","model","illumPositionDag","children"};graph.elements={{0,3,{},{}},{1,3,{},{}},{1,3,{},{}},{1,3,{},{}}};for(uint32_t i=0;i<3;++i)graph.elements[i+1].type=([&]{return i==0?q[1]:i==1?q[3]:q[4];}()==1)?1:2;
        auto ref=[](uint32_t kind,uint32_t value){Value v;v.reference=kind?value:UINT32_MAX;return v;};if(q[0])graph.elements[0].attributes.push_back(Attribute{4,1,{ref(q[1],1)}});Attribute children{5,15,{}};if(q[2])children.values.push_back(ref(q[3],2));if(q[2]>1)children.values.push_back(ref(q[4],3));graph.elements[0].attributes.push_back(children);
        const auto selection=sessionModelIlluminationDagCandidate(graph,0,[&](uint32_t id){return graph.elements.at(id).type==1;});
        const auto result=ifmModelLightingOriginCandidate(selection.attributePresent,selection.dag,q[7]!=0,uint8_t(q[8]),[&]{if(q[0])throw std::runtime_error("present attribute queried header");return q[5]?index:0;},[&](uint32_t id){++calls;dag=id-1;return points.at(id-1);},[&]{++boxCalls;return bb;});if(result){has=1;point=*result;}
#endif
        out.write(reinterpret_cast<const char*>(&has),4);out.write(reinterpret_cast<const char*>(point.data()),12);out.write(reinterpret_cast<const char*>(&calls),4);out.write(reinterpret_cast<const char*>(&boxCalls),4);out.write(reinterpret_cast<const char*>(&dag),4);++count;
    }
    if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("fixture input length");out.flush();if(!out)throw std::runtime_error("fixture output failure");
#ifdef NATIVE_IFM_LIGHTING_ORIGIN
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS IFM lighting-origin selection cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
