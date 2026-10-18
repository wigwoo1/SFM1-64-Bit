#include "graph_modifier_transition_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned prior,mode=99,resets=0,stage=0;unsigned state1c(){return prior&1;}unsigned state1d(){return prior&2;}unsigned state1e(){return prior&4;}
 void reset100d34e0(){assert(stage==0);++resets;stage=1;}void capturePointAfterReset(){stage=2;}void setMode174(unsigned n){assert(stage==2);mode=n;}
};
int main(){for(unsigned p=0;p<8;++p)for(unsigned m=0;m<8;++m)for(unsigned b=0;b<8;++b){Adapter a{p};auto s=graphModifierTransitionCandidate(m<<25,b,a);assert(s.bit25==bool(m&1) && s.bit26==bool(m&2) && s.bit27==bool(m&4) && s.left==bool(b&1) && s.middle==bool(b&4));assert(a.resets==unsigned(((p&1)&&!(m&4))||((p&2)&&!(m&2))||((p&4)&&!(m&1))));unsigned expected[4]={0,3,2,1};assert(a.mode==expected[m>>1]);}
 std::cout<<"x64 graph modifier transition:512 prior/modifier/button combinations and reset/capture/mode order passed\n";}
