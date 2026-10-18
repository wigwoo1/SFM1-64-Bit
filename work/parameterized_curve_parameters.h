#pragma once
#include <cstdint>
struct RecoveredParameterizedCurve {float first,second,third;};
//10066960 parameters supplied in order to1049B7E0 ->1049B400.
inline RecoveredParameterizedCurve recoveredParameterizedCurve(uint32_t code){
 if(code==7)return {0.7699999809265137f,0,0.7699999809265137f};
 if(code==8)return {0.7699999809265137f,-1,0.7699999809265137f};
 if(code==9)return {0.7699999809265137f,1,0.7699999809265137f};
 return {0,0,0};
}
