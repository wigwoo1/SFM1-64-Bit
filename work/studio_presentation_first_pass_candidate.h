#pragma once
#include <array>
#include <cstdint>
// Display mode is normalized scalar data; descriptor semantics remain external.
template<class Descriptor,class Services>std::array<uint32_t,6> studioPresentationFirstPassCandidate(unsigned adapter,Descriptor& descriptor,Services& services){
 std::array<uint32_t,6> display{};display[0]=1;
 services.displayMode(display,adapter);
 int32_t width=0,height=0;services.dimensions(display,descriptor,width,height);
 const int wide=static_cast<float>(width)/static_cast<float>(height)>=services.aspectThreshold()?1:0;
 // Reload the material service separately for each original call.
 services.material().setCondition("WIN32WIDE",wide);
 services.material().setCondition("WIN32HIDEF",height>=720?1:0);
 services.material().setCondition("WIN32LODEF",height<720?1:0);
 return display;
}
