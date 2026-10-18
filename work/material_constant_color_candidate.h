#pragma once
#include "vmt_keyvalues_candidate.h"
#include <array>
#include <sstream>
#include <cmath>
// Unlit diagnostic color only; native gamma, lighting and proxy parity
// require separate verification. Both color multipliers are retained.
inline std::array<uint8_t,4> materialConstantColorCandidate(const VmtNodeCandidate&syntax){
 std::array<float,3>result{1,1,1};
 for(auto key:{"$color","$color2"}){auto values=vmtValuesCandidate(syntax,key);if(values.empty())continue;if(values.size()!=1)throw std::runtime_error("duplicate material color");auto text=values.front();if(text.size()<2||text.front()!='['||text.back()!=']')throw std::runtime_error("unsupported material color syntax");std::istringstream input(text.substr(1,text.size()-2));std::array<float,3>color;for(auto&v:color)if(!(input>>v)||!std::isfinite(v)||v<0||v>1)throw std::runtime_error("material color range");input>>std::ws;if(!input.eof())throw std::runtime_error("material color trailing values");for(size_t n=0;n<3;++n)result[n]*=color[n];}
 return {uint8_t(std::lround(result[0]*255)),uint8_t(std::lround(result[1]*255)),uint8_t(std::lround(result[2]*255)),255};
}
