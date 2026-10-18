#pragma once
#include <cstdint>
// Each gate is evaluated immediately before its list. Earlier callbacks may
// change later registration state or suppression; do not snapshot both gates.
template<class First,class Second,class Gate,class... Arguments>
void nativeRestoreCallbackListsCandidate(First& first,std::uint8_t& firstFlags,
 Second& second,std::uint8_t& secondFlags,Gate&& gate,Arguments... arguments){
 first.dispatchRestoreWith(gate(),firstFlags,arguments...);
 second.dispatchRestoreWith(gate(),secondFlags,arguments...);
}
