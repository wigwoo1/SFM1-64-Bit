#pragma once
#include <cstdint>
#include <optional>
// Data role8 branch1007f8c9. Palette color supplied by live UI preference bridge.
// darkFactor0 means no QColor::dark call,135 means original dark(135).
struct ControlSelectionBackgroundCandidate{std::int32_t brushStyle=1,darkFactor=0;};
inline std::optional<ControlSelectionBackgroundCandidate>
controlSelectionBackgroundCandidate(std::uint32_t state){
 if(state==2)return ControlSelectionBackgroundCandidate{1,0};
 if(state==3)return ControlSelectionBackgroundCandidate{1,135};
 return std::nullopt;
}
