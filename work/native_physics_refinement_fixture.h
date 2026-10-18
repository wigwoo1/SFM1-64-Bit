#pragma once
#include "native_physics_support_fixture.h"
// Persistent original support object for a complete native28F40 call. All
// geometry, metadata and scratch allocations stay alive until the call returns.
struct NativeRefinementSupportFixture {
    NativeSupportAlignedBytesFixture motion,ledge,metadata,scratch;
    const PhysicsSupportInputFixture&input;
    std::vector<uint8_t>oldMotion,oldLedge,oldMetadata;
    explicit NativeRefinementSupportFixture(NativeVphysicsLibraryFixture&image,const PhysicsSupportInputFixture&f):
        motion(0xb0+(f.context.mode==2?f.context.points.size()/4*48:0)),
        ledge(16+(f.context.mode==0?f.context.triangles.size():1)*16+((f.context.mode==1?f.context.firstVertex:0)+f.context.points.size())*16),metadata(32),scratch(2052),input(f){
        const auto&c=f.context;const size_t triangles=c.mode==0?c.triangles.size():1;const size_t prefix=c.mode==1?c.firstVertex:0;if(prefix+c.points.size()>32768)throw std::runtime_error("native refinement support extent");const uint32_t pointOffset=uint32_t(16+triangles*16);supportWriteNative(ledge.data(),0,pointOffset);supportWriteNative(ledge.data(),12,uint16_t(triangles));
        if(c.mode==0)for(size_t i=0;i<triangles;++i)supportWriteNative(ledge.data(),16+i*16,c.triangles[i]);for(size_t i=0;i<c.points.size();++i)supportWriteNative(ledge.data(),pointOffset+(prefix+i)*16,c.points[i]);
        supportWriteNative(motion.data(),0,reinterpret_cast<uintptr_t>(image.address(0xc8084)));supportWriteNative(motion.data(),0x10,ledge.data());supportWriteNative(motion.data(),0x14,scratch.data());std::memcpy(motion.data()+0x18,c.transform.data(),48);for(unsigned col=0;col<3;++col)std::memcpy(motion.data()+0x50+col*16,c.inverseBasis.data()+col*3,12);
        if(c.mode==1){metadata.data()[6]=2;supportWriteNative(metadata.data(),4,uint16_t(c.points.size()));supportWriteNative(metadata.data(),8,uint16_t(c.firstVertex));supportWriteNative(motion.data(),0xc,metadata.data());}
        else if(c.mode==0&&c.seeded){metadata.data()[6]=1;std::memcpy(metadata.data()+8,c.seeds.data(),16);supportWriteNative(motion.data(),0xc,metadata.data());}
        else if(c.mode==2){supportWriteNative(motion.data(),0xa0,uint32_t(c.points.size()/4));for(size_t group=0;group<c.points.size()/4;++group)for(unsigned lane=0;lane<4;++lane)for(unsigned axis=0;axis<3;++axis)supportWriteNative(motion.data(),0xb0+group*48+axis*16+lane*4,c.points[group*4+lane][axis]);}
        std::memcpy(scratch.data(),f.cache.entries.data(),2048);std::memcpy(scratch.data()+2048,&f.cache.generation,2);oldMotion.assign(motion.data(),motion.data()+motion.size);oldLedge.assign(ledge.data(),ledge.data()+ledge.size);oldMetadata.assign(metadata.data(),metadata.data()+metadata.size);
    }
    SourcePhysicsSupportCacheCandidate readCache()const{auto result=input.cache;std::memcpy(result.entries.data(),scratch.p+16,2048);std::memcpy(&result.generation,scratch.p+16+2048,2);return result;}
    void verify(){if(std::memcmp(motion.data(),oldMotion.data(),motion.size)||std::memcmp(ledge.data(),oldLedge.data(),ledge.size)||std::memcmp(metadata.data(),oldMetadata.data(),metadata.size))throw std::runtime_error("native refinement support mutation");for(size_t i=2050;i<2052;++i)if(scratch.data()[i])throw std::runtime_error("refinement cache tail mutation");motion.verify();ledge.verify();metadata.verify();scratch.verify();}
};
