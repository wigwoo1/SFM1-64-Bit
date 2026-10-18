#include "base_element_factory_creation_candidate.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
struct Fixture {
    struct Element {} element;
    bool fail=false;
    std::vector<unsigned> calls;
    Element* allocateNativeBaseElement(){calls.push_back(1);return fail?nullptr:&element;}
    const char* factoryName(){calls.push_back(2);return "DmElement";}
    Element* constructBaseElement(Element* p,std::uintptr_t first,const char* name,std::uintptr_t third,std::uintptr_t fourth,std::uintptr_t fifth){
        calls.push_back(3);assert(p==&element && std::string(name)=="DmElement");
        assert(first==0x100000001ull && third==0x500000005ull && fourth==0x300000003ull && fifth==0x400000004ull);return p;
    }
};
int main(){
    static_assert(sizeof(void*)==8,"native x64 required");
    BaseElementFactoryArgumentsCandidate args{0x100000001ull,0x200000002ull,0x300000003ull,0x400000004ull,0x500000005ull};
    Fixture f;assert(createBaseElementCandidate(args,f)==&f.element && (f.calls==std::vector<unsigned>{1,2,3}));
    f.calls.clear();f.fail=true;
    assert(!createBaseElementCandidate(args,f) && (f.calls==std::vector<unsigned>{1}));
    f.calls.clear();f.fail=false;args.arg1=0;assert(createBaseElementCandidate(args,f)==&f.element);
    std::cout<<"x64 base factory: native allocation/null gate/name/constructor argument permutation/wide values passed\n";
}
