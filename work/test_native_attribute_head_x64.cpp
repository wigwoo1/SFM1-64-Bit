#include "attribute_creation_order.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
struct Node{Node*next=nullptr;uint32_t id=0;};
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;for(unsigned seq=0;seq<128;++seq){std::array<Node,8>nodes{};Node*head=nullptr;uint8_t state=uint8_t((seq*2)|1);for(unsigned step=0;step<8;++step){nodes[step].id=step+1;prependRecoveredCreatedAttributeCandidate(head,&nodes[step],state);std::array<uint32_t,12>actual{},expected{};for(auto*node=head;node;node=node->next)actual[1+actual[0]++]=node->id;actual[10]=state;f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native attribute insertion mismatch");++cases;}}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_attribute_head_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
