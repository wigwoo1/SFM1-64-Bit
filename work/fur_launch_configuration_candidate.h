#pragma once
#include <optional>
#include <array>
#include <charconv>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <stdexcept>
struct FurLaunchConfigurationCandidate{uint32_t model,count;float length,width;std::array<uint8_t,3>color={220,220,220};size_t workers=0;uint64_t seed=123;bool coverage=false;};
inline std::optional<FurLaunchConfigurationCandidate>furLaunchConfigurationCandidate;
template<class T>T parseFurLaunchNumberCandidate(const char*text){T value{};auto end=text+std::strlen(text);auto parsed=std::from_chars(text,end,value);if(parsed.ec!=std::errc{}||parsed.ptr!=end)throw std::runtime_error("invalid fur numeric argument");return value;}
inline void validateFurLaunchConfigurationCandidate(const FurLaunchConfigurationCandidate&value){if(!value.count||value.count>UINT32_MAX/6||!std::isfinite(value.length)||value.length<=0||!std::isfinite(value.width)||value.width<=0||value.workers>64)throw std::runtime_error("fur requires positive count/length/width and 0..64 workers");}
