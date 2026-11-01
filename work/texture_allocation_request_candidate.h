#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>

struct TextureAllocationDescriptorCandidate {
    uint32_t flags=0,format=0,width=0,height=0,depth=0,mips=0,frames=0,depthMode=0;
    // Already interned/canonical CTexture name, not a filename to resolve.
    std::string name,group;
};
struct TextureAllocationBatchCandidate {
    // count, width, height, depth, format, mips, copies, API flags.
    std::array<uint32_t,8>arguments{};
    std::string name,group;
};
struct TextureAllocationDepthCandidate {
    // format, width, height, mode; native writes its result to frame slot1.
    std::array<uint32_t,4>arguments{};
    std::string name;
    uint32_t destinationFrame=1;
};
struct TextureAllocationRequestCandidate {
    uint32_t deviceValue=0;
    bool deviceActive=false;
    std::optional<TextureAllocationBatchCandidate>batch;
    std::optional<TextureAllocationDepthCandidate>depth;
};
// Bounded request planner, not an allocator. No handles, status-bit publication,
// cache insertion, upload, filesystem or implicit GPU-ready result. A successful
// plan (including an inactive device/no commands) proves none of those operations.
// Domain: measured registry0..63, positive16-bit extents/mips,1..8 frame slots,
// canonical ASCII names1..79, nonempty ASCII groups<=255. Other inputs refused.
inline bool textureAllocationRequestCandidate(const TextureAllocationDescriptorCandidate&input,
    const std::function<std::optional<uint32_t>()>&device,
    TextureAllocationRequestCandidate&output)noexcept{
    try{
        const auto v=input;const auto query=device;
        const auto half=[](uint32_t n){return n&&n<=65535;};
        const auto ascii=[](const std::string&s){for(unsigned char c:s)if(!c||c>=128)return false;return true;};
        if(!query||v.format>63||!half(v.width)||!half(v.height)||!half(v.depth)||!half(v.mips)||
            !v.frames||v.frames>8||((v.flags&0x10000u)&&v.frames<2)||
            v.name.empty()||v.name.size()>79||!ascii(v.name)||v.group.empty()||v.group.size()>255||!ascii(v.group))return false;
        for(char c:v.name)if(c=='\\'||(c>='A'&&c<='Z'))return false;
        if(v.name.size()>=4&&v.name.compare(v.name.size()-4,4,".vtf")==0)return false;
        const auto active=query();if(!active)return false;
        TextureAllocationRequestCandidate next;next.deviceValue=*active;next.deviceActive=(*active&255u)!=0;
        if(next.deviceActive){
            uint32_t flags=(v.flags&0x4000u)?1u:0u;
            const bool floating=v.format==24||v.format==27||v.format==28||v.format==29;
            if((v.flags&0x80000u)&&!floating)flags|=0x20000u;
            flags|=(v.flags&0x8000u)?2u:4u;
            if(v.flags&1u)flags|=0x1000u;
            if(v.flags&0x4000000u)flags|=0x40u;
            const uint32_t count=(v.flags&0x8000u)&&(v.flags&0x10000u)&&v.frames==2?1u:v.frames;
            const uint32_t copies=(v.flags&0x800u)&&!(v.flags&0x40000u)?6u:1u;
            const uint32_t depth=(v.flags&0x10000u)&&v.depthMode==4?1u:v.depth;
            next.batch=TextureAllocationBatchCandidate{{count,v.width,v.height,depth,v.format,v.mips,copies,flags},v.name,v.group};
            if(v.flags&0x10000u)next.depth=TextureAllocationDepthCandidate{{v.format,v.width,v.height,uint32_t(v.depthMode==4)},v.name+"_ZBuffer",1};
        }
        static_assert(std::is_nothrow_move_assignable<TextureAllocationRequestCandidate>::value,"transaction publication");
        output=std::move(next);return true;
    }catch(...){return false;}
}
