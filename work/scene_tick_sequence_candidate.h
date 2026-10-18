#pragma once
#include <vector>
#include <fstream>
#include <string>
#include <charconv>
#include <optional>
#include <stdexcept>
#include <cstdint>
inline std::vector<int32_t>readSceneTickSequenceCandidate(const char*path){std::ifstream input(path);if(!input)throw std::runtime_error("scene tick sequence missing");std::vector<int32_t>ticks;std::string token;while(input>>token){int32_t tick=0;auto value=std::from_chars(token.data(),token.data()+token.size(),tick);if(value.ec!=std::errc{}||value.ptr!=token.data()+token.size()||ticks.size()>=10000)throw std::runtime_error("scene tick sequence invalid");ticks.push_back(tick);}if(!input.eof())throw std::runtime_error("scene tick sequence read failed");if(ticks.empty())throw std::runtime_error("scene tick sequence empty");return ticks;}
inline size_t scenePlaybackFrameCountCandidate(const std::vector<int32_t>&ticks,std::optional<size_t>explicitCount={}){if(explicitCount&&(!*explicitCount||*explicitCount>10000))throw std::runtime_error("playback frame count");if(!ticks.empty()){if(explicitCount&&*explicitCount!=ticks.size())throw std::runtime_error("playback frame count disagrees with scene ticks");return ticks.size();}if(!explicitCount)throw std::runtime_error("playback frame count missing");return *explicitCount;}
