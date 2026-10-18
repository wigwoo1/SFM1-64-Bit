#include "global_flex_registry.h"
#include "flex_registry_fixture.h"
#include <fstream>
#include <iostream>
static uint32_t read(std::ifstream&in){uint32_t v;if(!in.read(reinterpret_cast<char*>(&v),4))throw std::runtime_error("short oracle");return v;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream in(argv[1],std::ios::binary);RecoveredGlobalFlexRegistryCandidate registry;for(unsigned i=0;i<1200;++i){auto index=registry.registerName(flexRegistryFixtureName(i));if(index!=read(in)||registry.names().size()!=read(in))throw std::runtime_error("index/count mismatch");for(auto&name:registry.names()){auto length=read(in);if(length>10000)throw std::runtime_error("invalid oracle string");std::string text(length,'\0');if(!in.read(text.data(),length)||text!=name)throw std::runtime_error("stored name mismatch");}}if(in.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64 global flex registry:1200 native comparisons passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
