#pragma once
#include "graph_bookmark_equality_candidate.h"
// Temporary allocation/growth/free remain explicit adapter operations, allowing
// native storage size without assuming the original x86 vector layout.
template<class Adapter>void graphBookmarkUpdateCandidate(Adapter& a){
 a.beginProfile();
 if(auto* reference=a.findGraphCurveReference())if(auto* log=a.logFromReference104cd3e0(reference)){
  const auto components=a.componentCountSlot62(log);std::uint32_t mask=1;
  for(int component=0;component<components;++component){
   if(auto* element=a.resolveLiveHandle5c()){
    const auto type=a.elementTypeSlot36(element);
    if(type==10?bool(a.componentMask70()&mask):component==0){
     const auto count=a.sourceCount84(component);
     auto&& view=a.bookmarkView(log);
     if(!graphBookmarkEqualityCandidate(count,component,view)){
      auto temporary=a.allocateTemporaryTimes(count);
      for(int i=0;i<count;++i)a.appendTemporaryTime(temporary,a.sourceMappedTime(component,i));
      a.replaceBookmarks10517340(log,component,temporary);a.releaseTemporaryTimes(temporary);
     }
    }
   }
   mask=(mask<<1)|(mask>>31);
  }
 }
 a.endProfile();
}
