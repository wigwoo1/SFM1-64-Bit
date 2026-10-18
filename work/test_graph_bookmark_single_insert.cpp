#include "graph_bookmark_single_insert_candidate.h"
#include "graph_bookmark_storage_composition_fixture.h"
#include <iostream>
int main(){
 unsigned cases=0;
 for(int n=0;n<=256;++n)for(int position=0;position<=n;++position){
  GraphBookmarkStorageFixture a;a.data.resize(n);a.cap=n;a.count=n;
  for(int i=0;i<n;++i)a.data[i]=std::uint32_t(i)*0x9e3779b9u;
  auto expected=a.data;const std::uint32_t value=0x80000000u+std::uint32_t(n);
  expected.insert(expected.begin()+position,value);
  assert(graphBookmarkSingleInsertCandidate(position,&value,a)==position);
  assert(a.count==n+1&&std::vector<std::uint32_t>(a.data.begin(),a.data.begin()+a.count)==expected);++cases;
 }
 assert(cases==33153);std::cout<<"x64 single bookmark insertion: 33153 positions with composed growth/shift passed\n";
}
