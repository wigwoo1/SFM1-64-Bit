#include "editor_rate_object_candidate.h"
#include "editor_adjust_time_candidate.h"
#include "named_element_reference_candidate.h"
#include <cmath>
#include <vector>
#include <string>
#include <iostream>
struct Fixture {
    struct Element {} primary,settings,render;
    struct Attribute {unsigned flags=1;std::uint32_t handle;} settingsAttr{1,20},renderAttr{1,30};
    struct FloatAttribute {float value=30;} frameRate;
    bool missingPrimary=false,missingSettings=false,missingRender=false,missingRate=false;
    bool rejectSettings=false,rejectRender=false;
    unsigned primaryResolves=0,settingsChecks=0,renderChecks=0,rateReads=0;
    std::vector<std::uint32_t> resolved;
    std::uint32_t editorHandle80(){return 10;}
    Element* resolve(std::uint32_t h){resolved.push_back(h);if(h==10){++primaryResolves;return missingPrimary?nullptr:&primary;}if(h==20)return &settings;if(h==30)return &render;return nullptr;}
    void initializeNamedDefault(){}
    void initializeAttributeDefault(){}
    Attribute* findAttribute(Element* p,const char* name){
        if(std::string(name)=="settings"){assert(p==&primary);return missingSettings?nullptr:&settingsAttr;}
        assert(p==&settings && std::string(name)=="renderSettings");return missingRender?nullptr:&renderAttr;
    }
    auto attributeFlags(Attribute* p){return p->flags;}
    auto directHandle(Attribute* p){return p->handle;}
    bool isElementHandleType(Attribute* p){return (p->flags&31)==1;}
    void copyIntoSharedHandle(Attribute*){assert(false);}
    std::uint32_t sharedHandle(){assert(false);return 0;}
    std::uint32_t attributeDefaultHandle(){return 0xffffffff;}
    std::uint32_t namedDefaultHandle(){return 0xffffffff;}
    bool acceptElementType(Element* p){if(p==&settings){++settingsChecks;return !rejectSettings;}assert(p==&render);++renderChecks;return !rejectRender;}
    Element* namedElement(Element* p,const char* name){return namedElementReferenceCandidate(p,name,*this);}
    Element* rateObject(){return editorRateObjectCandidate(*this);}
    FloatAttribute* frameRateAttribute(Element* p){assert(p==&render);return missingRate?nullptr:&frameRate;}
    float attributeFloat(FloatAttribute* p){++rateReads;return p->value;}
    auto constructRate(float v){return constructSfmFrameRateCandidate(v,[](float f){return static_cast<std::int32_t>(std::nearbyint(f));});}
};
int main(){
    Fixture f;
    assert(editorAdjustTimeCandidate(209,0,f)==333);
    assert((f.resolved==std::vector<std::uint32_t>{10,10,20,30}));
    assert(f.primaryResolves==2 && f.settingsChecks==1 && f.renderChecks==1 && f.rateReads==1);
    for(unsigned flags=0;flags<256;++flags){
        Fixture g;g.settingsAttr.flags=flags;
        assert(editorAdjustTimeCandidate(209,0,g)==((flags&31)==1?333:417));
        assert(g.rateReads==((flags&31)==1?1u:0u));
    }
    Fixture missing;missing.missingSettings=true;
    assert(editorAdjustTimeCandidate(209,0,missing)==417 && missing.resolved.back()==0xffffffff && missing.renderChecks==0);
    Fixture wrong;wrong.rejectRender=true;
    assert(editorAdjustTimeCandidate(209,0,wrong)==417 && wrong.renderChecks==1 && wrong.rateReads==0);
    Fixture stale;stale.renderAttr.handle=0x80000000;
    assert(editorAdjustTimeCandidate(209,0,stale)==417 && stale.renderChecks==0);
    Fixture noRate;noRate.missingRate=true;
    assert(editorAdjustTimeCandidate(209,0,noRate)==417 && noRate.rateReads==0);
    Fixture fractional;fractional.frameRate.value=29.97f;
    assert(editorAdjustTimeCandidate(10010000,0,fractional)==10010000);
    std::cout<<"x64 typed settings timing: handle chain/256 flags/type reject/stale/default/fractional snap passed\n";
}
