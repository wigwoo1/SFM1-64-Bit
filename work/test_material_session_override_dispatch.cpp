#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_material_override_dispatch_candidate.h"
#include <iostream>
struct OverrideSafetyServices:MaterialSessionOverrideServicesCandidate {
 unsigned finds=0,writes=0,textureReads=0;bool present=true,found=true,throwOnWrite=false;
 std::optional<uintptr_t>textureResult;uintptr_t lastResource=1;uint32_t lastWord=0,lastCount=0;
 std::vector<std::string>names;std::array<uint32_t,16>lastMatrix{};
 MaterialSessionOverrideLookupCandidate find(std::string_view s)override{++finds;names.emplace_back(s);return {present?uintptr_t(17):0,found};}
 void note(){if(throwOnWrite)throw std::runtime_error("Explicit staged service failure");++writes;}
 void integer(uintptr_t,uint32_t v)override{note();lastWord=v;}
 void scalar(uintptr_t,uint32_t v)override{note();lastWord=v;}
 void vector(uintptr_t,const std::array<uint32_t,4>&v,uint32_t n)override{note();lastWord=v[0];lastCount=n;}
 void matrix(uintptr_t,const std::array<uint32_t,16>&v)override{note();lastMatrix=v;}
 std::optional<uintptr_t>texture(const std::optional<std::string>&)override{++textureReads;return textureResult;}
 void setTexture(uintptr_t,uintptr_t v)override{note();lastResource=v;}
};
int main(){try{
 static_assert(sizeof(void*)==8,"Owned typed override safety x64");unsigned checks=0;
 auto check=[&](bool b){if(!b)throw std::runtime_error("Override safety assertion "+std::to_string(checks));++checks;};
 MaterialSessionOverrideInputCandidate scalar;scalar.name="$a";scalar.typeFlags=3;scalar.words[0]=0x7fa12345;
 OverrideSafetyServices s;check(dispatchMaterialSessionOverridesCandidate({scalar},s)&&s.lastWord==0x7fa12345&&s.writes==1);
 s={};auto tooLong=scalar;tooLong.name.assign(4096,'a');check(!dispatchMaterialSessionOverridesCandidate({scalar,tooLong},s)&&!s.finds&&!s.writes);
 s={};check(!dispatchMaterialSessionOverridesCandidate(std::vector<MaterialSessionOverrideInputCandidate>(4097,scalar),s)&&!s.finds);
 s={};auto unavailable=scalar;unavailable.typeFlags=5;unavailable.textureName="[1 1 1]";check(!dispatchMaterialSessionOverridesCandidate({unavailable},s)&&s.textureReads==1&&!s.writes&&s.lastResource==1);
 s={};s.textureResult=uintptr_t(0);check(dispatchMaterialSessionOverridesCandidate({unavailable},s)&&s.writes==1&&s.lastResource==0&&!s.lastCount);
 s={};s.present=false;s.found=true;check(dispatchMaterialSessionOverridesCandidate({unavailable},s)&&s.finds==1&&!s.textureReads&&!s.writes);
 s={};s.present=true;s.found=false;check(dispatchMaterialSessionOverridesCandidate({unavailable},s)&&s.finds==1&&!s.textureReads&&!s.writes);
 s={};std::vector<MaterialSessionOverrideInputCandidate>ordered{scalar,scalar,scalar};ordered[0].name="$b";ordered[2].name="$b";check(dispatchMaterialSessionOverridesCandidate(ordered,s)&&s.names==std::vector<std::string>({"$b","$a","$b"}));
 s={};auto flags=scalar;flags.name="$FLAGS";auto flags2=flags;flags2.name="$fLaGs2";auto defined=scalar;defined.name="$flags_defined";check(dispatchMaterialSessionOverridesCandidate({flags,defined,flags2},s)&&s.finds==1&&s.names[0]=="$flags_defined");
 s={};auto matrix=scalar;matrix.typeFlags=14;for(unsigned i=0;i<16;++i)matrix.words[i]=0x7f800001+i;check(dispatchMaterialSessionOverridesCandidate({matrix},s)&&s.lastMatrix==matrix.words);
 s={};auto ignored=scalar;ignored.typeFlags=17;check(dispatchMaterialSessionOverridesCandidate({ignored},s)&&s.finds==1&&!s.writes);
 s={};s.throwOnWrite=true;bool threw=false;try{dispatchMaterialSessionOverridesCandidate({scalar},s);}catch(const std::runtime_error&){threw=true;}check(threw&&!s.writes); // Caller owns transaction rollback.
 Graph g;g.strings={"DmeMaterial","$b","$a","mtlName"};Element e{};e.type=0;
 Value raw;raw.wire.resize(4);const uint32_t bits=0x7fa12345;std::memcpy(raw.wire.data(),&bits,4);
 e.attributes={{1,3,{raw}},{2,3,{raw}},{1,3,{raw}}};g.elements.push_back(e);
 auto parsed=readSessionMaterialOverrideDispatchCandidate(g,0);check(parsed.size()==3&&parsed[0].name=="$b"&&parsed[1].name=="$a"&&parsed[2].name=="$b"&&parsed[0].words[0]==bits);
 check(g.elements[0].attributes[0].values[0].wire==raw.wire);
 auto bad=g;bad.elements[0].attributes[1].values[0].wire.resize(3);threw=false;try{readSessionMaterialOverrideDispatchCandidate(bad,0);}catch(const std::runtime_error&){threw=true;}check(threw);
 bad=g;bad.elements[0].type=1;threw=false;try{readSessionMaterialOverrideDispatchCandidate(bad,0);}catch(const std::runtime_error&){threw=true;}check(threw);
 bad=g;bad.elements[0].attributes[1].name=99;threw=false;try{readSessionMaterialOverrideDispatchCandidate(bad,0);}catch(const std::out_of_range&){threw=true;}check(threw);
 bad=g;bad.elements[0].attributes[0].type=14;threw=false;try{readSessionMaterialOverrideDispatchCandidate(bad,0);}catch(const std::runtime_error&){threw=true;}check(threw);
 bad=g;bad.elements[0].attributes[0].type=5;bad.elements[0].attributes[0].values[0].text="[1 1 1]";parsed=readSessionMaterialOverrideDispatchCandidate(bad,0);check(parsed[0].typeFlags==5&&parsed[0].textureName==std::optional<std::string>("[1 1 1]"));
 std::cout<<"PASS typed_override_safety_checks="<<checks<<" transaction_owner=CALLER graph_order=NATIVE_LOAD_UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
