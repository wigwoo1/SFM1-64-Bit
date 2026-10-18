#pragma once
// Test-only normal-DLL support fixture; requires native_vphysics_library_fixture.h.
#include <malloc.h>
template<class T>inline T supportReadNative(const uint8_t*p,size_t at){T value;std::memcpy(&value,p+at,sizeof(value));return value;}
template<class T>inline void supportWriteNative(uint8_t*p,size_t at,const T&value){std::memcpy(p+at,&value,sizeof(value));}
inline PhysicsSupportInputFixture supportCaptureNative(const uint8_t*self,const float*direction){
    PhysicsSupportInputFixture f;auto&c=f.context;std::memcpy(f.direction.data(),direction,12);
    for(unsigned column=0;column<3;++column)std::memcpy(c.inverseBasis.data()+column*3,self+0x50+column*16,12);
    std::memcpy(c.transform.data(),self+0x18,48);const uint32_t groups=supportReadNative<uint32_t>(self,0xa0);
    if(groups){if(groups>8192)throw std::runtime_error("native packed support extent");c.mode=2;for(uint32_t group=0;group<groups;++group)for(unsigned lane=0;lane<4;++lane){std::array<float,3>p;for(unsigned axis=0;axis<3;++axis)p[axis]=supportReadNative<float>(self,0xb0+group*48+axis*16+lane*4);c.points.push_back(p);}return f;}
    const auto meta=supportReadNative<const uint8_t*>(self,0xc),ledge=supportReadNative<const uint8_t*>(self,0x10);if(!ledge)throw std::runtime_error("native support missing ledge");const auto points=ledge+supportReadNative<int32_t>(ledge,0);
    if(meta&&(meta[6]&2)){c.mode=1;c.firstVertex=supportReadNative<uint16_t>(meta,8);const uint32_t count=supportReadNative<uint16_t>(meta,4);if(!count||c.firstVertex+count>32768)throw std::runtime_error("native support span extent");for(uint32_t i=0;i<count;++i)c.points.push_back(supportReadNative<std::array<float,3>>(points,size_t(c.firstVertex+i)*16));return f;}
    c.mode=0;c.seeded=meta&&(meta[6]&1);if(c.seeded)std::memcpy(c.seeds.data(),meta+8,16);const uint32_t count=supportReadNative<uint16_t>(ledge,12);if(!count||count>32767)throw std::runtime_error("native support triangle extent");uint32_t last=0;
    for(uint32_t i=0;i<count;++i){const auto t=supportReadNative<std::array<uint32_t,4>>(ledge,16+i*16);c.triangles.push_back(t);for(unsigned corner=1;corner<4;++corner)last=(std::max)(last,t[corner]&0xffff);}
    if(last>=32768)throw std::runtime_error("native support vertex extent");for(uint32_t i=0;i<=last;++i)c.points.push_back(supportReadNative<std::array<float,3>>(points,i*16));const auto cache=supportReadNative<const uint8_t*>(self,0x14);if(!cache)throw std::runtime_error("native support missing cache");std::memcpy(f.cache.entries.data(),cache,2048);std::memcpy(&f.cache.generation,cache+2048,2);return f;
}
struct NativeSupportAlignedBytesFixture {
    uint8_t*p=nullptr;size_t size;
    explicit NativeSupportAlignedBytesFixture(size_t bytes):size(bytes){p=static_cast<uint8_t*>(_aligned_malloc(bytes+32,16));if(!p)throw std::bad_alloc();std::memset(p,0xa7,bytes+32);std::memset(p+16,0,bytes);}
    uint8_t*data(){return p+16;}
    void verify(){for(size_t i=0;i<16;++i)if(p[i]!=0xa7||p[16+size+i]!=0xa7)throw std::runtime_error("native support canary");}
    ~NativeSupportAlignedBytesFixture(){_aligned_free(p);}
};
inline SourcePhysicsSupportResultCandidate supportInvokeNative(NativeVphysicsLibraryFixture&image,const PhysicsSupportInputFixture&f,SourcePhysicsSupportCacheCandidate&cache){
    const auto&c=f.context;const size_t triangles=c.mode==0?c.triangles.size():1;const size_t prefix=c.mode==1?c.firstVertex:0;if(prefix+c.points.size()>32768)throw std::runtime_error("native support span overflow");
    NativeSupportAlignedBytesFixture motion(0xb0+(c.mode==2?c.points.size()/4*48:0)),ledge(16+triangles*16+(prefix+c.points.size())*16),metadata(32),scratch(2052);
    const uint32_t pointOffset=uint32_t(16+triangles*16);supportWriteNative(ledge.data(),0,pointOffset);supportWriteNative(ledge.data(),12,uint16_t(triangles));
    if(c.mode==0)for(size_t i=0;i<triangles;++i)supportWriteNative(ledge.data(),16+i*16,c.triangles[i]);
    for(size_t i=0;i<c.points.size();++i)supportWriteNative(ledge.data(),pointOffset+(prefix+i)*16,c.points[i]);
    supportWriteNative(motion.data(),0x10,ledge.data());supportWriteNative(motion.data(),0x14,scratch.data());std::memcpy(motion.data()+0x18,c.transform.data(),48);for(unsigned col=0;col<3;++col)std::memcpy(motion.data()+0x50+col*16,c.inverseBasis.data()+col*3,12);
    if(c.mode==1){metadata.data()[6]=2;supportWriteNative(metadata.data(),4,uint16_t(c.points.size()));supportWriteNative(metadata.data(),8,uint16_t(c.firstVertex));supportWriteNative(motion.data(),0xc,metadata.data());}
    else if(c.mode==0&&c.seeded){metadata.data()[6]=1;std::memcpy(metadata.data()+8,c.seeds.data(),16);supportWriteNative(motion.data(),0xc,metadata.data());}
    else if(c.mode==2){supportWriteNative(motion.data(),0xa0,uint32_t(c.points.size()/4));for(size_t group=0;group<c.points.size()/4;++group)for(unsigned lane=0;lane<4;++lane)for(unsigned axis=0;axis<3;++axis)supportWriteNative(motion.data(),0xb0+group*48+axis*16+lane*4,c.points[group*4+lane][axis]);}
    std::memcpy(scratch.data(),cache.entries.data(),2048);std::memcpy(scratch.data()+2048,&cache.generation,2);
    std::vector<uint8_t>beforeMotion(motion.data(),motion.data()+motion.size),beforeLedge(ledge.data(),ledge.data()+ledge.size),beforeMeta(metadata.data(),metadata.data()+metadata.size);std::array<float,5>point{123456,0,0,0,123456};using Function=uint32_t(__thiscall*)(void*,const float*,float*);SourcePhysicsSupportResultCandidate result;
    result.feature=reinterpret_cast<Function>(image.address(0x26d40))(motion.data(),f.direction.data(),point.data()+1);std::memcpy(result.point.data(),point.data()+1,12);
    if(point.front()!=123456||point.back()!=123456)throw std::runtime_error("native support output canary");if(std::memcmp(motion.data(),beforeMotion.data(),motion.size)||std::memcmp(ledge.data(),beforeLedge.data(),ledge.size)||std::memcmp(metadata.data(),beforeMeta.data(),metadata.size))throw std::runtime_error("native support static input changed");
    std::memcpy(cache.entries.data(),scratch.data(),2048);std::memcpy(&cache.generation,scratch.data()+2048,2);for(size_t i=2050;i<2052;++i)if(scratch.data()[i])throw std::runtime_error("native cache padding changed");motion.verify();ledge.verify();metadata.verify();scratch.verify();return result;
}
