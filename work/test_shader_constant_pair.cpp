#include "studio_shader_constant_pair_candidate.h"
#include "studio_vertex_constant_owner_candidate.h"
#include <cstdio>
struct Ops {
 std::array<uint32_t,80> initial{},replacement{},secondary{};
 std::array<uint32_t,80> secondaryReplacement{};uint32_t* secondaryPointer=secondary.data();
 uint32_t* primary=initial.data();std::array<uint32_t,4>& parameters;std::array<uint32_t,4>& color;
 bool replace,mutate;unsigned calls=0;uint32_t indices[2]{};
 uint32_t* primaryCache(){return primary;}uint32_t* secondaryCache(){return secondaryPointer;}
 void submit(uint32_t index,uint32_t count,const uint32_t*){
  if(count!=1||calls>=2)throw 1;indices[calls++]=index;
  if(replace){primary=replacement.data();secondaryPointer=secondaryReplacement.data();}
  if(mutate){if(index==16)parameters[0]=0xabcdef01;else color[0]=0xabcdef02;}
 }
};
int main(){unsigned cases=0;
 for(unsigned mode=0;mode<16;++mode){
  std::array<uint32_t,4> parameters{0,0x3f800000,1,2},color{3,4,5,6};
  Ops ops{{},{},{},{},nullptr,nullptr,parameters,color,bool(mode&4),bool(mode&8)};ops.secondaryPointer=ops.secondary.data();ops.primary=ops.initial.data();
  // Signed zero has different rawbits and must cause submission on this path.
  std::memcpy(ops.initial.data()+64,parameters.data(),16);std::memcpy(ops.initial.data()+8,color.data(),16);
  if(mode&1)ops.initial[64]=0x80000000;if(mode&2)ops.initial[8]=99;
  ops.replacement=ops.initial;
  StudioVertexConstantOwnerCandidate owner{ops.primary,ops.secondaryPointer};
  studioVertexConstantOwnerPairCandidate(owner,parameters,color,ops);
  const unsigned expected=unsigned(bool(mode&1))+unsigned(bool(mode&2));if(ops.calls!=expected)return 2;
  if(mode&1){if(ops.indices[0]!=16||std::memcmp(ops.primary+64,parameters.data(),16)||std::memcmp(ops.secondaryPointer+64,parameters.data(),16))return 3;}
  if(mode&2){if(ops.indices[expected-1]!=2||std::memcmp(ops.primary+8,color.data(),16)||std::memcmp(ops.secondaryPointer+8,color.data(),16))return 4;}
  ++cases;
 }
 std::printf("constant_pair_cases=%u raw_equality=1 post_callback_pointer_reload=1 dual_cache_publication=1\n",cases);return 0;
}

