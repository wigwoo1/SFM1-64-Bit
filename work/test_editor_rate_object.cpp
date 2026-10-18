#include "editor_rate_object_candidate.h"
#include "editor_adjust_time_candidate.h"
#include <vector>
#include <string>
#include <iostream>
struct Fixture {
    struct Object {} primary,settings,render;
    struct Attribute {float value=24;} attr;
    std::vector<unsigned> calls;
    bool missingPrimary=false,missingSettings=false,missingRender=false,missingAttribute=false;
    std::uint32_t handle=10;
    std::uint32_t editorHandle80(){calls.push_back(1);return handle;}
    Object* resolve(std::uint32_t h){calls.push_back(2);assert(h==10);return missingPrimary?nullptr:&primary;}
    Object* namedElement(Object* p,const char* name){
        if(std::string(name)=="settings") {assert(p==&primary);calls.push_back(3);return missingSettings?nullptr:&settings;}
        assert(p==&settings && std::string(name)=="renderSettings");calls.push_back(4);return missingRender?nullptr:&render;
    }
    Object* rateObject(){return editorRateObjectCandidate(*this);}
    Attribute* frameRateAttribute(Object* p){assert(p==&render);calls.push_back(5);return missingAttribute?nullptr:&attr;}
    float attributeFloat(Attribute* p){calls.push_back(6);return p->value;}
    SfmFrameRateCandidate constructRate(float v){assert(v==24);calls.push_back(7);return {24,10000};}
};
int main(){
    Fixture f;
    assert(editorAdjustTimeCandidate(209,0,f)==417);
    assert((f.calls==std::vector<unsigned>{1,2,1,2,3,4,5,6,7}));
    f.calls.clear();f.missingPrimary=true;
    assert(editorAdjustTimeCandidate(209,0,f)==417 && (f.calls==std::vector<unsigned>{1,2,7}));
    f.calls.clear();f.missingPrimary=false;f.missingSettings=true;
    assert(editorAdjustTimeCandidate(209,0,f)==417 && (f.calls==std::vector<unsigned>{1,2,1,2,3,7}));
    f.calls.clear();f.missingSettings=false;f.missingRender=true;
    assert(editorAdjustTimeCandidate(209,0,f)==417 && (f.calls==std::vector<unsigned>{1,2,1,2,3,4,7}));
    f.calls.clear();f.missingRender=false;f.missingAttribute=true;
    assert(editorAdjustTimeCandidate(209,0,f)==417 && (f.calls==std::vector<unsigned>{1,2,1,2,3,4,5,7}));
    std::cout<<"x64 rate-object lookup: settings/renderSettings/duplicate resolve/defaults/composed snap passed\n";
}
