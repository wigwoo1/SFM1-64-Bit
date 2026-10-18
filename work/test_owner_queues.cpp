#include "owner_change_queues.h"
#include <iostream>
#include <stdexcept>
int main(){try{OwnerChangeQueues q;uint8_t a=0,b=0;q.markDirty(7,a);q.markDirty(7,a);q.markDirty(9,b);q.submitAttributeMoved(7);q.submitAttributeMoved(7);
 if(q.dirty!=std::vector<uint32_t>{7,9}||a!=3||b!=3||q.attributeMoved!=std::vector<uint32_t>{7,7})throw std::runtime_error("queue mismatch");a&=0xfe;q.markDirty(7,a);if(q.dirty!=std::vector<uint32_t>{7,9,7})throw std::runtime_error("dirty resubmission mismatch");std::cout<<"queue ordering, guard, duplicate and resubmission checks passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
