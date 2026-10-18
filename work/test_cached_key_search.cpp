#include "cached_key_search.h"
#include <iostream>
#include <stdexcept>
int main(){try{CachedKeySearch s;std::vector<int32_t> t={10,20,20,40};
 for(auto p:std::vector<std::pair<int32_t,int32_t>>{{9,-1},{10,0},{20,2},{30,2},{50,3},{19,0},{9,-1},{20,2}})if(s.find(t,p.first)!=p.second)throw std::runtime_error("cached lookup mismatch");
 CachedKeySearch empty;if(empty.find({},0)!=-1)throw std::runtime_error("empty lookup mismatch");std::cout<<"9 cached key cases passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
