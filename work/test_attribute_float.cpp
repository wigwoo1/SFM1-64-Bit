#include "attribute_float_candidate.h"
#include <cassert>
#include <iostream>
struct Fixture {
    struct Attribute {unsigned flags=3;std::int32_t integer=60;float value=60;unsigned char boolean=2;} attr;
    float scratch=-1;unsigned writes=0;
    void initializeFloatDefault(){}
    auto attributeFlags(Attribute* p){return p->flags;}
    float directFloat(Attribute* p){return p->value;}
    float floatDefault(){return 0;}
    auto signedValue(Attribute* p){return p->integer;}
    auto booleanByte(Attribute* p){return p->boolean;}
    float copyFloatX87(Attribute* p){return p->value;}
    void setSharedFloat(float v){scratch=v;++writes;}
    float sharedFloat(){return scratch;}
};
int main(){
    for(unsigned flags=0;flags<256;++flags){
        Fixture f;f.attr.flags=flags;float expected=0;
        switch(flags&31){case 2:case 3:expected=60;break;case 4:expected=1;break;case 7:expected=_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(60),_mm_set_ss(0.0001f)));break;}
        assert(attributeFloatCandidate(&f.attr,f)==expected);
        assert(f.writes==(((flags&31)==2 || (flags&31)==4 || (flags&31)==7)?1u:0u));
    }
    Fixture t;t.attr.flags=7;t.attr.integer=-3125;assert(attributeFloatCandidate(&t.attr,t)==-0.3125f);
    t.attr.flags=4;t.attr.boolean=0;assert(attributeFloatCandidate(&t.attr,t)==0);
    std::cout<<"x64 float accessor: 256 flags/direct/int/bool/time/unsupported zero passed\n";
}
