from pathlib import Path
p=Path('work/test_dmx_editor_timing.cpp');s=p.read_text().replace('#include "editor_adjust_time_candidate.h"','#include "editor_adjust_time_candidate.h"\n#include "attribute_float_candidate.h"')
s=s.replace('    float attributeFloat(Attribute* a){if(a->type!=3 || a->values.size()!=1)throw std::runtime_error("float extraction helper unsupported type");return wireFloat(a->values[0]);}', '''    float floatScratch=0;
    void initializeFloatDefault(){}
    float floatDefault(){return 0;}
    float directFloat(Attribute* a){return wireFloat(a->values.at(0));}
    auto signedValue(Attribute* a){return wireInt(a->values.at(0));}
    unsigned char booleanByte(Attribute* a){if(a->values.at(0).wire.empty())throw std::runtime_error("missing boolean data");return static_cast<unsigned char>(a->values[0].wire[0]);}
    float copyFloatX87(Attribute*){throw std::runtime_error("immutable graph cannot enter reread float-copy branch");}
    void setSharedFloat(float v){floatScratch=v;}
    float sharedFloat(){return floatScratch;}
    float attributeFloat(Attribute* a){if(a->values.size()!=1)throw std::runtime_error("invalid scalar cardinality");return attributeFloatCandidate(a,*this);}''')
s=s.replace('        std::cout<<"session="', '''        const auto savedRate=*rate;
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
        std::cout<<"session="''')
p.write_text(s)
