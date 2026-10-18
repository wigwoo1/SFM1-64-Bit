#pragma once
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <cstdint>
inline std::vector<uint8_t> readBinaryAssetCandidate(const std::filesystem::path&path){
 std::ifstream file(path,std::ios::binary|std::ios::ate);
 if(!file)throw std::runtime_error("binary asset open failed");
 auto end=file.tellg();if(end<0)throw std::runtime_error("binary asset size unavailable");
 auto size=uint64_t(end);std::vector<uint8_t>bytes;
 if(size>bytes.max_size()||size>uint64_t(std::numeric_limits<std::streamsize>::max()))throw std::runtime_error("binary asset exceeds buffer range");
 bytes.resize(size_t(size));file.seekg(0,std::ios::beg);
 if(!file)throw std::runtime_error("binary asset seek failed");
 if(size){file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size));if(!file)throw std::runtime_error("binary asset read truncated");}
 if(file.peek()!=std::char_traits<char>::eof()||file.bad())throw std::runtime_error("binary asset size changed or read failed");
 return bytes;
}
