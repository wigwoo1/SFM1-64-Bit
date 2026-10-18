#include "resolve_queue_dirty_mark.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
static uint32_t read(std::ifstream&in){uint32_t v;if(!in.read(reinterpret_cast<char*>(&v),4))throw std::runtime_error("short oracle");return v;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream in(argv[1],std::ios::binary);for(unsigned initial=0;initial<256;++initial){uint8_t flags=static_cast<uint8_t>(initial),observed=0;unsigned calls=0;uint32_t queued=0;for(unsigned pass=0;pass<2;++pass){recoveredResolveQueueDirtyMarkCandidate(flags,0x12345678,[&](uint32_t h){++calls;queued=h;observed=flags;});for(auto v:{uint32_t(flags),calls,queued,uint32_t(observed)})if(v!=read(in))throw std::runtime_error("dirty queue transition mismatch");}}if(in.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64 resolve queue:512 native dirty marks matched\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
