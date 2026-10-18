#include "undo_stack_availability_candidate.h"
#include <cassert>
#include <iostream>
int main(){
    const std::uint32_t values[]={0,1,2,0x7fffffff,0x80000000,0xfffffffe,0xffffffff};
    for(auto v:values){assert(undoAvailableCandidate(v)==(v!=0));const bool positive=v>0 && v<=0x7fffffff;assert(redoAvailableCandidate(v)==positive && undoGroupChangedCandidate(v)==positive);}
    assert(undoAvailableCandidate(0xffffffff) && !redoAvailableCandidate(0xffffffff));
    std::cout<<"x64 undo availability: zero/positive/sign boundary/negative raw counts passed\n";
}
