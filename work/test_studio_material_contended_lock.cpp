#include <iostream>
#include "studio_material_contended_lock_candidate.h"
int main(){unsigned cases=0;
for(int parameter: {-1,0,1})for(int priorityValue: {0,1})for(unsigned target: {1u,8193u,16385u,24577u}){StudioMaterialImmediateLockCandidate lock;lock.owner=99;unsigned pauses=0,sleeps=0,priorities=0,positiveSleeps=0;
studioMaterialContendedLockCandidate(lock,7,parameter,[&]{if(++pauses==target)lock.owner=0;},[&](int milliseconds){++sleeps;if(milliseconds)++positiveSleeps;},[&]{++priorities;return priorityValue;});
if(lock.owner!=7||lock.depth!=1||pauses!=target)return 1;
unsigned expectedSleeps=0,expectedPositive=0,expectedPriorities=0;
if(parameter!=-1){auto second=target>8192?target-8192:0u;if(second>8192)second=8192;expectedSleeps=second?1+(second-1)/1024:0;
if(target>16384){expectedPriorities=parameter==0?1:0;bool third=parameter!=0||priorityValue>0;unsigned remaining=target-16384;expectedSleeps+=remaining;if(third){if(remaining>8192)expectedPositive=remaining-8192;}else if(parameter)expectedPositive=remaining;}}
if(sleeps!=expectedSleeps||positiveSleeps!=expectedPositive||priorities!=expectedPriorities)return 1;studioMaterialImmediateEndCandidate(lock);++cases;
}std::cout<<"contended_wait_stage_cases="<<cases<<" staged_pause_sleep_priority_counts=1\n";}
