#include <vector>
#include <iostream>
#include "studio_material_context_teardown_candidate.h"
int main(){unsigned cases=0;for(unsigned linked=0;linked<2;++linked)for(unsigned second=0;second<2;++second)for(unsigned count=1;count<=8;++count){StudioMaterialContextBindingsCandidate bindings{0x100000001ULL,linked?0x200000001ULL:0,0x300000001ULL,second?0x400000001ULL:0};auto saved=bindings;uint32_t users=count;std::vector<unsigned> events;
studioMaterialContextTeardownCandidate(bindings,[&](uintptr_t value){if(value!=saved.firstHelper)throw 1;events.push_back(1);},[&](uintptr_t value,unsigned flags){if(value!=saved.secondHelper||flags!=1)throw 1;events.push_back(2);},[&](unsigned index){if(bindings.material||bindings.immediate||bindings.firstHelper||bindings.secondHelper)throw 1;events.push_back(3+index);},[&]{events.push_back(5);studioMaterialSharedBuffersReleaseCandidate(users,[&](unsigned index){events.push_back(6+index);});});
std::vector<unsigned> expected;if(linked){expected.push_back(1);if(second)expected.push_back(2);expected.insert(expected.end(),{3,4,5});if(count==1)expected.insert(expected.end(),{6,7});}if(events!=expected||users!=count-linked)return 1;
auto before=events.size();if(linked)studioMaterialContextTeardownCandidate(bindings,[&](uintptr_t){throw 1;},[&](uintptr_t,unsigned){throw 1;},[&](unsigned){throw 1;},[]{throw 1;});if(events.size()!=before)return 1;++cases;
}std::cout<<"context_teardown_cases="<<cases<<" ordered_cleanup=1 shared_last_user_release=1 repeated_teardown_noop=1\n";}
