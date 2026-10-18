#pragma once
#include <cstdint>
// Instance names from native factory 0x10519150, not class identifiers.
inline const char* channelLogFactoryName(uint32_t type){switch(type){
 case 2:return "int log";
 case 3:return "float log";
 case 4:return "bool log";
 case 5:return "string log";
 case 7:return "time log";
 case 8:return "color log";
 case 9:return "vector2 log";
 case 10:return "vector3 log";
 case 11:return "vector4 log";
 case 12:return "qangle log";
 case 13:return "quaternion log";
 case 14:return "vmatrix log";
 case 16:return "int log";
 case 17:return "float log";
 case 18:return "bool log";
 case 19:return "string log";
 case 21:return "time log";
 case 22:return "color log";
 case 23:return "vector2 log";
 case 24:return "vector3 log";
 case 25:return "vector4 log";
 case 26:return "qangle log";
 case 27:return "quaternion log";
 case 28:return "vmatrix log";
 default:return nullptr;}}
