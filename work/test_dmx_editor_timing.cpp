#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "editor_rate_object_candidate.h"
#include "named_element_reference_candidate.h"
#include "editor_adjust_time_candidate.h"
#include "attribute_float_candidate.h"
#include "base_element_type_validation_candidate.h"
#include <cmath>
#include <cassert>
struct SessionTimingAdapter {
    Graph& g;std::uint32_t owner;
    unsigned resolves=0;
    std::uint32_t editorHandle80(){return owner;}
    Element* resolve(std::uint32_t index){++resolves;return index<g.elements.size()?&g.elements[index]:nullptr;}
    void initializeNamedDefault(){}
    void initializeAttributeDefault(){}
    Attribute* findAttribute(Element* e,const char* name){
        for(auto& a:e->attributes)if(g.strings.at(a.name)==name)return &a;
        return nullptr;
    }
    auto attributeFlags(Attribute* a){return a->type;}
    auto directHandle(Attribute* a){if(a->values.size()!=1)throw std::runtime_error("invalid reference cardinality");return a->values[0].reference;}
    bool isElementHandleType(Attribute* a){return (a->type&31)==1;}
    void copyIntoSharedHandle(Attribute*){throw std::runtime_error("immutable graph cannot change type between reads");}
    std::uint32_t sharedHandle(){throw std::runtime_error("unreachable scratch");}
    std::uint32_t attributeDefaultHandle(){return 0xffffffff;}
    std::uint32_t namedDefaultHandle(){return 0xffffffff;}
    // Both retained settings nodes serialize as DmElement. Symbolic host
    // token represents base equality only; no original global token invented.
    bool acceptElementType(Element* e){
        if(g.strings.at(e->type)!="DmElement")throw std::runtime_error("derived graph type validation not recovered");
        constexpr std::uint32_t hostBaseToken=1;
        return baseElementAcceptsTypeCandidate(hostBaseToken,hostBaseToken);
    }
    Element* namedElement(Element* e,const char* name){return namedElementReferenceCandidate(e,name,*this);}
    Element* rateObject(){return editorRateObjectCandidate(*this);}
    Attribute* frameRateAttribute(Element* e){return findAttribute(e,"frameRate");}
    float floatScratch=0;
    void initializeFloatDefault(){}
    float floatDefault(){return 0;}
    float directFloat(Attribute* a){return wireFloat(a->values.at(0));}
    auto signedValue(Attribute* a){return wireInt(a->values.at(0));}
    unsigned char booleanByte(Attribute* a){if(a->values.at(0).wire.empty())throw std::runtime_error("missing boolean data");return static_cast<unsigned char>(a->values[0].wire[0]);}
    float copyFloatX87(Attribute*){throw std::runtime_error("immutable graph cannot enter reread float-copy branch");}
    void setSharedFloat(float v){floatScratch=v;}
    float sharedFloat(){return floatScratch;}
    float attributeFloat(Attribute* a){if(a->values.size()!=1)throw std::runtime_error("invalid scalar cardinality");return attributeFloatCandidate(a,*this);}
    auto constructRate(float f){return constructSfmFrameRateCandidate(f,[](float v){return static_cast<std::int32_t>(std::nearbyint(v));});}
};
int main(){try {
    for(auto path:{"work/roundtrip-11.dmx","work/roundtrip-139.dmx"}) {
        auto g=loadGraph(path);std::uint32_t owner=0xffffffff;
        for(std::uint32_t i=0;i<g.elements.size();++i)if(!hierarchyRefs(g,g.elements[i],"activeClip",1).empty()) {
            if(owner!=0xffffffff)throw std::runtime_error("ambiguous session owner");owner=i;
        }
        if(owner==0xffffffff)throw std::runtime_error("missing owner");
        SessionTimingAdapter a{g,owner};auto* render=a.rateObject();
        if(!render)throw std::runtime_error("missing render object");
        auto* rate=a.frameRateAttribute(render);if(!rate)throw std::runtime_error("missing session rate");
        const auto fps=a.attributeFloat(rate);
        assert(g.strings.at(render->type)=="DmElement");
        const auto snapped=editorAdjustTimeCandidate(209,0,a);
        const auto expected=sfmAdjustTimeCandidate(209,a.constructRate(fps),0);
        assert(snapped==expected && a.resolves==8);
        // In-memory mutation only, proving recovered missing-reference fallback.
        auto* settings=a.findAttribute(&g.elements[owner],"settings");assert(settings);
        const auto original=settings->values[0].reference;
        settings->values[0].reference=0xffffffff;
        assert(editorAdjustTimeCandidate(209,0,a)==417);
        settings->values[0].reference=original;
        const auto savedRate=*rate;
        rate->type=2;setWireU32(rate->values[0],30);
        assert(a.attributeFloat(rate)==30 && editorAdjustTimeCandidate(209,0,a)==333);
        rate->type=7;setWireU32(rate->values[0],600000);
        assert(a.attributeFloat(rate)==60 && editorAdjustTimeCandidate(209,0,a)==167);
        rate->type=4;rate->values[0].wire={2};
        assert(a.attributeFloat(rate)==1 && editorAdjustTimeCandidate(209,0,a)==0);
        rate->type=5;rate->values[0].wire.clear();
        assert(a.attributeFloat(rate)==0); // Present unsupported differs from absent24.
        *rate=savedRate;
        const auto rateIndex=static_cast<std::size_t>(rate-render->attributes.data());
        render->attributes.erase(render->attributes.begin()+rateIndex);
        assert(editorAdjustTimeCandidate(209,0,a)==417);
        render->attributes.insert(render->attributes.begin()+rateIndex,savedRate);
        assert(editorAdjustTimeCandidate(209,0,a)==167);
        validateGraph(g);
        std::cout<<"session="<<path<<" elements="<<g.elements.size()<<" fps="<<fps<<" snap209="<<snapped<<" numeric_accessor=int30/time60/bool1/unsupported0 missing_rate_fallback=417 base_DmElement_checked restored_graph_valid passed\n";
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
