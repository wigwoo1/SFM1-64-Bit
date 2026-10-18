#include "controller_autosave_cleanup_candidate.h"
#include <string>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>
struct Adapter{
    std::vector<std::uint8_t> bytes;std::vector<std::uint32_t> requests;
    std::uint8_t error=0,flags=1;std::uint32_t get=0,offset=0,max=0,cap=0;
    bool refill=true;std::uint8_t refillError=0;
    explicit Adapter(std::vector<std::uint8_t> b):bytes(b),max(static_cast<std::uint32_t>(b.size())),cap(max){}
    std::uint32_t grow=0;auto growSize(){return grow;}
    auto errorByte(){return error;}void setErrorByte(std::uint8_t e){error=e;}
    void setGetPosition(std::uint32_t v){get=v;}
    void copyOutput(std::uint8_t* dst,const std::uint8_t* src,std::uint32_t n){std::memmove(dst,src,n);}
    auto flagsByte(){return flags;}auto getPosition(){return get;}auto maxGet(){return max;}
    auto bufferOffset(){return offset;}auto capacity(){return cap;}
    bool readOverflow(std::uint32_t n){requests.push_back(n);cap=max;error|=refillError;return refill;}
    auto sourceByte(std::uint32_t i){assert(i<bytes.size());return bytes[i];}
    auto sourceData(){return bytes.data();}
};
struct Services{
    std::vector<std::string> events;bool success=true;std::uint8_t initialError=0;std::uint32_t grow=0;
    std::vector<std::uint8_t> contents={32,9,'p','r','o','j','e','c','t','.','d','m','x',10};
    const char* locatorPathService(){events.push_back("path");return "fixture";}
    void formatLocatorPath(std::uint8_t* out,unsigned capacity,const char* path,const char* suffix){assert(std::string(suffix)==".sfm_autosave.loc");assert(capacity==260);events.push_back("format");sprintf_s(reinterpret_cast<char*>(out),capacity,"%s%s",path,suffix);}
    Adapter createBuffer(unsigned initial,unsigned capacity,unsigned flags){assert(initial==0 && capacity==260 && flags==1);events.push_back("construct");Adapter b(contents);b.error=initialError;b.grow=grow;return b;}
    bool readSubinterfaceSlot14(const std::uint8_t* p,std::nullptr_t,Adapter&,unsigned capacity,unsigned x,unsigned y){assert(std::string(reinterpret_cast<const char*>(p))=="fixture.sfm_autosave.loc" && capacity==260 && x==0 && y==0);events.push_back("read14");return success;}
    void mainInterfaceSlot18(const std::uint8_t* p,std::nullptr_t){events.push_back("slot18:"+std::string(reinterpret_cast<const char*>(p)));}
    void freeBufferStorage(const std::uint8_t* p){assert(p);events.push_back("free");}
};
int main(){
    Services a;cleanupControllerAutosaveCandidate(a);assert((a.events==std::vector<std::string>{"path","format","construct","read14","slot18:fixture.sfm_autosave.loc","slot18:project.dmx","free"}));
    Services b;b.success=false;cleanupControllerAutosaveCandidate(b);assert((b.events==std::vector<std::string>{"path","format","construct","read14","free"}));
    Services c;c.initialError=1;cleanupControllerAutosaveCandidate(c);assert(c.events[5]=="slot18:" && c.events.back()=="free");
    Services d;d.grow=0xffffffff;cleanupControllerAutosaveCandidate(d);assert(d.events.size()==6 && d.events.back()=="slot18:project.dmx");
    std::cout<<"x64 controller cleanup composition: service order, parsed target, read failure, extraction error and external buffer passed; no filesystem operations\n";
}
