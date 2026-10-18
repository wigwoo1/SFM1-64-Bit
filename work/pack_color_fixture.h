#pragma once
#include <cstdint>
#include <cstring>
inline uint32_t packColorFixtureBits(uint32_t seed,unsigned component){const uint32_t edges[]={0,0x80000000u,0x3f800000u,0xbf800000u,0x7f800000u,0xff800000u,0x7fc01234u,0x7f801234u,1,0x80000001u};if(seed<40)return edges[(seed+component*3)%10];float v=float(int((seed*17+component*53)%1025)-256)/512.0f;uint32_t bits;std::memcpy(&bits,&v,4);if(seed>=2048){v=float((seed+component*13)%256)/255.99899291992188f;std::memcpy(&bits,&v,4);bits+=int(seed%3)-1;}return bits;}
