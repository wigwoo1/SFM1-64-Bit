#include "material_dictionary_tree_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#ifdef SFM64_NATIVE_DICTIONARY_TREE
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <string>
#include "material_typed_dispatch_native_fixture.h"
struct DictionaryTreeNativeFixture:MaterialTypedDispatchNativeFixture{
    std::array<uint8_t,96>storage{};
    explicit DictionaryTreeNativeFixture(const char*p):MaterialTypedDispatchNativeFixture(p){storage.fill(0xa5);using Ctor=void*(__thiscall*)(void*);reinterpret_cast<Ctor>(0x10012910)(tree());}
    uint8_t*tree(){return storage.data()+16;}
    static uint16_t half(const void*p,size_t n){uint16_t v;std::memcpy(&v,static_cast<const uint8_t*>(p)+n,2);return v;}
    static uint32_t word(const void*p,size_t n){uint32_t v;std::memcpy(&v,static_cast<const uint8_t*>(p)+n,4);return v;}
    uint16_t first(){using F=uint16_t(__thiscall*)(void*);return reinterpret_cast<F>(0x100088b0)(tree());}
    uint16_t next(uint16_t i){using F=uint16_t(__thiscall*)(void*,uint16_t);return reinterpret_cast<F>(0x10008980)(tree(),i);}
    uint8_t*node(uint16_t i){return reinterpret_cast<uint8_t*>(word(tree(),4))+size_t(i)*16;}
    uint16_t execute(const std::array<uint32_t,4>&q){
        std::array<uint8_t,8>key{};std::memcpy(key.data(),&q[1],4);const auto sym=uint16_t(q[2]);std::memcpy(key.data()+4,&sym,2);key[6]=uint8_t(q[3]);
        if(q[0]==1){uint16_t parent=0xffff;bool direction=false;using Position=void(__thiscall*)(void*,void*,uint16_t*,bool*);reinterpret_cast<Position>(0x10007e20)(tree(),key.data(),&parent,&direction);
            using Allocate=uint16_t(__thiscall*)(void*);auto index=reinterpret_cast<Allocate>(0x10007f10)(tree());using Link=void(__thiscall*)(void*,uint16_t,uint16_t,bool);reinterpret_cast<Link>(0x10008c30)(tree(),index,parent,direction);auto count=uint16_t(half(tree(),0x12)+1);std::memcpy(tree()+0x12,&count,2);std::memcpy(node(index)+8,key.data(),7);return index;}
        if(q[0]==2){uint16_t found=0xffff;for(auto i=first();i!=0xffff;i=next(i))if(word(node(i),8)==q[1]){found=i;break;}using Remove=void(__thiscall*)(void*,void*);reinterpret_cast<Remove>(0x10009ac0)(tree(),reinterpret_cast<void*>(q[1]));return found;}
        if(q[0]==3){using Find=uint16_t(__thiscall*)(void*,void*);return reinterpret_cast<Find>(0x10010480)(tree(),key.data());}
        using Purge=void(__thiscall*)(void*);reinterpret_cast<Purge>(0x10012840)(tree());return 0xffff;
    }
    void finishTree(){using Purge=void(__thiscall*)(void*);reinterpret_cast<Purge>(0x10012840)(tree());reinterpret_cast<Purge>(0x100128b0)(tree()+0x1c);for(unsigned i=0;i<16;++i)if(storage[i]!=0xa5||storage[80+i]!=0xa5)throw std::runtime_error("Original tree storage guard");finish();}
};
static LONG CALLBACK treeFault(EXCEPTION_POINTERS*p){std::fprintf(stderr,"tree fault=%08lx eip=%08lx\n",p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip);return EXCEPTION_CONTINUE_SEARCH;}
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
#ifndef SFM64_NATIVE_DICTIONARY_TREE
static constexpr uintptr_t highBits=uintptr_t(0x7654321000000000ull);
static uint32_t identity(uintptr_t v){need((v&~uintptr_t(0xffffffffu))==highBits,"64-bit opaque payload must retain upper bits");return uint32_t(v);}
static std::string snapshot(const MaterialDictionaryTreeCandidate&t){std::string s;for(size_t i=0;i<t.allocatedSlots();++i){const auto*n=t.slot(uint16_t(i));s.append(reinterpret_cast<const char*>(n),sizeof(*n));}return s+std::to_string(t.root())+":"+std::to_string(t.freeHead())+":"+std::to_string(t.size())+":"+std::to_string(t.capacity());}
static int safety(){
    using Tree=MaterialDictionaryTreeCandidate;Tree t;Tree::Index output=4321;unsigned checks=0;auto check=[&](bool b){need(b,"Owned tree safety");++checks;};
    check(t.valid()&&t.capacity()==256&&t.size()==0);for(unsigned i=0;i<256;++i)check(t.insert({highBits+i+1,uint16_t(i),i%2!=0},output)&&t.valid());
    auto before=snapshot(t);output=4321;unsigned allocationCalls=0;check(!t.insert({highBits+900,5,true},output,[&](size_t){++allocationCalls;return false;})&&output==4321&&snapshot(t)==before&&allocationCalls==1);
    check(!t.insert({highBits+900,5,true},output,[](size_t)->bool{throw std::bad_alloc();})&&snapshot(t)==before&&output==4321);
    check(!t.erase(Tree::none)&&!t.erase(500)&&snapshot(t)==before);check(t.erase(11)&&!t.erase(11)&&t.valid());
    allocationCalls=0;check(t.insert({highBits+900,5,true},output,[&](size_t){++allocationCalls;return false;})&&output==11&&allocationCalls==0&&t.valid());
    t.purge();check(t.size()==0&&t.capacity()==0&&t.valid());before=snapshot(t);output=1234;
    check(!t.insert({highBits+1,0,false},output,[](size_t){return false;})&&snapshot(t)==before&&output==1234);
    check(t.insert({highBits+1,0,false},output)&&output==0&&t.capacity()==2&&identity(t.slot(output)->entry.material)==1&&t.valid());
    t.purge();for(unsigned i=0;i<Tree::none;++i)check(t.insert({highBits+i+1,uint16_t(i),false},output));
    check(t.size()==Tree::none&&t.capacity()==Tree::none&&t.valid());before=snapshot(t);output=321;
    check(!t.insert({highBits+90000,0,false},output)&&snapshot(t)==before&&output==321);
    check(t.erase(12345)&&t.insert({highBits+90000,0,true},output)&&output==12345&&t.valid());
    std::cout<<"PASS owned_tree_safety_checks="<<checks<<" max_nodes=65535 high_payload_bits=RETAINED\n";return 0;
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_DICTIONARY_TREE
    if(argc==2&&std::string(argv[1])=="--test")return safety();static_assert(sizeof(void*)==8,"Owned dictionary must be x64");
#else
    static_assert(sizeof(void*)==4,"Original private reference");
#endif
    need(argc==4,"material-or-absent-engine query fresh-output");need(!std::ifstream(argv[3]).good(),"Fresh output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Query IO");
#ifdef SFM64_NATIVE_DICTIONARY_TREE
    DictionaryTreeNativeFixture tree(argv[1]);auto observer=AddVectoredExceptionHandler(1,&treeFault);need(observer!=nullptr,"Fault observer");
#else
    MaterialDictionaryTreeCandidate tree;
#endif
    std::array<uint32_t,4>q{};unsigned count=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]>=1&&q[0]<=4&&q[2]<=0xffff&&q[3]<=1&&q[1]<0x100000,"Query domain");uint16_t result=0xffff;
#ifdef SFM64_NATIVE_DICTIONARY_TREE
        need(q[0]!=1||DictionaryTreeNativeFixture::half(tree.tree(),0x12)<65000,"Bounded native tree");result=tree.execute(q);
        const auto*header=tree.tree();const auto last=tree.half(header,0x16);const uint32_t allocated=last==0xffff?0:uint32_t(last)+1;
        for(auto v:{count,q[0],uint32_t(result),uint32_t(tree.half(header,0x10)),uint32_t(tree.half(header,0x12)),uint32_t(tree.half(header,0x14)),allocated,tree.word(header,8),uint32_t((tree.image.base[0x105974]&1)?tree.half(tree.image.base+0x10596c,6):1)})put(out,v);
        for(uint32_t i=0;i<allocated;++i){auto*n=tree.node(uint16_t(i));for(auto v:{uint32_t(tree.half(n,0)),uint32_t(tree.half(n,2)),uint32_t(tree.half(n,4)),uint32_t(tree.half(n,6)),tree.word(n,8),uint32_t(tree.half(n,12)),uint32_t(n[14])})put(out,v);}
#else
        if(q[0]==1)need(tree.insert({highBits+q[1],uint16_t(q[2]),q[3]!=0},result),"Owned insertion refusal");else if(q[0]==2)result=tree.eraseMaterial(highBits+q[1]);else if(q[0]==3)result=tree.find(uint16_t(q[2]),q[3]!=0);else tree.purge();
        need(tree.valid(),"Owned structure/order/free-list invariant");
        for(auto v:{count,q[0],uint32_t(result),uint32_t(tree.root()),uint32_t(tree.size()),uint32_t(tree.freeHead()),uint32_t(tree.allocatedSlots()),uint32_t(tree.capacity()),uint32_t(tree.sentinelColor())})put(out,v);
        for(size_t i=0;i<tree.allocatedSlots();++i){const auto*n=tree.slot(uint16_t(i));for(auto v:{uint32_t(n->left),uint32_t(n->right),uint32_t(n->parent),uint32_t(n->color),identity(n->entry.material),uint32_t(n->entry.symbol),uint32_t(n->entry.suppliedSource)})put(out,v);}
#endif
        ++count;
    }
    need(count&&in.eof()&&in.gcount()==0,"Complete query framing");out.flush();need(bool(out),"Complete output");
#ifdef SFM64_NATIVE_DICTIONARY_TREE
    tree.finishTree();RemoveVectoredExceptionHandler(observer);std::cout<<"native_allocations="<<tree.allocations<<" releases="<<tree.releases<<'\n';
#endif
    std::cout<<"PASS material_tree_operations="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
