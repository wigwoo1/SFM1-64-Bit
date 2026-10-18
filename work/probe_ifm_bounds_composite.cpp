// Test-only full original renderable bounds oracle. Matrix and animated
// bounds helpers execute original code; only documented external inputs stubbed.
#ifndef NATIVE_IFM_BOUNDS_COMPOSITE
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_transform_hierarchy.h"
#else
#define DAG_WORLD_FIXTURE_NO_GRAPH
#endif
#include "dag_world_fixture.h"
#include "ifm_bounds_geometry_services_candidate.h"
#if defined(IFM_LIGHTING_COMPOSITE) && !defined(NATIVE_IFM_BOUNDS_COMPOSITE)
#include "session_ifm_lighting_origin_candidate.h"
#endif
#include <fstream>
#include <iostream>
#include <iterator>
struct CompositeBoundsFixture {
    uint32_t flags=0,root=0;int32_t tick=0;IfmRenderableBoundsStateCandidate state{};
    IfmBoundsCandidate engineBounds{};IfmStudioBoundsCandidate studio;
    IfmBoundsWrapperFramesCandidate frames;RecoveredMatrix3x4 modelWorld{};IfmFirstBonePoseCandidate firstBone;
    std::vector<DagWorldFixtureNode>nodes;std::vector<RecoveredMatrix3x4>palette;std::vector<IfmHitboxCandidate>boxes;
#ifdef IFM_LIGHTING_COMPOSITE
    // present, explicit node / FFFFFFFF null / FFFFFFFE non-DAG,
    // raw child-prefix kind(0 none,1 null,2 non-DAG), header2 present, index.
    std::array<uint32_t,5>origin{};
#endif
};
#ifdef NATIVE_IFM_BOUNDS_COMPOSITE
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
struct NativeDagAttribute {std::array<uint8_t,32>bytes{};uint32_t value=0;};
struct NativeDagNode {std::array<uint8_t,0x120>dag{};std::array<uint8_t,128>transform{};std::array<NativeDagAttribute,4>attributes{};};
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
static uint32_t fixtureFlags=0,fixtureRoot=0;
static std::array<uint8_t,128>firstBone{};
static std::array<uint32_t,9>events{};
static std::vector<RecoveredMatrix3x4>paletteInput;
static RecoveredMatrix3x4 modelWorldInput{};
static int32_t tickInput=0;
static uint32_t studioAddress=0;
#ifdef IFM_LIGHTING_COMPOSITE
static NativeDagAttribute illuminationAttribute;
static std::array<uint32_t,5>originInput{};
static std::array<uint32_t,2>nonDagObject{};
static std::array<uint32_t,4>originEvents{}; // selected,world,bounds,header
static uint8_t*nativeImageBase=nullptr;
#endif
static bool __fastcall typeHook(void*self,void*,uint32_t token){
    if(self==firstBone.data()&&token==0x369c)return (fixtureFlags&1024)!=0;
    for(size_t i=0;i<nativeNodes.size();++i)if(nativeNodes[i].dag.data()==self)return token==dagToken||(i==fixtureRoot&&token==0x2468&&(fixtureFlags&4));
    return false;
}
static void* __fastcall resolveHook(void*,void*,uint32_t handle){
#ifdef IFM_LIGHTING_COMPOSITE
    if(handle==0x30000)return nonDagObject.data();
#endif
    ++resolves;if(handle==0x20000){++events[8];return (fixtureFlags&512)?firstBone.data():nullptr;}
    if(handle>=1&&handle<=nativeNodes.size())return nativeNodes[handle-1].dag.data();
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
#ifdef IFM_LIGHTING_COMPOSITE
    if(self==nativeNodes[fixtureRoot].dag.data()&&!std::strcmp(name,"illumPositionDag"))return originInput[0]?illuminationAttribute.bytes.data():nullptr;
#endif
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

static void __fastcall synchronize(void*,void*){++events[0];}
static void __fastcall loadStudio(void*object,void*){++events[1];if(fixtureFlags&32)std::memcpy(static_cast<uint8_t*>(object)+0x70,&studioAddress,4);}
static bool __fastcall studioReady(void*,void*){return (fixtureFlags&64)!=0;}
static void* __fastcall getModel(void*,void*){return reinterpret_cast<void*>(0x11);}
static int32_t categoryInput=0;
static IfmBoundsCandidate engineBoundsInput{};
static int32_t __fastcall modelCategory(void*,void*,void*tag){if(tag!=reinterpret_cast<void*>(0x11))throw std::runtime_error("composite model tag");return categoryInput;}
static void __fastcall engineBounds(void*,void*,void*tag,float*lo,float*hi){if(tag!=reinterpret_cast<void*>(0x11))throw std::runtime_error("composite engine bounds tag");++events[2];std::memcpy(lo,engineBoundsInput[0].data(),12);std::memcpy(hi,engineBoundsInput[1].data(),12);}
static bool __fastcall setupBones(void*,void*,void*out,int maximum,int mask,float time){++events[3];events[4]=uint32_t(maximum);events[5]=uint32_t(mask);std::memcpy(&events[6],&time,4);if(maximum!=256||mask!=0x100)throw std::runtime_error("composite setup request");std::memcpy(out,paletteInput.data(),paletteInput.size()*48);return (fixtureFlags&256)!=0;}
static void* __fastcall modelWorld(void*,void*){++events[7];return modelWorldInput.data();}
static int32_t* __fastcall clockTick(void*,void*,int32_t*out){*out=tickInput;return out;}
#ifdef IFM_LIGHTING_COMPOSITE
// Counting bridges call the unmodified original routines, never supplied points
// or bounds. Only the draw-fragment callsites/vtable slot route through these.
static void __fastcall originWorldBridge(void*self,void*,float*out){
    ++originEvents[1];originEvents[0]=UINT32_MAX;
    for(uint32_t i=0;i<nativeNodes.size();++i)if(self==nativeNodes[i].dag.data())originEvents[0]=i;
    if(originEvents[0]==UINT32_MAX)throw std::runtime_error("unknown origin DAG");
    using World=void(__thiscall*)(void*,float*);reinterpret_cast<World>(nativeImageBase+0x4c5900)(self,out);
}
static void __fastcall originBoundsBridge(void*self,void*,float*lo,float*hi){++originEvents[2];using Bounds=void(__thiscall*)(void*,float*,float*);reinterpret_cast<Bounds>(nativeImageBase+0x240520)(self,lo,hi);}
static void* __fastcall originHeaderBridge(void*self,void*){++originEvents[3];using Header=void*(__thiscall*)(void*);return reinterpret_cast<Header>(nativeImageBase+0x23bf80)(self);}
#include "native_ifm_origin_frame_fixture.h"
#endif

#else
struct OwnedCompositeProviders {
    const CompositeBoundsFixture&f;GraphTransformHierarchy&hierarchy;bool cached;std::array<uint32_t,9>events{};
    void synchronize(){++events[0];}
    const IfmStudioBoundsCandidate*studio(){if(!cached){++events[1];cached=(f.flags&32)!=0;}return cached?&f.studio:nullptr;}
    IfmBoundsCandidate modelBounds(){++events[2];return f.engineBounds;}
    std::optional<IfmFirstBonePoseCandidate>firstBonePose(){++events[8];if((f.flags&(512|1024))==(512|1024))return f.firstBone;return {};}
    const std::vector<IfmHitboxCandidate>*animatedHitboxes(){if(!cached){++events[1];cached=(f.flags&32)!=0;}return cached?&f.boxes:nullptr;}
    bool hasClock(){return (f.flags&128)!=0;}
    int32_t clockTick(){return f.tick;}
    bool setupBones(std::array<RecoveredMatrix3x4,256>&palette,uint32_t maximum,uint32_t mask,float time){++events[3];events[4]=maximum;events[5]=mask;std::memcpy(&events[6],&time,4);std::copy(f.palette.begin(),f.palette.end(),palette.begin());return (f.flags&256)!=0;}
    RecoveredMatrix3x4 modelWorld(){++events[7];return f.modelWorld;}
    float localScale(){const auto&n=f.nodes.at(f.root);return f.state.rawModelResolved&&f.state.gameModelTyped&&(n.flags&32)&&(n.flags&64)?n.scale:1.0f;}
    std::optional<uint32_t>resolveDag(){return f.state.rawModelResolved?std::optional<uint32_t>(f.root):std::nullopt;}
    int32_t childCount(uint32_t id){int32_t count=0;for(const auto&n:f.nodes)count+=n.parent==id;return count;}
    std::optional<uint32_t>typedChildZero(uint32_t id){for(uint32_t i=0;i<f.nodes.size();++i)if(f.nodes[i].parent==id)return i;return {};}
    bool overrideParentEnabled(uint32_t id){const auto&n=f.nodes.at(id);return ifmBoundsChildOverrideCandidate((n.flags&1)!=0,n.overrideParent!=UINT32_MAX);}
    RecoveredMatrix3x4 world(uint32_t id){return hierarchy.world(id);}
    RecoveredMatrix3x4 local(uint32_t id){const auto&n=f.nodes.at(id);return (n.flags&32)?graphLocalTransformMatrix(hierarchy.graph,uint32_t(f.nodes.size())+id,true):GraphTransformHierarchy::identity();}
    const IfmBoundsWrapperFramesCandidate&wrapperFrames(){return f.frames;}
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("ifm.dll fixtures.bin output.bin required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("composite fixture IO");
#ifdef NATIVE_IFM_BOUNDS_COMPOSITE
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto*base=image.base;
    redirect(base,0x35dd50,reinterpret_cast<void*>(&lookupHook));redirect(base,0xa0ef50,reinterpret_cast<void*>(&std::memset));redirect(base,0x23bfb0,reinterpret_cast<void*>(&synchronize));redirect(base,0x23b810,reinterpret_cast<void*>(&loadStudio));redirect(base,0x5dac50,reinterpret_cast<void*>(&studioReady));redirect(base,0x29a950,reinterpret_cast<void*>(&clockTick));
    std::array<uintptr_t,117>dmTable{};dmTable[18]=reinterpret_cast<uintptr_t>(&resolveHook);dmTable[89]=reinterpret_cast<uintptr_t>(&attributeHook);dmTable[115]=reinterpret_cast<uintptr_t>(&firstHook);dmTable[116]=reinterpret_cast<uintptr_t>(&nextHook);
    const uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmTable.data()),dmAddress=reinterpret_cast<uintptr_t>(&dmObject);std::memcpy(base+0xe9a51c,&dmAddress,4);
    const uint32_t gameToken=0x2468,boneToken=0x369c;std::memcpy(base+0xeb131c,&dagToken,4);std::memcpy(base+0xeb52d4,&gameToken,4);std::memcpy(base+0xeb1914,&boneToken,4);std::memcpy(base+0xf5eac4,&childrenToken,4);base[0xf5eac8]|=1;
    std::array<uintptr_t,5>dagTable{};dagTable[4]=reinterpret_cast<uintptr_t>(&typeHook);
    std::array<uintptr_t,33>renderTable{};renderTable[8]=reinterpret_cast<uintptr_t>(&getModel);renderTable[13]=reinterpret_cast<uintptr_t>(&setupBones);renderTable[32]=reinterpret_cast<uintptr_t>(&modelWorld);
#ifdef IFM_LIGHTING_COMPOSITE
    nativeImageBase=base;renderTable[17]=reinterpret_cast<uintptr_t>(&originBoundsBridge);
    auto callBridge=[&](size_t at,void*target){if(base[at]!=0xe8)throw std::runtime_error("origin bridge call opcode");redirect(base,at,target);base[at]=0xe8;};
    callBridge(0x2439ac,reinterpret_cast<void*>(&originWorldBridge));callBridge(0x2439bc,reinterpret_cast<void*>(&originHeaderBridge));base[0x243a57]=0xc3;
#endif
    std::array<uintptr_t,10>engineTable{};engineTable[9]=reinterpret_cast<uintptr_t>(&modelCategory);engineTable[7]=reinterpret_cast<uintptr_t>(&engineBounds);const uintptr_t engineObject=reinterpret_cast<uintptr_t>(engineTable.data()),engineAddress=reinterpret_cast<uintptr_t>(&engineObject);std::memcpy(base+0xfd43a8,&engineAddress,4);
    std::array<uint8_t,0x34>modeObject{};const uint32_t modeAddress=reinterpret_cast<uint32_t>(modeObject.data());std::memcpy(base+0xf31774,&modeAddress,4);
    std::array<uint8_t,0x348>clockService{};const uint32_t clockAddress=reinterpret_cast<uint32_t>(clockService.data());std::memcpy(base+0xf32568,&clockAddress,4);image.seal();
#ifndef IFM_LIGHTING_COMPOSITE
    using Bounds=void(__thiscall*)(void*,float*,float*);
#endif
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    size_t cases=0;std::array<uint32_t,11>header{};
    while(in.read(reinterpret_cast<char*>(header.data()),44)){
        CompositeBoundsFixture f;f.flags=header[0];f.root=header[5];if((f.flags&~2047u)||header[4]>255||!header[6]||header[6]>64||f.root>=header[6]||header[7]>4096||header[8]>256||header[9]>256)throw std::runtime_error("composite fixture range");
        std::memcpy(&f.state.category,&header[1],4);std::memcpy(&f.state.sequence,&header[2],4);std::memcpy(&f.state.runtimeMode,&header[3],4);std::memcpy(&f.tick,&header[10],4);f.state.renderableFlags=uint8_t(header[4]);f.state.modelPresent=(f.flags&1)!=0;f.state.rawModelResolved=(f.flags&2)!=0;f.state.gameModelTyped=(f.flags&4)!=0;f.state.computeBounds=(f.flags&8)!=0;f.studio.ready=(f.flags&64)!=0;
#ifdef IFM_LIGHTING_COMPOSITE
        in.read(reinterpret_cast<char*>(f.origin.data()),20);
        if(!in||f.origin[0]>1||f.origin[2]>2||f.origin[3]>1||(f.origin[1]!=UINT32_MAX&&f.origin[1]!=UINT32_MAX-1&&f.origin[1]>=header[6]))throw std::runtime_error("origin fixture selection range");
        if(!f.origin[0]&&!(f.flags&(16|32)))throw std::runtime_error("origin fixture has no acquirable header");
        if(!f.origin[0]&&f.origin[2]&&f.state.runtimeMode!=0)throw std::runtime_error("origin fixture invalid child0 on bounds path");
#endif
        std::array<float,49>values{};in.read(reinterpret_cast<char*>(values.data()),196);f.nodes.resize(header[6]);f.studio.sequences.resize(header[7]);f.palette.resize(header[8]);f.boxes.resize(header[9]);
        in.read(reinterpret_cast<char*>(f.nodes.data()),std::streamsize(f.nodes.size()*44));in.read(reinterpret_cast<char*>(f.studio.sequences.data()),std::streamsize(f.studio.sequences.size()*24));in.read(reinterpret_cast<char*>(f.palette.data()),std::streamsize(f.palette.size()*48));in.read(reinterpret_cast<char*>(f.boxes.data()),std::streamsize(f.boxes.size()*28));if(!in)throw std::runtime_error("truncated composite fixture");validateDagWorldFixture(f.nodes);
        for(const auto&box:f.boxes)if(box.bone<0||uint32_t(box.bone)>=f.palette.size())throw std::runtime_error("composite palette span");if(f.studio.sequences.empty())throw std::runtime_error("composite fixture needs sequence zero");
        std::memcpy(f.engineBounds.data(),values.data(),24);std::memcpy(f.studio.hull.data(),values.data()+6,24);std::memcpy(f.studio.view.data(),values.data()+12,24);std::memcpy(&f.frames,values.data()+18,48);std::memcpy(f.modelWorld.data(),values.data()+30,48);std::memcpy(f.firstBone.position.data(),values.data()+42,12);std::memcpy(f.firstBone.orientation.data(),values.data()+45,16);
        for(const auto&n:f.nodes)f.state.childCount+=n.parent==f.root;
#ifdef IFM_LIGHTING_COMPOSITE
        if(f.origin[2])++f.state.childCount;
#endif
#ifdef NATIVE_IFM_BOUNDS_COMPOSITE
        activeCase=cases;fixtureFlags=f.flags;fixtureRoot=f.root;categoryInput=f.state.category;engineBoundsInput=f.engineBounds;paletteInput=f.palette;modelWorldInput=f.modelWorld;tickInput=f.tick;inputs=&f.nodes;
        nativeNodes=std::vector<NativeDagNode>(f.nodes.size());references=std::vector<ReverseDagReference>(f.nodes.size()*2);std::vector<std::vector<uint32_t>>children(f.nodes.size());for(uint32_t i=0;i<f.nodes.size();++i)if(f.nodes[i].parent!=UINT32_MAX)children[f.nodes[i].parent].push_back(i+1);
#ifdef IFM_LIGHTING_COMPOSITE
        originInput=f.origin;nonDagObject={reinterpret_cast<uint32_t>(dagTable.data()),0};
        const uint32_t illuminationHandle=f.origin[1]==UINT32_MAX?UINT32_MAX:f.origin[1]==UINT32_MAX-1?0x30000:f.origin[1]+1;
        initializeAttribute(illuminationAttribute,1,nativeNodes[f.root].dag.data(),0,illuminationHandle);
        if(f.origin[2])children[f.root].insert(children[f.root].begin(),f.origin[2]==1?UINT32_MAX:0x30000);
#endif
        for(uint32_t i=0;i<f.nodes.size();++i){auto&n=nativeNodes[i];const auto&s=f.nodes[i];put(n.dag,0,reinterpret_cast<uint32_t>(dagTable.data()));put(n.dag,0x1c,i+1);put(n.dag,0x38,i+1);n.dag[0x64]=(s.flags&1)!=0;put(n.dag,0x6c,(s.flags&32)?0x10000+i:UINT32_MAX);put(n.dag,0x84,reinterpret_cast<uint32_t>(children[i].data()));put(n.dag,0x90,uint32_t(children[i].size()));std::memcpy(n.transform.data()+0x5c,s.position.data(),12);std::memcpy(n.transform.data()+0x6c,s.orientation.data(),16);
            initializeAttribute(n.attributes[0],4,n.dag.data(),0,(s.flags&4)!=0);initializeAttribute(n.attributes[1],4,n.dag.data(),0,(s.flags&16)!=0);initializeAttribute(n.attributes[2],1,n.dag.data(),0,s.overrideParent==UINT32_MAX?UINT32_MAX:s.overrideParent+1);uint32_t scale;std::memcpy(&scale,&s.scale,4);initializeAttribute(n.attributes[3],3,n.transform.data(),0,scale);
            initializeAttribute(references[2*i].attribute,15,nullptr,childrenToken,0);references[2*i].next=s.parent==UINT32_MAX?0:2*i+2;initializeAttribute(references[2*i+1].attribute,15,s.parent==UINT32_MAX?nullptr:nativeNodes[s.parent].dag.data(),childrenToken,0);
        }
        nativeNodes[f.root].dag[0xfc]=f.state.computeBounds?1:0;const uint32_t firstBoneHandle=0x20000;put(nativeNodes[f.root].dag,0x10c,reinterpret_cast<uint32_t>(&firstBoneHandle));firstBone={};put(firstBone,0,reinterpret_cast<uint32_t>(dagTable.data()));std::memcpy(firstBone.data()+0x5c,f.firstBone.position.data(),12);std::memcpy(firstBone.data()+0x6c,f.firstBone.orientation.data(),16);
        // Separate classic sequence and first-hitbox tables in one controlled header.
        std::vector<uint8_t>studio(0x1000+f.studio.sequences.size()*212+f.boxes.size()*68);auto putStudio=[&](size_t at,uint32_t value){std::memcpy(studio.data()+at,&value,4);};std::memcpy(studio.data()+0x68,f.studio.view.data(),24);std::memcpy(studio.data()+0x80,f.studio.hull.data(),24);putStudio(0xbc,uint32_t(f.studio.sequences.size()));putStudio(0xc0,0x1000);for(size_t i=0;i<f.studio.sequences.size();++i)std::memcpy(studio.data()+0x1000+i*212+32,f.studio.sequences[i].data(),24);
        const size_t boxesAt=0x1000+f.studio.sequences.size()*212;putStudio(0xb0,0x200);putStudio(0x204,uint32_t(f.boxes.size()));putStudio(0x208,uint32_t(boxesAt-0x200));for(size_t i=0;i<f.boxes.size();++i){const size_t at=boxesAt+i*68;putStudio(at,uint32_t(f.boxes[i].bone));std::memcpy(studio.data()+at+8,f.boxes[i].bounds.data(),24);}std::array<uint32_t,2>studioWrapper{reinterpret_cast<uint32_t>(studio.data()),0};studioAddress=reinterpret_cast<uint32_t>(studioWrapper.data());
#ifdef IFM_LIGHTING_COMPOSITE
        if(f.origin[3]){putStudio(0x190,0x300);putStudio(0x308,f.origin[4]);}
#endif
        std::array<uint8_t,0xa0>initialObject{};put(initialObject,4,reinterpret_cast<uint32_t>(renderTable.data()));put(initialObject,0x2c,f.state.rawModelResolved?f.root+1:UINT32_MAX);put(initialObject,0x34,f.state.modelPresent?0x11:0);std::memcpy(initialObject.data()+0x38,&f.frames,48);put(initialObject,0x6c,header[2]);if(f.flags&16)put(initialObject,0x70,studioAddress);initialObject[0x80]=f.state.renderableFlags;put(modeObject,0x30,header[3]);put(clockService,0x344,(f.flags&128)?1:0);
        const auto beforeNodes=nativeNodes;const auto beforeReferences=references;const auto beforeChildren=children;const auto beforeStudio=studio;const auto beforeBone=firstBone;const auto beforePalette=paletteInput;
        const auto beforeWorld=modelWorldInput;const auto beforeMode=modeObject;const auto beforeClock=clockService;const auto beforeStudioWrapper=studioWrapper;const auto beforeEngineBounds=engineBoundsInput;
#ifdef IFM_LIGHTING_COMPOSITE
        const auto beforeIllumination=illuminationAttribute;const auto beforeNonDag=nonDagObject;
#endif
#else
        auto graph=dagWorldFixtureGraph(f.nodes);
#ifdef IFM_LIGHTING_COMPOSITE
        const uint32_t externalNonDag=uint32_t(graph.elements.size()),modelType=uint32_t(graph.strings.size());graph.strings.push_back("DmeGameModel");graph.strings.push_back("DmElement");graph.strings.push_back("illumPositionDag");graph.elements[f.root].type=modelType;
        graph.elements.push_back(Element{modelType+1,2,{},{}});
        if(f.origin[0]){Value v;v.reference=f.origin[1]==UINT32_MAX-1?externalNonDag:f.origin[1];graph.elements[f.root].attributes.push_back(Attribute{modelType+2,1,{v}});}
        if(f.origin[2]){auto&attrs=graph.elements[f.root].attributes;Attribute*rawChildren=nullptr;for(auto&a:attrs)if(graph.strings.at(a.name)=="children")rawChildren=&a;if(!rawChildren){attrs.push_back(Attribute{4,15,{}});rawChildren=&attrs.back();}Value v;v.reference=f.origin[2]==1?UINT32_MAX:externalNonDag;rawChildren->values.insert(rawChildren->values.begin(),v);}
#endif
        GraphTransformHierarchy hierarchy(graph);
#endif
        for(unsigned wrapper=0;wrapper<
#ifdef IFM_LIGHTING_COMPOSITE
            1
#else
            2
#endif
            ;++wrapper){
#ifdef NATIVE_IFM_BOUNDS_COMPOSITE
            auto object=initialObject;
#else
            OwnedCompositeProviders providers{f,hierarchy,(f.flags&16)!=0};IfmBoundsGeometryServicesCandidate<OwnedCompositeProviders>services{providers,f.state.runtimeMode};
#endif
            for(unsigned repeat=0;repeat<2;++repeat){std::array<uint32_t,9>counts{};
#ifdef IFM_LIGHTING_COMPOSITE
                std::array<float,3>point{123.25f,-456.5f,789.75f};uint32_t has=0;std::array<uint32_t,4>selection{UINT32_MAX,0,0,0};
#else
                IfmBoundsCandidate bounds{};
#endif
#ifdef NATIVE_IFM_BOUNDS_COMPOSITE
                events={};const auto objectBefore=object;std::array<uint32_t,8>guarded{};constexpr uint32_t canary=0xa7c5932e;guarded.front()=guarded.back()=canary;
#ifdef IFM_LIGHTING_COMPOSITE
                originEvents={UINT32_MAX,0,0,0};guarded[4]=canary;std::memcpy(guarded.data()+1,point.data(),12);
                invokeOrigin(base+0x243967,nativeNodes[f.root].dag.data(),object.data()+4,object.data(),reinterpret_cast<float*>(guarded.data()+1),&has);
                if(guarded.front()!=canary||guarded[4]!=canary||guarded.back()!=canary)throw std::runtime_error("origin output canary");std::memcpy(point.data(),guarded.data()+1,12);selection=originEvents;counts=events;
                auto expected=objectBefore;if(events[1]&&(f.flags&32))put(expected,0x70,studioAddress);
                if(std::memcmp(&illuminationAttribute,&beforeIllumination,sizeof(illuminationAttribute))||nonDagObject!=beforeNonDag)throw std::runtime_error("illumination input mutation");
#else
                reinterpret_cast<Bounds>(base+(wrapper?0x240520:0x23f310))(object.data()+(wrapper?4:0),reinterpret_cast<float*>(guarded.data()+1),reinterpret_cast<float*>(guarded.data()+4));if(guarded.front()!=canary||guarded.back()!=canary)throw std::runtime_error("composite output canary");std::memcpy(bounds.data(),guarded.data()+1,24);counts=events;auto expected=objectBefore;if(events[1]&&(f.flags&32))put(expected,0x70,studioAddress);
#endif
                if(object!=expected||children!=beforeChildren||studio!=beforeStudio||studioWrapper!=beforeStudioWrapper||firstBone!=beforeBone||modelWorldInput!=beforeWorld||engineBoundsInput!=beforeEngineBounds||modeObject!=beforeMode||clockService!=beforeClock||firstBoneHandle!=0x20000||std::memcmp(nativeNodes.data(),beforeNodes.data(),nativeNodes.size()*sizeof(NativeDagNode))||std::memcmp(references.data(),beforeReferences.data(),references.size()*sizeof(ReverseDagReference))||std::memcmp(paletteInput.data(),beforePalette.data(),paletteInput.size()*48))throw std::runtime_error("composite input/cache mutation");
#else
#ifdef IFM_LIGHTING_COMPOSITE
                providers.events={};
                const auto result=sessionIfmLightingOriginCandidate(graph,f.root,f.state,[&](uint32_t id){return id<f.nodes.size();},[&]()->std::optional<int32_t>{if(!providers.studio())return {};int32_t index=0;if(f.origin[3])std::memcpy(&index,&f.origin[4],4);return index;},[&](uint32_t id){return hierarchy.world(id);},services);
                if(result.point){has=1;point=*result.point;}selection={result.selectedDag.value_or(UINT32_MAX),result.selectedDag?1u:0u,result.boundsQueried?1u:0u,result.headerQueried?1u:0u};counts=providers.events;
#else
                providers.events={};bounds=wrapper?ifmRenderableBoundsWrapperCandidate(f.state,services):ifmRenderableBoundsCandidate(f.state,services);counts=providers.events;
#endif
#endif
#ifdef IFM_LIGHTING_COMPOSITE
                out.write(reinterpret_cast<const char*>(&has),4);out.write(reinterpret_cast<const char*>(point.data()),12);out.write(reinterpret_cast<const char*>(selection.data()),16);out.write(reinterpret_cast<const char*>(counts.data()),36);
#else
                out.write(reinterpret_cast<const char*>(bounds.data()),24);out.write(reinterpret_cast<const char*>(counts.data()),36);
#endif
            }
        }++cases;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("composite fixture length");out.flush();if(!out)throw std::runtime_error("composite fixture output");
#ifdef NATIVE_IFM_BOUNDS_COMPOSITE
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS complete bounds composition cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
