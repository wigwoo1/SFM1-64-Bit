#include "global_flex_binding_lifecycle.h"
#include <optional>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
static uint32_t read(std::ifstream&in){uint32_t v;if(!in.read(reinterpret_cast<char*>(&v),4))throw std::runtime_error("short oracle");return v;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream in(argv[1],std::ios::binary);for(uint32_t index:{0xffffffffu,0x80000000u,0u,7u})for(uint32_t destination:{0xffffffffu,0x1234u})for(unsigned model=0;model<2;++model)for(unsigned attribute=0;attribute<2;++attribute){RecoveredFlexBindingCache cache{destination,index};std::vector<uint32_t>events;recoveredLazyFlexBindingCandidate(cache,[&]{events.push_back(1);return 7u;},[&]()->std::optional<uint32_t>{events.push_back(2);if(!model)return {};events.push_back(3);if(!attribute)return {};events.push_back(4);return 0x4567u;});if(cache.destination!=read(in)||cache.index!=read(in)||events.size()!=read(in))throw std::runtime_error("cache mismatch");for(auto e:events)if(e!=read(in))throw std::runtime_error("event mismatch");}if(in.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64 lazy flex binding:32 native comparisons passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
