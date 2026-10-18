#include "studio_restore_cached_state_candidate.h"
#include <cstdio>
struct Services{unsigned& cache;unsigned calls=0;
 void submit(unsigned key,unsigned value){if(key!=23||value!=3)throw 1;++calls;cache=99;}
};
int main(){unsigned cache=3;Services services{cache};studioRestoreCachedStateCandidate(cache,23,3,services);if(services.calls||cache!=3)return 1;cache=1;studioRestoreCachedStateCandidate(cache,23,3,services);if(services.calls!=1||cache!=3)return 2;std::puts("restore_cached_state_cases=2 passed");return 0;}
