#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
struct ModelMaterialMetadataCandidate {
 std::vector<std::string>names,directories;
 std::vector<std::vector<uint16_t>>skins;
 const std::string&resolve(size_t family,int32_t slot)const {
  if(family>=skins.size()||slot<0||size_t(slot)>=skins[family].size())throw std::runtime_error("material family/slot");auto index=skins[family][size_t(slot)];if(index>=names.size())throw std::runtime_error("material texture index");return names[index];
 }
};
inline ModelMaterialMetadataCandidate readModelMaterialMetadataCandidate(const std::vector<uint8_t>&mdl){
 auto range=[&](uint64_t o,uint64_t n){if(o>mdl.size()||n>mdl.size()-o)throw std::runtime_error("MDL material span");};
 auto word=[&](size_t o){range(o,4);uint32_t v;std::memcpy(&v,mdl.data()+o,4);return v;};
 auto nonnegative=[&](size_t o){auto v=word(o);if(v>INT32_MAX)throw std::runtime_error("negative MDL material field");return v;};
 auto string=[&](int64_t o){if(o<0)throw std::runtime_error("material name offset");range(uint64_t(o),1);size_t end=size_t(o);while(end<mdl.size()&&mdl[end])++end;if(end==mdl.size())throw std::runtime_error("material string termination");return std::string(reinterpret_cast<const char*>(mdl.data()+size_t(o)),end-size_t(o));};
 range(0,232);if(word(0)!=0x54534449||word(4)!=49)throw std::runtime_error("MDL49 materials required");ModelMaterialMetadataCandidate result;
 auto count=nonnegative(204),start=nonnegative(208);range(start,uint64_t(count)*64);for(uint32_t n=0;n<count;++n){size_t base=size_t(start)+size_t(n)*64;auto raw=word(base);int32_t relative;std::memcpy(&relative,&raw,4);result.names.push_back(string(int64_t(base)+relative));}
 auto directories=nonnegative(212),directoryStart=nonnegative(216);range(directoryStart,uint64_t(directories)*4);for(uint32_t n=0;n<directories;++n)result.directories.push_back(string(nonnegative(size_t(directoryStart)+size_t(n)*4)));
 auto slots=nonnegative(220),families=nonnegative(224),skinStart=nonnegative(228);range(skinStart,uint64_t(slots)*families*2);result.skins.resize(families);for(uint32_t f=0;f<families;++f){auto&skin=result.skins[f];skin.reserve(slots);for(uint32_t s=0;s<slots;++s){uint16_t index;std::memcpy(&index,mdl.data()+skinStart+(size_t(f)*slots+s)*2,2);if(index>INT16_MAX||index>=result.names.size())throw std::runtime_error("skin texture index");skin.push_back(index);}}return result;
}
