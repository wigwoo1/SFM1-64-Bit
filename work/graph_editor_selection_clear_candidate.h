#pragma once
#include <cstdint>
template<class Adapter>void graphEditorSelectionClearCandidate(Adapter& a){
 for(int i=a.count0c()-1;i>=0;--i){
  const std::uint32_t handle=a.handleAt(i);
  if(a.modelServicePresent())a.releaseElementHandle(handle,0);
 }
 a.setByte28(0);a.setCount0c(0);
}
