#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
struct SourceLinePlaneSplitCandidate{bool split=false;unsigned side=0;float nearEnd=1,farStart=0;};
// Zero-extent split arithmetic. Float distances precede double intermediates
// matching the installed x87 split sequence under the tested precision mode.
inline SourceLinePlaneSplitCandidate sourceLinePlaneSplitCandidate(float from,float to){
 if(!std::isfinite(from)||!std::isfinite(to))throw std::runtime_error("nonfinite split distance");
 if(from>0&&to>0)return {false,0,1,0};if(from<0&&to<0)return {false,1,1,0};
 SourceLinePlaneSplitCandidate result;result.split=true;
 if(from<to){result.side=1;const double inverse=1.0/(double(from)-double(to));result.nearEnd=float((double(from)-.03125)*inverse);result.farStart=float((double(from)+.03125)*inverse);}
 else if(from>to){const double inverse=1.0/(double(from)-double(to));result.nearEnd=float((double(from)+.03125)*inverse);result.farStart=float((double(from)-.03125)*inverse);}
 auto clamp=[](float value){return value>1?1.0f:value<0?0.0f:value;};result.nearEnd=clamp(result.nearEnd);result.farStart=clamp(result.farStart);return result;
}
