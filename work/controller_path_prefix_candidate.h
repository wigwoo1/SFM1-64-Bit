#pragma once
#include <cstring>
inline void normalizePathSlashesCandidate(char* path,unsigned char replacement){
    for(;*path;++path)if(*path=='/' || *path=='\\')*path=static_cast<char>(replacement);
}
template<class Adapter>
char* controllerPathPrefixCandidate(Adapter& a){
    char* cache=a.pathCache();
    if(!cache[0]){
        a.mainInterfaceSlot16("GAMEROOT",nullptr,cache,260);
        if(char* separator=a.findCharacter(cache,';'))*separator=0;
        normalizePathSlashesCandidate(cache,'\\');
    }
    return cache;
}
