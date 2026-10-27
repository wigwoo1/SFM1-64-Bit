#pragma once
#include <string>
#include <string_view>
// Original FindMaterial77 cached branch: lowercase, strip last extension, then
// normalize slashes. This is a lookup key, NOT filesystem path validation.
// Locale-dependent non-ASCII case mapping is outside the proven default branch.
inline bool materialFindCacheKeyCandidate(std::string_view input,std::string&output){
 if(input.size()>4095)return false;
 std::string key(input.substr(0,input.find('\0')));
 for(char&c:key)if(c>='A'&&c<='Z')c=char(c+32);
 if(key.size()>1){size_t at=key.size()-1;while(at>0&&key[at]!='.'&&key[at]!='/'&&key[at]!='\\')--at;if(at>0&&key[at]=='.')key.resize(at);}
 for(char&c:key)if(c=='\\')c='/';
 output.swap(key);return true;
}
