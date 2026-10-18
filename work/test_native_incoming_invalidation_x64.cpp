#include "attribute_mutation_gate.h"
#include "element_mutation_state.h"
#include "reference_update_queues.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream file(argv[1],std::ios::binary);unsigned cases=0;for(unsigned phase=0;phase<8;++phase)for(unsigned initial=0;initial<65536;++initial){uint16_t flags=uint16_t(initial);uint8_t state=uint8_t(initial);unsigned prepares=0;RecoveredReferenceUpdateQueueCandidate queue;bool result=beginAttributeMutation(flags,phase,[&](){++prepares;},[&](){markElementMutation(state,123,[&](uint32_t h){queue.append(h);});});std::array<uint32_t,7>actual{0xabcd0000u|flags,0xa500beefu|(uint32_t(state)<<16),uint32_t(queue.handles.size()),queue.handles.empty()?0:queue.handles[0],uint32_t(result),prepares,initial&0x20?0u:1u},expected{};if(!file.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");++cases;}char extra;if(file.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_incoming_attribute_invalidation_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
