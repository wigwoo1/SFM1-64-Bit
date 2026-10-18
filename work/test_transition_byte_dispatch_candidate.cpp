#include "studio_transition_byte_dispatch_candidate.h"
#include <cstdio>
struct State{unsigned calls=0,last=0;std::uint8_t* live=nullptr;};
static void ordinary(State* state,unsigned arg){if(arg)throw 1;++state->calls;state->last=100;if(state->live)state->live[1]=0x9f;}
static void packed0(State* state,unsigned arg){++state->calls;state->last=arg;}
static void packed1(State* state,unsigned arg){++state->calls;state->last=16+arg;}
int main(){std::array<void(*)(State*,unsigned),13> basic{};basic.fill(ordinary);std::array<void(*)(State*,unsigned),2> compressed{packed0,packed1};unsigned cases=0;
 for(unsigned code=0;code<256;++code){if(!(code<13||(code>=0x80&&code<=0x9f)))continue;State state;const auto op=static_cast<uint8_t>(code);studioTransitionByteDispatchCandidate(&state,&op,1,basic,compressed);if(state.calls!=1||state.last!=(code<13?100u:((code>>4)&1)*16+(code&15)))return 1;++cases;}
 std::uint8_t ops[]{0,0};State state{0,0,ops};studioTransitionByteDispatchCandidate(&state,ops,2,basic,compressed);if(state.calls!=2||state.last!=31)return 2;
 studioTransitionByteDispatchCandidate<State>(nullptr,nullptr,-1,basic,compressed);std::puts("transition_byte_dispatch_valid_cases=45 live_mutation=passed empty=passed");return 0;}
