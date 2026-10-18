#pragma once
#include <cstdint>
#include <string>
//1007f3a0: caller supplies recovered base label and live/type/target callbacks.
// Capture metadata adapter owns animation-set formatting, still under recovery.
template<class Resolve,class IsAnimationSet,class AnimationLabel,class IsTransform,
 class WorkCameraParent,class TransformTarget>
std::string controlModelDisplayDecorationCandidate(const char* base,std::int32_t kind,
 Resolve resolve,IsAnimationSet isAnimationSet,AnimationLabel animationLabel,
 IsTransform isTransform,WorkCameraParent workCameraParent,TransformTarget transformTarget){
 auto* element=resolve();
 if(element&&isAnimationSet(element))return animationLabel(element,base);
 if(kind!=4)return base;
 // Original calls element lookup again for kind4 after animation type check.
 element=resolve();
 if(!element||!isTransform(element))return base;
 auto* parent=workCameraParent();
 if(!parent||transformTarget(element)!=parent)return base;
 return std::string(base)+" (work camera parent)";
}
