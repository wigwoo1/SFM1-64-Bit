#define NOMINMAX
#include <windows.h>
#include "physics_tree_traversal_test_fixture.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "native_vphysics_library_fixture.h"

static void(__thiscall*originalTree)(void*,void*)=nullptr;
static void(__thiscall*originalLeaf)(void*,void*)=nullptr;
static std::ofstream*captures=nullptr;static uint32_t active=0;static size_t captured=0;
static std::vector<const uint8_t*>nodes;
static std::map<const uint8_t*,uint32_t>indices;
static std::vector<SourcePhyCollisionTreeNodeCandidate>tree;
struct Event {uint32_t node;SourcePhysicsTreeStateCandidate before,after;};
static std::vector<Event>events;static bool walking=false;
template<class T>static T readNative(const uint8_t*p,size_t at){T value;std::memcpy(&value,p+at,sizeof(value));return value;}
static SourcePhysicsTreeStateCandidate stateNative(void*self){const auto*p=static_cast<const uint8_t*>(self);SourcePhysicsTreeStateCandidate s;std::memcpy(s.midpoint.data(),p+0xa0,12);std::memcpy(s.start.data(),p+0xb0,12);std::memcpy(s.direction.data(),p+0xc0,12);std::memcpy(&s.length,p+0xe0,4);std::memcpy(&s.boxRadius,p+0x7c,4);return s;}
static void __fastcall observeLeaf(void*self,void*,void*node){
    if(!walking)throw std::runtime_error("leaf outside observed traversal");const auto it=indices.find(static_cast<const uint8_t*>(node));if(it==indices.end()||tree[it->second].raw[0])throw std::runtime_error("unknown native leaf");Event e{it->second,stateNative(self),{}};originalLeaf(self,node);e.after=stateNative(self);
    if(std::memcmp(e.before.start.data(),e.after.start.data(),12)||std::memcmp(e.before.direction.data(),e.after.direction.data(),12)||std::memcmp(&e.before.boxRadius,&e.after.boxRadius,4))throw std::runtime_error("leaf changed fixed tree state");events.push_back(e);
}
static void __fastcall observeTree(void*self,void*,void*root){
    if(walking||nodes.empty()||root!=nodes[0])throw std::runtime_error("unexpected native traversal root");const auto initial=stateNative(self);events.clear();walking=true;originalTree(self,root);walking=false;const auto finalState=stateNative(self);
    supportPut(*captures,active);treeWriteState(*captures,initial);supportPut(*captures,uint32_t(events.size()));for(const auto&e:events){supportPut(*captures,e.node);treeWriteState(*captures,e.before);treeWriteState(*captures,e.after);}treeWriteState(*captures,finalState);++captured;
    for(size_t i=0;i<nodes.size();++i)if(std::memcmp(nodes[i],tree[i].raw.data(),28))throw std::runtime_error("original changed collision tree bytes");
}
static void captureTree(NativeVphysicsLibraryFixture&image,void*shape,std::ostream&out){
    using Create=void*(__thiscall*)(void*,void*);using Destroy=void(__thiscall*)(void*,void*);using Compact=const uint8_t*(__thiscall*)(void*);
    void*query=reinterpret_cast<Create>(image.table[43])(image.object,shape);if(!query)throw std::runtime_error("tree query missing");const auto*q=static_cast<const uint8_t*>(query);const uint32_t count=readNative<uint32_t>(q,8);const auto ledges=readNative<const uint8_t*const*>(q,12);if(!count||count>1000000||!ledges)throw std::runtime_error("tree ledge count");std::map<const uint8_t*,uint32_t>ordinals;for(uint32_t i=0;i<count;++i)if(!ordinals.emplace(ledges[i],i).second)throw std::runtime_error("duplicate native ledge");
    const auto vt=*reinterpret_cast<const uintptr_t*const*>(shape);if(image.rva(vt[12])!=0x184f0)throw std::runtime_error("compact getter changed");const auto compact=reinterpret_cast<Compact>(vt[12])(shape);if(!compact)throw std::runtime_error("compact missing");std::vector<const uint8_t*>pending{compact+readNative<int32_t>(compact,32)};
    while(!pending.empty()){const auto*p=pending.back();pending.pop_back();if(nodes.size()>=1000000||!indices.emplace(p,uint32_t(nodes.size())).second)throw std::runtime_error("native tree repeated/large");nodes.push_back(p);SourcePhyCollisionTreeNodeCandidate n;std::memcpy(n.raw.data(),p,28);const auto right=readNative<int32_t>(p,0);if(right){pending.push_back(p+right);pending.push_back(p+28);}else{const auto it=ordinals.find(p+readNative<int32_t>(p,4));if(it==ordinals.end())throw std::runtime_error("native tree ledge missing");n.ledge=it->second;}tree.push_back(n);}
    for(size_t i=0;i<nodes.size();++i)if(tree[i].raw[0]){tree[i].left=indices.at(nodes[i]+28);tree[i].right=indices.at(nodes[i]+readNative<int32_t>(nodes[i],0));}sourcePhysicsValidateTreeCandidate(tree);treeWrite(out,tree);reinterpret_cast<Destroy>(image.table[44])(image.object,query);
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"simplex capture fault query="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
struct Query {std::array<float,3>start{},delta{},origin{},angles{};uint32_t mask=0;};static_assert(sizeof(Query)==52,"query wire");
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"original physics observer x86");if(argc!=7)throw std::runtime_error("DLL directory PHY queries traces captures tree required");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);originalTree=reinterpret_cast<decltype(originalTree)>(image.address(0x2ad00));originalLeaf=reinterpret_cast<decltype(originalLeaf)>(image.address(0x2a340));image.observeCall(0x2b473,0x2ad00,reinterpret_cast<void*>(&observeTree));image.observeCall(0x2ade6,0x2a340,reinterpret_cast<void*>(&observeLeaf));image.observeCall(0x2b1fa,0x2a340,reinterpret_cast<void*>(&observeLeaf));
    std::ifstream phy(argv[2],std::ios::binary);if(!phy)throw std::runtime_error("capture PHY read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(phy)),{});if(b.size()<32)throw std::runtime_error("capture PHY header");uint32_t header,count;std::memcpy(&header,b.data(),4);std::memcpy(&count,b.data()+8,4);if(header<16||header>=b.size()||count!=1||b.size()-header>INT32_MAX)throw std::runtime_error("capture needs one original solid");const auto before=b;
    alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);reinterpret_cast<Load>(image.table[36])(image.object,vc.data()+8,1,b.data()+header,int(b.size()-header),false);void**shapes=nullptr;std::memcpy(&shapes,vc.data()+12,4);if(!shapes||!shapes[0])throw std::runtime_error("capture collision shape");
    std::ofstream treeOutput(argv[6],std::ios::binary);if(!treeOutput)throw std::runtime_error("tree output open");captureTree(image,shapes[0],treeOutput);treeOutput.flush();if(!treeOutput)throw std::runtime_error("tree output write");
    std::ifstream queries(argv[3],std::ios::binary);std::ofstream traces(argv[4],std::ios::binary),records(argv[5],std::ios::binary);if(!queries||!traces||!records)throw std::runtime_error("capture streams");captures=&records;Query q;
    while(queries.read(reinterpret_cast<char*>(&q),sizeof(q))){const auto old=q;for(const auto&v:{q.start,q.delta,q.origin,q.angles})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite capture query");alignas(16)std::array<uint8_t,80>ray{};std::memcpy(ray.data(),q.start.data(),12);std::memcpy(ray.data()+16,q.delta.data(),12);const float xy=q.delta[0]*q.delta[0]+q.delta[1]*q.delta[1],squared=xy+q.delta[2]*q.delta[2];ray[68]=1;ray[69]=squared!=0;const auto beforeRay=ray;alignas(16)std::array<uint8_t,112>result;result.fill(0xa7);
        reinterpret_cast<Trace>(image.table[31])(image.object,ray.data(),q.mask,nullptr,shapes[0],q.origin.data(),q.angles.data(),result.data()+16);if(ray!=beforeRay||std::memcmp(&q,&old,sizeof(q)))throw std::runtime_error("capture input mutation");for(size_t i=0;i<16;++i)if(result[i]!=0xa7)throw std::runtime_error("capture leading canary");for(size_t i=100;i<result.size();++i)if(result[i]!=0xa7)throw std::runtime_error("capture trailing canary");traces.write(reinterpret_cast<const char*>(result.data()+16),60);++active;
    }if(!queries.eof()||queries.gcount()!=0||!active)throw std::runtime_error("capture query extent");traces.flush();records.flush();if(!traces||!records)throw std::runtime_error("capture output write");reinterpret_cast<Unload>(image.table[37])(image.object,vc.data()+8);for(size_t i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("capture vcollide canary");if(b!=before)throw std::runtime_error("capture PHY bytes changed");image.verify();std::cout<<"PASS original physics queries="<<active<<" complete tree traversals="<<captured<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
