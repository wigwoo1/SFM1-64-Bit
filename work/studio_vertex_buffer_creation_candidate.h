#pragma once
#include <cstdint>
#include <string>
// Logical pointer-wide object; counter pointers borrow the profile service lifetime.
struct StudioVertexBufferCreationCandidate{
 std::uintptr_t resource=0;std::uint32_t bytes=0,zero=0,stride=0,count=0,flags=0,sentinel=0;
 std::uint32_t* frameCounter=nullptr;std::uint32_t* globalCounter=nullptr;
 std::uint32_t zero24=0,zero28=0,tail=0;
};
template<class Ops>void studioVertexBufferCreationCandidate(StudioVertexBufferCreationCandidate& state,std::uint32_t count,std::uint32_t stride,const char* suffix,std::uint32_t fvf,std::uint32_t managed,std::uint32_t dynamic,std::uintptr_t opaqueA,std::uintptr_t opaqueB,Ops& ops){
 state.count=count;state.stride=stride;state.bytes=count*stride;state.flags=(state.flags&0xffffff00u)|((std::uint8_t(state.flags)&0xfcu)|4u|(std::uint8_t(dynamic)&1u));state.resource=0;state.zero=0;state.sentinel=0xffffffffu;state.zero24=state.zero28=0;
 if(!(std::uint8_t(state.flags)&1u)){auto name=(std::string("TexGroup_global_")+suffix).substr(0,255);state.globalCounter=ops.findCounter(name.c_str(),2);name=(std::string("TexGroup_frame_")+suffix).substr(0,255);state.frameCounter=ops.findCounter(name.c_str(),3);}else state.globalCounter=ops.findCounter("TexGroup_global_Dynamic Verts",2);
 const auto flags=std::uint8_t(state.flags);const std::uint32_t usage=8u|((flags&1u)?0x200u:0u)|(std::uint8_t(managed)?0x10u:0u),pool=(flags&1u)?0u:1u;auto bytes=state.bytes;
 ops.flush();auto result=ops.create(ops.device(),bytes,usage,fvf,pool,state.resource);
 if(result==0x8876017cu||result==0x8007000eu){if(ops.device()){ops.flush();ops.cleanup(ops.device());}bytes=state.bytes;ops.flush();ops.create(ops.device(),bytes,usage,fvf,pool,state.resource);}
 if(!(std::uint8_t(state.flags)&1u))*state.globalCounter+=state.bytes;
 ops.registerBuffer(state.resource,std::uint8_t(state.flags)&1u,state.bytes,state.count,opaqueA,opaqueB);
}
