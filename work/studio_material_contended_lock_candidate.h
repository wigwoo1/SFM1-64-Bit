#pragma once
#include "studio_material_immediate_lock_candidate.h"
//100104c0. Parameters and scheduling callbacks match recovered Tier0 stages.
template<class Pause,class Sleep,class Priority>void studioMaterialContendedLockCandidate(StudioMaterialImmediateLockCandidate& lock,uint32_t thread,int sleepParameter,Pause pause,Sleep sleep,Priority priority){
 auto attempt=[&]{if(lock.owner.load()==thread)return true;uint32_t expected=0;return lock.owner.compare_exchange_strong(expected,thread);};
 auto acquired=[&]{++lock.depth;};
 if(sleepParameter==-1){for(;;){if(attempt()){acquired();return;}pause();}}
 for(unsigned remaining=8192;remaining;--remaining){if(attempt()){acquired();return;}pause();}
 for(unsigned remaining=8192;remaining;--remaining){if(attempt()){acquired();return;}pause();if(remaining%1024==0)sleep(0);}
 if(!sleepParameter&&priority()>0)sleepParameter=1;
 if(sleepParameter){for(unsigned remaining=8192;remaining;--remaining){if(attempt()){acquired();return;}pause();sleep(0);}}
 for(;;){if(attempt()){acquired();return;}pause();sleep(sleepParameter);}
}
