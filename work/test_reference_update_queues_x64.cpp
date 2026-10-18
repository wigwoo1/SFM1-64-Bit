#include "reference_update_queues.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream file(argv[1],std::ios::binary);for(unsigned kind=0;kind<3;++kind){RecoveredReferenceUpdateQueueCandidate q;unsigned growth=0;for(unsigned step=0;step<256;++step){auto old=q.storageCapacity;q.append(step%7==0?0xffffffffu:step%5);growth+=q.storageCapacity!=old;std::array<uint32_t,259>actual{},expected{};actual[0]=uint32_t(q.handles.size());actual[1]=q.storageCapacity;actual[2]=growth;for(unsigned i=0;i<q.handles.size();++i)actual[3+i]=q.handles[i];if(!file.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");}}char extra;if(file.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_update_queue_steps=768"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
