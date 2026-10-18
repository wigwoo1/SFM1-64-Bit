#pragma once
#include <cstdint>
// CEngineTool slot29,101E5670; signed client-state comparison.
inline bool documentEngineGateCandidate(uint8_t toolFlag4,int32_t clientState60){
 return toolFlag4!=0&&clientState60>=2;
}
