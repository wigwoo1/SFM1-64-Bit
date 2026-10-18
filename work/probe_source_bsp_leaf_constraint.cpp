#include "source_bsp_point_leaf_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef SFM64_NATIVE_LEAF_CONSTRAINT
#include <windows.h>
#include "native_bsp_leaf_constraint_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("usage: probe map queries.bin results.bin");
 std::ifstream map(argv[1],std::ios::binary),input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);if(!map||!input||!output)throw std::runtime_error("test file open failed");
 std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});SourceBspPointLeafCandidate tree(b);
#ifdef SFM64_NATIVE_LEAF_CONSTRAINT
 auto u32=[&](size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};
 const size_t po=u32(8+16),no=u32(8+5*16);
 struct Node{const uint8_t*plane;int32_t children[2];};static_assert(sizeof(Node)==12,"original node stride");
 std::vector<Node>nodes(tree.nodeCount());
 for(size_t i=0;i<nodes.size();++i){nodes[i].plane=b.data()+po+size_t(u32(no+i*32))*20;std::memcpy(nodes[i].children,b.data()+no+i*32+4,8);}
 const Node*head=nodes.data();
 auto code=VirtualAlloc(nullptr,sizeof(nativeLeafConstraintBytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)throw std::runtime_error("oracle allocation failed");std::memcpy(code,nativeLeafConstraintBytes,sizeof(nativeLeafConstraintBytes));DWORD old=0;if(!VirtualProtect(code,sizeof(nativeLeafConstraintBytes),PAGE_EXECUTE_READ,&old))throw std::runtime_error("oracle protection failed");FlushInstructionCache(GetCurrentProcess(),code,sizeof(nativeLeafConstraintBytes));
 using Function=void(__cdecl*)(const Node**,const float*,int32_t,float,float*);auto native=reinterpret_cast<Function>(code);
#endif
 std::array<float,7>query{};size_t count=0;
 while(input.read(reinterpret_cast<char*>(query.data()),sizeof(query))){
  const std::array<float,3>start{query[0],query[1],query[2]};std::array<float,3>target{query[4],query[5],query[6]};
#ifdef SFM64_NATIVE_LEAF_CONSTRAINT
  const auto before=query;native(&head,start.data(),tree.worldRoot(),query[3],target.data());if(query!=before)throw std::runtime_error("original query changed");
#else
  target=tree.constrainToStartLeaf(start,query[3],target);
#endif
  output.write(reinterpret_cast<const char*>(target.data()),12);++count;
 }
 if(!input.eof()||input.gcount()!=0||!count)throw std::runtime_error("invalid query length");output.flush();if(!output)throw std::runtime_error("output failed");
#ifdef SFM64_NATIVE_LEAF_CONSTRAINT
 if(std::memcmp(code,nativeLeafConstraintBytes,sizeof(nativeLeafConstraintBytes)))throw std::runtime_error("original code modified");VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
