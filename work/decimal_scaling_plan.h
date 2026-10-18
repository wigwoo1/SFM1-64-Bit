#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
// Offline binary table addresses for recovery; not pointers in x64 runtime.
inline std::vector<uint32_t> decimalScalingPlan(int exponent){
 if(exponent>5200||exponent< -5200)throw std::runtime_error("native exponent range exceeded");
 uint32_t base=exponent<0?0x10F229E8u:0x10F22888u;
 unsigned magnitude=static_cast<unsigned>(exponent<0?-exponent:exponent);
 std::vector<uint32_t> plan;
 for(unsigned group=0;magnitude; ++group,magnitude>>=3){unsigned digit=magnitude&7u;if(digit)plan.push_back(base+group*0x54u+(digit-1)*12u);}
 return plan;
}
