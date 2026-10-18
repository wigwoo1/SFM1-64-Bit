#include "float_components_undo_replay.h"
#include <cstdio>
#include <stdexcept>
template<size_t N>void check(){
 const uint32_t palette[]={0,0x80000000u,0x3f800000u,0x7f800000u,0xff800000u,0x7f801234u,0xff801234u,0x7fc01234u,1,0x807fffffu};
 for(auto bits:palette){std::array<float,N>a,b;for(size_t i=0;i<N;++i){std::memcpy(&a[i],&bits,4);uint32_t other=palette[(i+3)%10];std::memcpy(&b[i],&other,4);}
 RecoveredFloatComponentsUndoPayloadCandidate<N>r;r.owner=123;r.name=456;captureRecoveredFloatUndoValuesCandidate(r,a,b);
 if(r.owner!=123||r.name!=456)throw std::runtime_error("identity changed");
 for(size_t i=0;i<N;++i)for(int side=0;side<2;++side){uint32_t expected,actual;std::memcpy(&expected,&(side?b:a)[i],4);std::memcpy(&actual,&(side?r.after:r.before)[i],4);if constexpr(N==3)if((expected&0x7f800000u)==0x7f800000u&&(expected&0x7fffffu))expected|=0x400000u;if(actual!=expected)throw std::runtime_error("capture bits mismatch");}
 }
}
template<size_t N>void checkDefaults(){std::array<float,N>stored{},incoming{};stored.fill(7.0f);incoming.fill(9.0f);for(unsigned flags=0;flags<1024;++flags){RecoveredFloatComponentsUndoPayloadCandidate<N>r;if constexpr(N==4)if((flags&31)==12){bool rejected=false;try{captureRecoveredFloatUndoAttributeCandidate(r,uint16_t(flags),stored,incoming);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("Euler silently accepted");continue;}captureRecoveredFloatUndoAttributeCandidate(r,uint16_t(flags),stored,incoming);for(size_t i=0;i<N;++i){float expected=(flags&31)==(N==3?10u:13u)?7.0f:((N==4&&i==3)?1.0f:0.0f);if(r.before[i]!=expected||r.after[i]!=9.0f)throw std::runtime_error("type fallback");}}}
int main(){static_assert(sizeof(void*)==8);check<3>();check<4>();checkDefaults<3>();checkDefaults<4>();std::puts("typed undo capture: 140 bit-copy checks and 2048 type/flag cases passed");}
