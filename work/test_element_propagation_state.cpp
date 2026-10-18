#include "element_propagation_state.h"
#include <iostream>
#include <stdexcept>
int main(){try{size_t checks=0;
 for(unsigned s=0;s<256;++s)for(unsigned m=0;m<256;++m)for(unsigned request=0;request<4;++request)for(unsigned callbacks=0;callbacks<2;++callbacks)for(unsigned suppress=0;suppress<2;++suppress){
  auto state=uint8_t(s);auto mask=uint32_t(m);auto result=beginElementPropagation(state,mask,request,suppress!=0,callbacks!=0);
  // Independent branch-by-branch native trace model.
  auto expectedState=uint8_t(s);auto expectedMask=uint32_t(m);bool changed=false,propagated=false,prepared=false,traverse=false;
  if(!suppress){bool active=(s&16)!=0;if(!active||(((m&3)|request)!=(m&3))){changed=true;propagated=!active;
   auto required=(m>>2)&3;bool matched=active&&(((m&3)&required)==required);
   if(callbacks&&(request&required)==required&&!matched)prepared=true;
   expectedState=uint8_t(s|16);expectedMask=m|(request&3);
   traverse=!active||(((expectedMask&3)|(request&~2u))!=(expectedMask&3));
  }}
  if(state!=expectedState||mask!=expectedMask||result.changed!=changed||result.queuePropagated!=propagated||result.queuePrepared!=prepared||result.traverseIncoming!=traverse||(changed&&result.incomingMask!=(request&~2u)))throw std::runtime_error("propagation trace mismatch");++checks;
 }
 uint8_t state=0;uint32_t mask=0;auto first=beginElementPropagation(state,mask,2,false,true);auto second=beginElementPropagation(state,mask,3,false,true);auto third=beginElementPropagation(state,mask,3,false,true);
 if(!first.queuePropagated||!first.queuePrepared||!first.traverseIncoming||second.queuePropagated||second.queuePrepared||second.traverseIncoming||third.changed||mask!=3)throw std::runtime_error("propagation escalation mismatch");
 std::cout<<"propagation_state_checks="<<checks<<" escalation_checks=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
