#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_server_hull_ray_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
using Vec=std::array<float,3>;
struct Query { Vec start,end,minimum,maximum; unsigned mask; int group; bool skip; } query{};
SourceServerHullRayCandidate observed{};
std::array<uint8_t,32> output{};
uintptr_t engine=0,skip=0;
unsigned calls=0;
void engineCall(const SourceServerHullRayCandidate& ray,unsigned mask,uintptr_t entity,int group,uint8_t* result) {
    require(mask==query.mask && entity==skip && group==query.group,"trace boundary mask/filter");
    observed=ray; ++calls; for(unsigned i=0;i<32;++i) result[i]=uint8_t(3*i+1);
}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
Vec vectorAt(uintptr_t p) { Vec v; std::memcpy(v.data(),reinterpret_cast<const void*>(p),12); return v; }
void __fastcall traceEngine(void* self,void*,const uint8_t* ray,unsigned mask,const uint8_t* filter,void* result) {
    require(reinterpret_cast<uintptr_t>(self)==engine,"trace engine owner");
    const auto r=reinterpret_cast<uintptr_t>(ray),f=reinterpret_cast<uintptr_t>(filter);
    require((r&15)==0 && at<uintptr_t>(r+0x40)==0,"ray alignment/axis transform");
    require(at<uintptr_t>(f)==0x104e7390 && at<uintptr_t>(f+12)==0,"trace filter type/extra callback");
    SourceServerHullRayCandidate value;
    value.center=vectorAt(r); value.delta=vectorAt(r+0x10); value.offset=vectorAt(r+0x20); value.extents=vectorAt(r+0x30);
    value.isRay=at<uint8_t>(r+0x44)!=0; value.swept=at<uint8_t>(r+0x45)!=0;
    engineCall(value,mask,at<uintptr_t>(f+4),at<int>(f+8),static_cast<uint8_t*>(result));
}
void run() {
    std::array<uintptr_t,6> table{}; uintptr_t instance=reinterpret_cast<uintptr_t>(table.data());
    std::array<uint8_t,0x34> debugVariable{}; int ignored=0;
    engine=reinterpret_cast<uintptr_t>(&instance); skip=query.skip?reinterpret_cast<uintptr_t>(&ignored):0;
    table[5]=reinterpret_cast<uintptr_t>(traceEngine);
    at<uintptr_t>(0x10635ccc)=engine; at<uintptr_t>(0x1066f8bc)=reinterpret_cast<uintptr_t>(debugVariable.data());
    reinterpret_cast<void (__cdecl*)(const Vec*,const Vec*,const Vec*,const Vec*,unsigned,void*,int,void*)>(0x10019790)(
        &query.start,&query.end,&query.minimum,&query.maximum,query.mask,reinterpret_cast<void*>(skip),query.group,output.data());
}
#else
void run() {
    skip=query.skip?1:0;
    engineCall(sourceServerHullRayCandidate(query.start,query.end,query.minimum,query.maximum),query.mask,skip,query.group,output.data());
}
#endif

bool same(const Vec& a,const Vec& b) { return std::memcmp(a.data(),b.data(),12)==0; }
int main(int argc,char** argv) {
    try {
        require(argc==3,"expected installed server or - and semantic-record output");
        std::ofstream records(argv[2],std::ios::binary); require(bool(records),"semantic record output");
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
#endif
        const std::array<Vec,4> starts={Vec{},Vec{32,-10,100},Vec{1e8f,0,-32},Vec{-0.f,0.f,-0.f}};
        const std::array<Vec,4> deltas={Vec{},Vec{0,0,-500},Vec{1e-24f,0,0},Vec{0.125f,-1,2}};
        const std::array<std::array<Vec,2>,5> hulls={
            std::array<Vec,2>{Vec{},Vec{}},std::array<Vec,2>{Vec{-24,-24,0},Vec{24,24,82}},
            std::array<Vec,2>{Vec{-2,3,-4},Vec{4,7,12}},std::array<Vec,2>{Vec{},Vec{0.0008f,0,0}},
            std::array<Vec,2>{Vec{},Vec{0.002f,0,0}}};
        unsigned cases=0,nonRay=0,stationary=0,underflow=0;
        for(const auto& start:starts) for(const auto& delta:deltas) for(const auto& hull:hulls)
        for(unsigned mask:{1u,0x200400bu}) for(int group:{0,5}) for(bool ignore:{false,true}) {
            Vec end{}; for(unsigned i=0;i<3;++i) end[i]=start[i]+delta[i];
            query={start,end,hull[0],hull[1],mask,group,ignore}; output.fill(0xa5); calls=0; run();
            const auto expected=sourceServerHullRayCandidate(start,end,hull[0],hull[1]);
            require(calls==1 && same(observed.center,expected.center) && same(observed.delta,expected.delta) &&
                same(observed.offset,expected.offset) && same(observed.extents,expected.extents) &&
                observed.isRay==expected.isRay && observed.swept==expected.swept,"server hull Ray_t mismatch");
            for(unsigned i=0;i<output.size();++i) require(output[i]==uint8_t(3*i+1),"trace result changed after callback");
            nonRay+=!observed.isRay; stationary+=!observed.swept;
            underflow+=!observed.swept && observed.delta!=Vec{};
            std::array<uint32_t,14> row{};
            std::memcpy(row.data(),observed.center.data(),12); std::memcpy(row.data()+3,observed.delta.data(),12);
            std::memcpy(row.data()+6,observed.offset.data(),12); std::memcpy(row.data()+9,observed.extents.data(),12);
            row[12]=observed.isRay; row[13]=observed.swept;
            records.write(reinterpret_cast<const char*>(row.data()),sizeof(row)); require(bool(records),"semantic record write"); ++cases;
        }
        require(nonRay>0 && stationary>0 && underflow>0,"hull classification coverage");
#ifndef _WIN64
        image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"non_ray\":"<<nonRay<<",\"stationary\":"<<stationary<<",\"underflow\":"<<underflow<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
