#pragma once
#include "vmt_keyvalues_candidate.h"
#include <filesystem>
struct GameInfoMountPlanCandidate {
 std::vector<std::filesystem::path> gamePaths;
 std::vector<std::filesystem::path> gameBinPaths;
 std::filesystem::path modPath;
 std::filesystem::path defaultWritePath;
};
// Resolves the derived directory paths documented by this installation's
// GameInfo comments. Steam caches, includes, dynamic mounts and archives still
// require separate native-resolution work.
inline GameInfoMountPlanCandidate gameinfoMountPlanCandidate(const std::string&source,const std::filesystem::path&gameinfoDirectory,const std::filesystem::path&gameDirectory,const std::string&language="english"){
 if(!language.empty())for(unsigned char c:language)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))throw std::runtime_error("invalid GameInfo language suffix");
 auto root=parseVmtCandidate(source);if(vmtAsciiLowerCandidate(root.key)!="gameinfo")throw std::runtime_error("GameInfo root required");
 auto block=[](const VmtNodeCandidate&parent,const char*name)->const VmtNodeCandidate&{const VmtNodeCandidate*found=nullptr;for(auto&child:parent.children)if(vmtAsciiLowerCandidate(child.key)==vmtAsciiLowerCandidate(name)){if(found||!child.block)throw std::runtime_error("ambiguous GameInfo block");found=&child;}if(!found)throw std::runtime_error("missing GameInfo block");return *found;};
 auto&paths=block(block(root,"FileSystem"),"SearchPaths");GameInfoMountPlanCandidate plan;
 auto resolvePath=[&](const std::string&input,const char*kind){if(input.empty())throw std::runtime_error(std::string("invalid ")+kind+" search path");auto value=input;for(char&c:value)if(c=='\\')c='/';std::filesystem::path base=gameDirectory;
  const std::string token="|gameinfo_path|";if(value.compare(0,token.size(),token)==0){base=gameinfoDirectory;value.erase(0,token.size());}
  if(value.find('|')!=std::string::npos||value.find('*')!=std::string::npos)throw std::runtime_error(std::string("unsupported ")+kind+" search path token");std::filesystem::path relative(value);if(relative.is_absolute()||relative.has_root_name()||relative.has_root_directory()||value.find(':')!=std::string::npos)throw std::runtime_error(std::string("unsupported absolute ")+kind+" search path");for(auto&part:relative)if(part=="..")throw std::runtime_error(std::string("unsupported parent ")+kind+" search path");auto mount=(base/relative).lexically_normal();if(mount.filename().empty())mount=mount.parent_path();return mount;};
 for(auto&entry:paths.children){const auto key=vmtAsciiLowerCandidate(entry.key);if(key!="game"&&key!="gamebin")continue;if(entry.block)throw std::runtime_error("search path entry must be a string");if(key=="gamebin"){plan.gameBinPaths.push_back(resolvePath(entry.value,"GameBin"));continue;}auto mount=resolvePath(entry.value,"Game");
  // Source adds a language-specific Game path directly in front of each
  // declared Game path. Keep it in the plan even when absent on disk: the
  // filesystem provider will naturally fall through to the base path.
  if(plan.gamePaths.empty()){plan.modPath=mount;plan.defaultWritePath=mount;}
  if(!language.empty()){auto localized=mount;localized+="_"+language;plan.gamePaths.push_back(std::move(localized));}plan.gamePaths.push_back(mount);plan.gameBinPaths.push_back(mount/"bin");
 }
 if(plan.gamePaths.empty())throw std::runtime_error("Game search paths empty");return plan;
}
inline std::vector<std::filesystem::path> gameinfoMountsCandidate(const std::string&source,const std::filesystem::path&gameinfoDirectory,const std::filesystem::path&gameDirectory,const std::string&language="english"){
 return gameinfoMountPlanCandidate(source,gameinfoDirectory,gameDirectory,language).gamePaths;
}
