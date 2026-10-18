#include "controller_path_prefix_candidate.h"
#include <cassert>
#include <iostream>
#include <string>
struct Adapter{
    char cache[260]={};std::string reply="F:/SFM/game/;G:/other/";int calls=0;
    char* pathCache(){return cache;}
    void mainInterfaceSlot16(const char* id,std::nullptr_t,char* out,unsigned size){assert(std::string(id)=="GAMEROOT" && out==cache && size==260);++calls;strcpy_s(out,size,reply.c_str());}
    char* findCharacter(char* value,char ch){return std::strchr(value,ch);}
};
int main(){
    Adapter a;assert(controllerPathPrefixCandidate(a)==a.cache);assert(std::string(a.cache)=="F:\\SFM\\game\\" && a.calls==1);
    a.reply="different";controllerPathPrefixCandidate(a);assert(a.calls==1);
    Adapter b;b.reply="";controllerPathPrefixCandidate(b);controllerPathPrefixCandidate(b);assert(b.calls==2);
    Adapter c;c.reply=";other";controllerPathPrefixCandidate(c);assert(!c.cache[0]);controllerPathPrefixCandidate(c);assert(c.calls==2);
    Adapter d;strcpy_s(d.cache,"cached/path");controllerPathPrefixCandidate(d);assert(d.calls==0 && std::string(d.cache)=="cached/path");
    char path[]="a/b\\c";normalizePathSlashesCandidate(path,'/');assert(std::string(path)=="a/b/c");
    std::cout<<"x64 path prefix: first search path, slash normalization, cached bypass and empty retry passed\n";
}
