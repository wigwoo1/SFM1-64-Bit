#pragma once
#include <algorithm>
// Graph declarations must be included first. Exact-key sampling only.
struct KeyTrack {
 const Attribute* times=nullptr;
 const Attribute* values=nullptr;
 explicit KeyTrack(const Graph&g,const Element&e){
  for(auto&a:e.attributes){auto&n=g.strings.at(a.name);if(n=="times")times=&a;if(n=="values")values=&a;if(n=="compressed"&&!a.values.empty()&&!a.values[0].wire.empty())throw std::runtime_error("compressed animation unsupported");}
  if(!times||!values||times->type!=21||times->values.size()!=values->values.size())throw std::runtime_error("invalid animation key arrays");
  for(size_t i=1;i<times->values.size();++i)if(wireInt(times->values[i])<wireInt(times->values[i-1]))throw std::runtime_error("unsorted animation times");
 }
 const Value* exact(int32_t tick)const{
  auto i=std::lower_bound(times->values.begin(),times->values.end(),tick,[](const Value&v,int32_t t){return wireInt(v)<t;});
  if(i==times->values.end()||wireInt(*i)!=tick)return nullptr;
  return &values->values[static_cast<size_t>(i-times->values.begin())];
 }
};
