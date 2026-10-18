// Test-only original-code oracle. Datamodel/attributes/types are controlled;
// original child selector, recursive world/local transforms and wrapper execute.
#ifndef NATIVE_IFM_BOUNDS_FRAMES
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_transform_hierarchy.h"
#else
#define DAG_WORLD_FIXTURE_NO_GRAPH
#endif
#include "dag_world_fixture.h"
#include "ifm_bounds_frames_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_IFM_BOUNDS_FRAMES
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
struct NativeDagAttribute {std::array<uint8_t,32>bytes{};uint32_t value=0;};
struct NativeDagNode {std::array<uint8_t,160>dag{};std::array<uint8_t,128>transform{};std::array<NativeDagAttribute,4>attributes{};};
struct ReverseDagReference {NativeDagAttribute attribute;uint32_t next=0;};
static std::vector<NativeDagNode> nativeNodes;
static std::vector<ReverseDagReference> references;
static const std::vector<DagWorldFixtureNode>* inputs=nullptr;
static uint64_t lookups=0,resolves=0,firsts=0,nexts=0,attributeReads=0;
static constexpr uint32_t childrenToken=0x5151,dagToken=0x1357;
template<size_t N> static void put(std::array<uint8_t,N>&target,size_t offset,uint32_t v){std::memcpy(target.data()+offset,&v,4);}
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

#endif
#ifndef NATIVE_IFM_BOUNDS_FRAMES
struct OwnedFrameServices {
    const std::vector<DagWorldFixtureNode>&nodes;GraphTransformHierarchy&hierarchy;uint32_t root;bool resolved;
    std::optional<uint32_t>resolveDag(){return resolved?std::optional<uint32_t>(root):std::nullopt;}
    int32_t childCount(uint32_t id){int32_t count=0;for(const auto&n:nodes)count+=n.parent==id;return count;}
    std::optional<uint32_t>typedChildZero(uint32_t id){for(uint32_t i=0;i<nodes.size();++i)if(nodes[i].parent==id)return i;return {};}
    bool overrideParentEnabled(uint32_t id){const auto&n=nodes.at(id);return ifmBoundsChildOverrideCandidate((n.flags&1)!=0,n.overrideParent!=UINT32_MAX);}
    RecoveredMatrix3x4 world(uint32_t id){return hierarchy.world(id);}
    RecoveredMatrix3x4 local(uint32_t id){const auto&n=nodes.at(id);if(!(n.flags&32))return GraphTransformHierarchy::identity();return graphLocalTransformMatrix(hierarchy.graph,uint32_t(nodes.size())+id,true);}
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("ifm.dll fixtures.bin output.bin required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("frame fixture IO");
#ifdef NATIVE_IFM_BOUNDS_FRAMES
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto*base=image.base;
    redirect(base,0x35dd50,reinterpret_cast<void*>(&lookupHook));redirect(base,0xa0ef50,reinterpret_cast<void*>(&std::memset));
    std::array<uintptr_t,117>dmVtable{};dmVtable[18]=reinterpret_cast<uintptr_t>(&resolveHook);dmVtable[89]=reinterpret_cast<uintptr_t>(&attributeHook);dmVtable[115]=reinterpret_cast<uintptr_t>(&firstHook);dmVtable[116]=reinterpret_cast<uintptr_t>(&nextHook);
    const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmVtable.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);
    std::memcpy(base+0xeb131c,&dagToken,4);std::memcpy(base+0xf5eac4,&childrenToken,4);base[0xf5eac8]|=1;
    std::array<uintptr_t,5>dagVtable{};dagVtable[4]=reinterpret_cast<uintptr_t>(&typeHook);
    std::array<uint8_t,0x34>modeObject{};const uint32_t modeAddress=reinterpret_cast<uint32_t>(modeObject.data());std::memcpy(base+0xf31774,&modeAddress,4);image.seal();
    using Adjust=void(__thiscall*)(void*,float*,float*);using Wrapper=void(__thiscall*)(void*,float*);using Predicate=bool(__thiscall*)(void*);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    uint32_t count=0;size_t cases=0;
    while(in.read(reinterpret_cast<char*>(&count),4)){
        if(!count||count>64)throw std::runtime_error("frame node count");std::array<uint32_t,3>header{};std::array<float,18>values{};std::vector<DagWorldFixtureNode>nodes(count);
        in.read(reinterpret_cast<char*>(header.data()),12);in.read(reinterpret_cast<char*>(values.data()),72);in.read(reinterpret_cast<char*>(nodes.data()),std::streamsize(count*44));if(!in)throw std::runtime_error("truncated frame fixture");
        validateDagWorldFixture(nodes);if(header[0]>=count||header[2]>1)throw std::runtime_error("frame root/resolve flag");int32_t mode;std::memcpy(&mode,&header[1],4);
        IfmBoundsWrapperFramesCandidate frames;std::memcpy(frames.position38.data(),values.data(),12);std::memcpy(frames.angles44.data(),values.data()+3,12);std::memcpy(frames.position50.data(),values.data()+6,12);std::memcpy(frames.angles5c.data(),values.data()+9,12);
        IfmBoundsCandidate bounds;std::memcpy(bounds.data(),values.data()+12,24);uint32_t predicate=UINT32_MAX;
        std::optional<uint32_t>child;for(uint32_t i=0;i<count;++i)if(nodes[i].parent==header[0]){child=i;break;}
        RecoveredMatrix3x4 wrapper{};
#ifdef NATIVE_IFM_BOUNDS_FRAMES
        activeCase=cases;inputs=&nodes;nativeNodes=std::vector<NativeDagNode>(count);references=std::vector<ReverseDagReference>(count*2);std::vector<std::vector<uint32_t>>children(count);
        for(uint32_t i=0;i<count;++i)if(nodes[i].parent!=UINT32_MAX)children[nodes[i].parent].push_back(i+1);
        for(uint32_t i=0;i<count;++i){auto&n=nativeNodes[i];const auto&s=nodes[i];put(n.dag,0,reinterpret_cast<uint32_t>(dagVtable.data()));put(n.dag,0x1c,i+1);put(n.dag,0x38,i+1);n.dag[0x64]=(s.flags&1)!=0;put(n.dag,0x6c,(s.flags&32)?0x10000+i:UINT32_MAX);put(n.dag,0x84,reinterpret_cast<uint32_t>(children[i].data()));put(n.dag,0x90,uint32_t(children[i].size()));
            std::memcpy(n.transform.data()+0x5c,s.position.data(),12);std::memcpy(n.transform.data()+0x6c,s.orientation.data(),16);
            initializeAttribute(n.attributes[0],4,n.dag.data(),0,(s.flags&4)!=0);initializeAttribute(n.attributes[1],4,n.dag.data(),0,(s.flags&16)!=0);initializeAttribute(n.attributes[2],1,n.dag.data(),0,s.overrideParent==UINT32_MAX?UINT32_MAX:s.overrideParent+1);uint32_t scale;std::memcpy(&scale,&s.scale,4);initializeAttribute(n.attributes[3],3,n.transform.data(),0,scale);
            initializeAttribute(references[2*i].attribute,15,nullptr,childrenToken,0);references[2*i].next=s.parent==UINT32_MAX?0:2*i+2;
            initializeAttribute(references[2*i+1].attribute,15,s.parent==UINT32_MAX?nullptr:nativeNodes[s.parent].dag.data(),childrenToken,0);
        }
        std::array<uint8_t,0xa0>object{};put(object,0x2c,header[2]?header[0]+1:UINT32_MAX);std::memcpy(object.data()+0x38,values.data(),48);put(modeObject,0x30,header[1]);
        const auto beforeNodes=nativeNodes;const auto beforeReferences=references;const auto beforeObject=object;const auto beforeChildren=children;
        if(child)predicate=reinterpret_cast<Predicate>(base+0x4c3d50)(nativeNodes[*child].dag.data());
        std::array<uint32_t,8>guardedBounds{};std::array<uint32_t,14>guardedWrapper{};constexpr uint32_t canary=0x5a37c2e1;
        guardedBounds.front()=guardedBounds.back()=guardedWrapper.front()=guardedWrapper.back()=canary;std::memcpy(guardedBounds.data()+1,bounds.data(),24);
        reinterpret_cast<Adjust>(base+0x23bcd0)(object.data(),reinterpret_cast<float*>(guardedBounds.data()+1),reinterpret_cast<float*>(guardedBounds.data()+4));
        reinterpret_cast<Wrapper>(base+0x23b640)(object.data(),reinterpret_cast<float*>(guardedWrapper.data()+1));
        if(guardedBounds.front()!=canary||guardedBounds.back()!=canary||guardedWrapper.front()!=canary||guardedWrapper.back()!=canary)throw std::runtime_error("native frame output canary");
        std::memcpy(bounds.data(),guardedBounds.data()+1,24);std::memcpy(wrapper.data(),guardedWrapper.data()+1,48);
        if(object!=beforeObject||children!=beforeChildren||std::memcmp(nativeNodes.data(),beforeNodes.data(),count*sizeof(NativeDagNode))||std::memcmp(references.data(),beforeReferences.data(),references.size()*sizeof(ReverseDagReference)))throw std::runtime_error("original frame inputs mutated");
#else
        const auto graph=dagWorldFixtureGraph(nodes);GraphTransformHierarchy hierarchy(graph);OwnedFrameServices services{nodes,hierarchy,header[0],header[2]!=0};
        if(child)predicate=services.overrideParentEnabled(*child);
        bounds=ifmBoundsAdjustChildCandidate(mode,services,bounds);wrapper=ifmBoundsWrapperMatrixCandidate(frames);
#endif
        out.write(reinterpret_cast<const char*>(bounds.data()),24);out.write(reinterpret_cast<const char*>(wrapper.data()),48);out.write(reinterpret_cast<const char*>(&predicate),4);++cases;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("frame fixture length");out.flush();if(!out)throw std::runtime_error("frame output");
#ifdef NATIVE_IFM_BOUNDS_FRAMES
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS bounds child/wrapper frames cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
