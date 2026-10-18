#include "graph_bookmark_storage_composition_fixture.h"
#include <iostream>
int main(){
 GraphBookmarkCopyFixture a;std::vector<std::uint32_t> source(4097);
 for(unsigned i=0;i<source.size();++i)source[i]=i*0x1234567u;
 graphBookmarkCopyCandidate(source.data(),int(source.size()),a);
 assert(a.storage.count==4097&&a.storage.cap==8192&&a.reads==4097);
 assert(std::vector<std::uint32_t>(a.storage.data.begin(),a.storage.data.begin()+4097)==source);
 auto* saved=a.storage.base0();auto growth=a.storage.growth;
 graphBookmarkCopyCandidate(nullptr,0,a);
 assert(a.storage.count==0&&a.storage.cap==8192&&a.storage.base0()==saved&&a.storage.growth==growth);
 for(int count=0;count<=4097;++count){
  graphBookmarkCopyCandidate(source.data(),count,a);
  assert(a.storage.count==count&&a.storage.cap==8192&&a.storage.base0()==saved&&a.storage.growth==growth);
  assert(std::vector<std::uint32_t>(a.storage.data.begin(),a.storage.data.begin()+count)==std::vector<std::uint32_t>(source.begin(),source.begin()+count));
 }
 std::cout<<"x64 composed bookmark replacement: 4098 sizes, rawtime preservation and retained capacity passed\n";
}
