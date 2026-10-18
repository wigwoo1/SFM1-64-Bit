#pragma once
#include <array>
#include <cstdint>
#include <string_view>
// Recovered label/check state only; no interpolation or key-editing implementation.
struct GraphEditorKeyModeStateCandidate {
 int32_t mode=0;
 static constexpr std::array<std::string_view,4> labels{"Curve Fit","Linear","Spline","Stepped"};
 void setMode(int32_t value){mode=value;} // Original setter stores without a range guard.
 std::array<bool,4> checked()const{return {mode==0,mode==1,mode==2,mode==3};}
 std::string_view label()const{return uint32_t(mode)<labels.size()?labels[size_t(mode)]:std::string_view{};}
};
