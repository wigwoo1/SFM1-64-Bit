#pragma once
#include "vmt_keyvalues_candidate.h"
#include <array>
#include <cmath>
#include <locale>
#include <sstream>
struct MaterialSelfIllumCandidate {bool enabled=false;std::array<float,3>tint{1,1,1};std::string mask;};
// Retain authored HDR tint; gamma/mask channel/composition need renderer parity.
inline MaterialSelfIllumCandidate materialSelfIllumCandidate(const VmtNodeCandidate&syntax){
 MaterialSelfIllumCandidate result;
 auto single=[&](const char*key){auto values=vmtValuesCandidate(syntax,key);if(values.size()>1)throw std::runtime_error(std::string("duplicate ")+key);return values;};
 auto flag=single("$selfillum");if(!flag.empty()){if(flag[0]!="0"&&flag[0]!="1")throw std::runtime_error("unsupported selfillum flag");result.enabled=flag[0]=="1";}
 auto masks=single("$selfillummask");if(!masks.empty())result.mask=masks[0];
 auto tint=single("$selfillumtint");if(!tint.empty()){
  const auto&text=tint[0];if(text.size()<2||text.front()!='['||text.back()!=']')throw std::runtime_error("unsupported selfillum tint syntax");
  std::istringstream input(text.substr(1,text.size()-2));input.imbue(std::locale::classic());
  for(auto&value:result.tint)if(!(input>>value)||!std::isfinite(value)||value<0)throw std::runtime_error("selfillum tint range");
  input>>std::ws;if(!input.eof())throw std::runtime_error("selfillum tint trailing values");
 }
 return result;
}
