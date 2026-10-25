#include "shader_skin_material_predicate_candidate.h"
#include "material_var_defined_candidate.h"
#include <iostream>
#include <stdexcept>
int main(){try{
    static_assert(sizeof(void*)==8,"x64 required");unsigned checks=0;
    auto check=[&](const ShaderSkinMaterialInputsCandidate&i,bool wants,bool queried){const auto r=shaderSkinMaterialPredicateCandidate(i);if(r.wantsSkin!=wants||r.bumpQueried!=queried)throw std::runtime_error("skin predicate decision/trace");++checks;};
    check({0,1,uint8_t(3),0,true},false,false);
    check({1,std::nullopt,uint8_t(3),0,true},false,false);
    check({1,0,uint8_t(3),0,true},false,false);
    check({1,-1,uint8_t(0xf3),0,false},true,false);
    check({1,1,uint8_t(0),std::nullopt,std::nullopt},true,false);
    check({1,1,std::nullopt,1,false},true,false);
    check({1,1,std::nullopt,0,std::nullopt},false,false);
    check({1,1,std::nullopt,0,false},false,true);
    check({1,1,std::nullopt,2,true},true,true);
    for(int level:{-1,91,92,100})for(bool wants:{false,true}){if(shaderSkinDrawGateCandidate({wants,false},level)!=(wants&&level>=92))throw std::runtime_error("skin caller hardware gate");++checks;}
    // A defined string bump value (type1), float0, vector2, texture3 or int4
    // accepts this gate. UNDEFINED6 does not; high type bits are ignored.
    for(unsigned type=0;type<256;++type)
        check({1,1,uint8_t(1),0,materialVarDefinedTypeCandidate(uint8_t(type))},(type&15)!=6,true);
    std::cout<<"PASS checks="<<checks<<" typed_effective_inputs_only=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
