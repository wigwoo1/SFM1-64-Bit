#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
// Offline recovery of studiorender.dll 10012E70, finite ordered-input subset.
inline float flexRampCandidate(const std::array<float,4>&targets,float value){
 if(!std::isfinite(value))throw std::runtime_error("nonfinite flex ramp input");for(size_t i=0;i<4;++i)if(!std::isfinite(targets[i])||(i&&targets[i]<targets[i-1]))throw std::runtime_error("invalid flex ramp targets");
 if(value<=targets[0]||value>=targets[3])return 0;
 if(value<targets[1])return (value-targets[0])/(targets[1]-targets[0]);
 if(value>targets[2])return (targets[3]-value)/(targets[3]-targets[2]);return 1;
}
