#include "mdl_flex_metadata.h"
#include <fstream>
#include <iostream>
#include <iomanip>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc<2)throw std::runtime_error("MDL paths required");for(int a=1;a<argc;++a){std::ifstream f(argv[a],std::ios::binary);if(!f)throw std::runtime_error("MDL read failed");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});auto metadata=readMdl49FlexMetadata(b);std::cout<<argv[a]<<" controllers="<<metadata.size()<<'\n';for(auto& c:metadata)std::cout<<c.localIndex<<'\t'<<c.type<<'\t'<<c.name<<'\t'<<c.diskGlobalIndex<<'\t'<<std::hex<<c.minBits<<'\t'<<c.maxBits<<std::dec<<'\n';}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
