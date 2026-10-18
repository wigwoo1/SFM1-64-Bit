#include "named_element_reference_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Fixture {
    struct Element {} root,target;
    struct Attribute {unsigned flags=1;std::uint32_t handle=10;} attr;
    bool missing=false,mutateAfterRead=false,reject=false,unresolved=false;
    unsigned conversions=0,validations=0;
    std::uint32_t lastHandle=0,scratch=20;
    std::vector<unsigned> calls;
    void initializeAttributeDefault(){calls.push_back(2);}
    unsigned attributeFlags(Attribute* p){const auto flags=p->flags;if(mutateAfterRead)p->flags=1;return flags;}
    auto directHandle(Attribute* p){calls.push_back(3);return p->handle;}
    bool isElementHandleType(Attribute*){calls.push_back(4);return (attr.flags&31)==1;}
    void copyIntoSharedHandle(Attribute*){calls.push_back(5);++conversions;scratch=10;}
    auto sharedHandle(){calls.push_back(6);return scratch;}
    std::uint32_t attributeDefaultHandle(){calls.push_back(7);return 0xffffffff;}
    void initializeNamedDefault(){calls.push_back(1);}
    Attribute* findAttribute(Element* p,const char*){assert(p==&root);return missing?nullptr:&attr;}
    std::uint32_t namedDefaultHandle(){calls.push_back(8);return 0xffffffff;}
    Element* resolve(std::uint32_t h){calls.push_back(9);lastHandle=h;return h==10 && !unresolved?&target:nullptr;}
    bool acceptElementType(Element* p){assert(p==&target);calls.push_back(10);++validations;return !reject;}
};
int main(){
    Fixture f;
    assert(namedElementReferenceCandidate(&f.root,"settings",f)==&f.target);
    assert((f.calls==std::vector<unsigned>{1,2,3,9,10}));
    for(unsigned flags=0;flags<256;++flags){
        Fixture g;g.attr.flags=flags;
        assert((namedElementReferenceCandidate(&g.root,"settings",g)!=nullptr)==((flags&31)==1));
        assert(g.lastHandle==((flags&31)==1?10u:0xffffffffu));
    }
    Fixture missing;missing.missing=true;
    assert(!namedElementReferenceCandidate(&missing.root,"settings",missing));
    assert((missing.calls==std::vector<unsigned>{1,8,9}));
    Fixture converted;converted.attr.flags=2;converted.mutateAfterRead=true;
    assert(namedElementReferenceCandidate(&converted.root,"renderSettings",converted)==&converted.target);
    assert(converted.conversions==1 && (converted.calls==std::vector<unsigned>{1,2,4,5,6,9,10}));
    Fixture rejected;rejected.reject=true;
    assert(!namedElementReferenceCandidate(&rejected.root,"settings",rejected) && rejected.validations==1);
    Fixture unresolved;unresolved.unresolved=true;
    assert(!namedElementReferenceCandidate(&unresolved.root,"settings",unresolved) && unresolved.validations==0);
    std::cout<<"x64 named references: 256 type flags/default resolve/type reread/scratch copy ordering/type rejection passed\n";
}
