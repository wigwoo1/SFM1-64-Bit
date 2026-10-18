#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
inline bool recoveredFlexNameEqual(const std::string&a,const std::string&b){if(a.size()!=b.size())return false;auto fold=[](unsigned char c){return c>='A'&&c<='Z'?unsigned(c+32):unsigned(c);};for(size_t i=0;i<a.size();++i)if(fold(static_cast<unsigned char>(a[i]))!=fold(static_cast<unsigned char>(b[i])))return false;return true;}
class RecoveredGlobalFlexRegistryCandidate {std::vector<std::string> names_;public:uint32_t registerName(const std::string&name){if(name.find('\0')!=std::string::npos)throw std::runtime_error("embedded NUL flex name");for(uint32_t i=0;i<names_.size();++i)if(recoveredFlexNameEqual(names_[i],name))return i;auto index=uint32_t(names_.size());if(index<512)names_.push_back(name);return index;}const std::vector<std::string>& names()const{return names_;}};
