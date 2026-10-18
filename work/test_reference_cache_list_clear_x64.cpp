#include "reference_cache_list_clear.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
struct Node{unsigned index;Node* next;};
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream f(argv[1],std::ios::binary);for(unsigned kind=0;kind<6;++kind)for(unsigned length=0;length<=64;++length){std::array<Node,64>nodes{};for(unsigned i=0;i<length;++i){unsigned index=i*17%64;nodes[index]={index,i+1<length?&nodes[(i+1)*17%64]:nullptr};}Node* head=length?&nodes[0]:nullptr;auto* original=head;std::array<uint32_t,68>actual{},expected{};clearRecoveredReferenceCacheListCandidate(head,[](Node*n){return n->next;},[&](Node*n){if(head!=original)throw std::runtime_error("head cleared early");actual[2+actual[1]++]=n->index;n->next=nullptr;});actual[0]=head?1:0;if(!f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_cache_list_clear_cases=390"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
