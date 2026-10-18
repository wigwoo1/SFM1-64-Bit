#include "channel_activity.h"
#include <iostream>
#include <stdexcept>
int main(){try{unsigned checks=0;
 for(unsigned state=0;state<256;++state)for(int mode=-1;mode<6;++mode)for(unsigned cache=0;cache<2;++cache)for(unsigned source=0;source<2;++source)for(unsigned dirty=0;dirty<2;++dirty){
 bool expected=(state&2)!=0;if(!expected)switch(mode){case 3:expected=true;break;case 2:expected=cache|| (source&&dirty);break;case 1:expected=source&&dirty;break;default:break;}
 if(channelActive(uint8_t(state),mode,cache?0:0xffffffff,source!=0,dirty?0x4000:0xbfff)!=expected)throw std::runtime_error("channel activity mismatch");++checks;}
 for(unsigned raw=0;raw<256;++raw){uint8_t state=uint8_t(raw);if(forcedOperatorActive(state)!=bool(raw&2))throw std::runtime_error("shared expression activity mismatch");prepareChannelActivity(state);if(forcedOperatorActive(state))throw std::runtime_error("expression activity not consumed");if(state!=(raw&0xfd)||channelActive(state,0,0xffffffff,false,0))throw std::runtime_error("activity preparation mismatch");}
 std::cout<<"channel_preparation_checks=256 channel_activity_checks="<<checks<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
