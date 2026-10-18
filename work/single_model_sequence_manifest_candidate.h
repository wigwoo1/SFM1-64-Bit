#pragma once
#include <sstream>
inline void checkSingleModelSequenceManifestCandidate(const std::filesystem::path&directory,const std::filesystem::path&mount,const std::string&base){
 std::ifstream file(directory/"manifest.txt");if(!file)throw std::runtime_error("sequence manifest missing");std::string line;size_t models=0;
 while(std::getline(file,line)){if(line.rfind("model-",0)!=0)continue;++models;std::istringstream fields(line);std::string label,field,asset;fields>>label;if(label!="model-0")throw std::runtime_error("sequence supports only one model");while(fields>>field)if(field.rfind("asset=",0)==0){if(!asset.empty())throw std::runtime_error("duplicate sequence asset");asset=field.substr(6);}if(asset.empty())throw std::runtime_error("sequence asset missing");for(char&c:asset)if(c=='\\')c='/';std::filesystem::path relative(asset);if(relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||asset.find(':')!=std::string::npos)throw std::runtime_error("sequence asset path");for(auto&part:relative)if(part=="..")throw std::runtime_error("sequence asset traversal");if(std::filesystem::weakly_canonical(mount/relative)!=std::filesystem::weakly_canonical(base+".mdl"))throw std::runtime_error("sequence model changed; renderer rebuild required");}
 if(models!=1)throw std::runtime_error("sequence requires exactly one model");
}
