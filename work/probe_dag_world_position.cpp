#ifndef NATIVE_DAG_WORLD_POSITION
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_transform_hierarchy.h"
#else
#define DAG_WORLD_FIXTURE_NO_GRAPH
#include "transform_matrix_composition.h"
#include <fstream>
#include <iostream>
#include <iterator>
#endif
#include "dag_world_fixture.h"
#ifdef NATIVE_DAG_WORLD_POSITION
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
struct NativeDagAttribute {std::array<uint8_t,32>bytes{};uint32_t value=0;};
struct NativeDagNode {std::array<uint8_t,128>dag{},transform{};std::array<NativeDagAttribute,4>attributes{};};
struct ReverseDagReference {NativeDagAttribute attribute;uint32_t next=0;};
static std::vector<NativeDagNode> nativeNodes;
static std::vector<ReverseDagReference> references;
static const std::vector<DagWorldFixtureNode>* inputs=nullptr;
static uint64_t lookups=0,resolves=0,firsts=0,nexts=0,attributeReads=0;
static constexpr uint32_t childrenToken=0x5151,dagToken=0x1357;
static void put(std::array<uint8_t,128>&target,size_t offset,uint32_t v){std::memcpy(target.data()+offset,&v,4);}
static void initializeAttribute(NativeDagAttribute&a,uint32_t type,void*owner,uint32_t name,uint32_t value){
    a.value=value;const uint32_t address=reinterpret_cast<uint32_t>(&a.value),own=reinterpret_cast<uint32_t>(owner);
    std::memcpy(a.bytes.data()+4,&address,4);std::memcpy(a.bytes.data()+8,&own,4);
    std::memcpy(a.bytes.data()+0x10,&type,4);std::memcpy(a.bytes.data()+0x14,&name,4);
}
static bool __fastcall typeHook(void*self,void*,uint32_t token){
    for(const auto&n:nativeNodes)if(n.dag.data()==self)return token==dagToken;
    return false;
}
static void* __fastcall resolveHook(void*,void*,uint32_t handle){
    ++resolves;if(handle>=1&&handle<=nativeNodes.size())return nativeNodes[handle-1].dag.data();
    if(handle>=0x10000&&handle-0x10000<nativeNodes.size())return nativeNodes[handle-0x10000].transform.data();
    if(handle==UINT32_MAX)return nullptr;throw std::runtime_error("unexpected native handle");
}
static uint32_t __fastcall firstHook(void*,void*,uint32_t handle){
    ++firsts;if(handle<1||handle>nativeNodes.size())throw std::runtime_error("first reference handle");return 2*(handle-1)+1;
}
static uint32_t __fastcall nextHook(void*,void*,uint32_t handle){
    ++nexts;if(handle<1||handle>references.size())throw std::runtime_error("next reference handle");return references[handle-1].next;
}
static void* __fastcall attributeHook(void*,void*,uint32_t handle){
    ++attributeReads;if(handle<1||handle>references.size())throw std::runtime_error("attribute handle");return references[handle-1].attribute.bytes.data();
}
static void* __fastcall lookupHook(void*self,void*,const char*name){
    ++lookups;for(size_t i=0;i<nativeNodes.size();++i){auto&n=nativeNodes[i];const auto flags=inputs->at(i).flags;
        if(self==n.dag.data()){
            if(!std::strcmp(name,"overridePos"))return (flags&2)?n.attributes[0].bytes.data():nullptr;
            if(!std::strcmp(name,"overrideRot"))return (flags&8)?n.attributes[1].bytes.data():nullptr;
            if(!std::strcmp(name,"overrideParent"))return n.attributes[2].bytes.data();
        }else if(self==n.transform.data()&&!std::strcmp(name,"scale"))return (flags&64)?n.attributes[3].bytes.data():nullptr;
    }throw std::runtime_error("unexpected native attribute lookup");
}
static void redirect(uint8_t*base,size_t offset,void*target){auto*p=base+offset;p[0]=0xe9;const auto relative=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&relative,4);}
static size_t activeCase=0;static uint32_t activeNode=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){
    // Let ordinary C++ validation exceptions reach the enclosing catch block.
    if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;
    std::cerr<<"native DAG fault case="<<activeCase<<" node="<<activeNode<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);
}
static uint32_t nodeIndex(void*p){if(!p)return UINT32_MAX;for(uint32_t i=0;i<nativeNodes.size();++i)if(nativeNodes[i].dag.data()==p)return i;throw std::runtime_error("unknown selected DAG");}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("ifm.dll fixtures.bin result.bin required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace fixture IO");
#ifdef NATIVE_DAG_WORLD_POSITION
    static_assert(sizeof(void*)==4,"x86 oracle required");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto*base=image.base;
    redirect(base,0x35dd50,reinterpret_cast<void*>(&lookupHook));
    redirect(base,0xa0ef50,reinterpret_cast<void*>(&std::memset));
    std::array<uintptr_t,117>dmVtable{};dmVtable[18]=reinterpret_cast<uintptr_t>(&resolveHook);dmVtable[89]=reinterpret_cast<uintptr_t>(&attributeHook);dmVtable[115]=reinterpret_cast<uintptr_t>(&firstHook);dmVtable[116]=reinterpret_cast<uintptr_t>(&nextHook);
    const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmVtable.data());const uintptr_t dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);
    std::memcpy(base+0xeb131c,&dagToken,4);std::memcpy(base+0xf5eac4,&childrenToken,4);base[0xf5eac8]|=1;
    std::array<uintptr_t,5>dagVtable{};dagVtable[4]=reinterpret_cast<uintptr_t>(&typeHook);image.seal();
    using World=void(__thiscall*)(void*,float*);using Parent=void*(__thiscall*)(void*);
#else
    static_assert(sizeof(void*)==8,"owned probe must be AMD64");
#endif
    size_t cases=0,totalNodes=0;uint32_t count=0;
    while(in.read(reinterpret_cast<char*>(&count),4)){
        if(!count||count>64)throw std::runtime_error("fixture node count");std::vector<DagWorldFixtureNode>nodes(count);if(!in.read(reinterpret_cast<char*>(nodes.data()),std::streamsize(count*sizeof(nodes[0]))))throw std::runtime_error("truncated fixture nodes");
        // This test-only wire validator does not compute any native answer.
        validateDagWorldFixture(nodes);
#ifndef NATIVE_DAG_WORLD_POSITION
        const auto graph=dagWorldFixtureGraph(nodes);GraphTransformHierarchy hierarchy(graph);
#endif
#ifdef NATIVE_DAG_WORLD_POSITION
        activeCase=cases;inputs=&nodes;nativeNodes=std::vector<NativeDagNode>(count);references=std::vector<ReverseDagReference>(count*2);
        for(uint32_t i=0;i<count;++i){auto&n=nativeNodes[i];const auto&s=nodes[i];put(n.dag,0,reinterpret_cast<uint32_t>(dagVtable.data()));put(n.dag,0x1c,i+1);put(n.dag,0x38,i+1);n.dag[0x64]=(s.flags&1)!=0;put(n.dag,0x6c,(s.flags&32)?0x10000+i:UINT32_MAX);std::memcpy(n.transform.data()+0x5c,s.position.data(),12);std::memcpy(n.transform.data()+0x6c,s.orientation.data(),16);
            initializeAttribute(n.attributes[0],4,n.dag.data(),0,(s.flags&4)!=0);initializeAttribute(n.attributes[1],4,n.dag.data(),0,(s.flags&16)!=0);initializeAttribute(n.attributes[2],1,n.dag.data(),0,s.overrideParent==UINT32_MAX?UINT32_MAX:s.overrideParent+1);uint32_t scale;std::memcpy(&scale,&s.scale,4);initializeAttribute(n.attributes[3],3,n.transform.data(),0,scale);
            // Deliberately enumerate a non-owner reference before the unique parent.
            // This exercises the original rejection/next-reference loop.
            initializeAttribute(references[2*i].attribute,15,nullptr,childrenToken,0);references[2*i].next=s.parent==UINT32_MAX?0:2*i+2;
            initializeAttribute(references[2*i+1].attribute,15,s.parent==UINT32_MAX?nullptr:nativeNodes[s.parent].dag.data(),childrenToken,0);
        }
        const auto beforeNodes=nativeNodes;const auto beforeReferences=references;
#endif
        for(uint32_t i=0;i<count;++i){std::array<uint32_t,2>parents{};RecoveredMatrix3x4 matrix{};std::array<float,3>point{};
#ifdef NATIVE_DAG_WORLD_POSITION
            activeNode=i;void*self=nativeNodes[i].dag.data();parents={nodeIndex(reinterpret_cast<Parent>(base+0x4c3ed0)(self)),nodeIndex(reinterpret_cast<Parent>(base+0x4c3f60)(self))};
            reinterpret_cast<World>(base+0x4c5870)(self,matrix[0].data());reinterpret_cast<World>(base+0x4c5900)(self,point.data());
#else
            parents={hierarchy.parent(i,true),hierarchy.parent(i,false)};matrix=hierarchy.world(i);point={matrix[0][3],matrix[1][3],matrix[2][3]};
#endif
            out.write(reinterpret_cast<const char*>(parents.data()),8);out.write(reinterpret_cast<const char*>(matrix.data()),48);out.write(reinterpret_cast<const char*>(point.data()),12);
        }
#ifdef NATIVE_DAG_WORLD_POSITION
        if(std::memcmp(nativeNodes.data(),beforeNodes.data(),nativeNodes.size()*sizeof(NativeDagNode))||std::memcmp(references.data(),beforeReferences.data(),references.size()*sizeof(ReverseDagReference)))throw std::runtime_error("native fixture data changed");
#endif
        ++cases;totalNodes+=count;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("fixture input length");out.flush();if(!out)throw std::runtime_error("fixture output failure");
#ifdef NATIVE_DAG_WORLD_POSITION
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);std::cout<<"original lookup="<<lookups<<" resolve="<<resolves<<" first="<<firsts<<" next="<<nexts<<" attribute="<<attributeReads<<'\n';
#endif
    std::cout<<"PASS DAG world cases="<<cases<<" nodes="<<totalNodes<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
