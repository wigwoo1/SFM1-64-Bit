#include "element_mutation_state.h"
#include <iostream>
#include <stdexcept>
int main(){try{unsigned checks=0;
 for(unsigned raw=0;raw<256;++raw){uint8_t state=uint8_t(raw);unsigned queued=0;
  uint8_t initialized=state;uint32_t initialMask=0xabcdef0f;initializeElementMutation(initialized,initialMask);if(initialized!=(raw&0xe0)||initialMask!=0xabcdef00)throw std::runtime_error("constructor mutation initialization mismatch");
  uint8_t notificationState=state;
  setElementNotifications(notificationState,true);
  if(notificationState!=(raw|4))throw std::runtime_error("notification enable mismatch");
  setElementNotifications(notificationState,false);
  if(notificationState!=(raw&0xfb))throw std::runtime_error("notification disable mismatch");
  setElementNotifications(notificationState,true);
  if(notificationState!=(raw|4))throw std::runtime_error("notification restore mismatch");
  auto queue=[&](uint32_t handle){if(handle!=0x12345678||state!=raw)throw std::runtime_error("queue ordering mismatch");++queued;};
  markElementMutation(state,0x12345678,queue);markElementMutation(state,0x12345678,queue);
  if(state!=(raw|3)||queued!=unsigned(!(raw&1)))throw std::runtime_error("owner state mismatch");
  std::vector<uint16_t> flags;for(unsigned f=0;f<65536;++f)flags.push_back(uint16_t(f));clearElementMutation(state,flags);
  if(state!=((raw|3)&0xfe))throw std::runtime_error("owner reset mismatch");for(unsigned f=0;f<65536;++f){if(flags[f]!=(f&0xff7f))throw std::runtime_error("attribute reset mismatch");++checks;}
 }
 std::cout<<"owner_state_checks=256 notification_state_checks=768 attribute_reset_checks="<<checks<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
