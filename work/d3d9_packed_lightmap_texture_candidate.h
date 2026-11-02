#pragma once
#include "d3d9_prepared_scene_candidate.h"
#include "source_lightmap_float_storage_candidate.h"
#include "source_lightmap_integer_storage_candidate.h"

enum class D3D9PackedLightmapFormatCandidate { Rgba16Float, Rgba16Unorm };

namespace d3d9_packed_lightmap_detail {
// Internal row copy after caller validates the locked layout. Kept separate so
// padded layouts can be checked with guarded buffers when a HAL returns tight rows.
inline void copyRows(void* destination,size_t pitch,const uint8_t* packed,size_t stride,uint32_t height){
    for(uint32_t y=0;y<height;++y)std::memcpy(static_cast<uint8_t*>(destination)+size_t(y)*pitch,packed+size_t(y)*stride,stride);
}
inline uint32_t bits(float value)noexcept{uint32_t result;static_assert(sizeof(result)==sizeof(value),"binary32 lightmap values");std::memcpy(&result,&value,sizeof(result));return result;}
inline bool finiteNonnegative(uint32_t value)noexcept{
    const auto magnitude=value&0x7fffffffu;
    return magnitude<0x7f800000u&&(!(value&0x80000000u)||magnitude==0);
}
}

// Explicit packed texture upload only. Input RGB is already in the caller's
// lightmap units. No HDR selection, normalization, exposure, fallback, material
// interpretation or device binding. Build a replacement before publishing it.
class D3D9PackedLightmapTextureCandidate {
public:
    using Hook=std::function<void(const char*)>;
private:
    // Retained device is released after its texture.
    D3D9SceneComCandidate<IDirect3DDevice9> device_;
    D3D9SceneComCandidate<IDirect3DTexture9> texture_;
    const uint32_t width_,height_;
    const D3D9PackedLightmapFormatCandidate format_;
    D3D9PackedLightmapTextureCandidate(uint32_t width,uint32_t height,D3D9PackedLightmapFormatCandidate format)
        :width_(width),height_(height),format_(format){}
    static void stage(const Hook& hook,const char* name){if(hook)hook(name);}
public:
    D3D9PackedLightmapTextureCandidate(const D3D9PackedLightmapTextureCandidate&)=delete;
    D3D9PackedLightmapTextureCandidate& operator=(const D3D9PackedLightmapTextureCandidate&)=delete;
    static std::unique_ptr<D3D9PackedLightmapTextureCandidate> prepare(
            IDirect3DDevice9* device,uint32_t width,uint32_t height,
            const std::vector<std::array<float,4>>& pixels,
            D3D9PackedLightmapFormatCandidate format,const Hook& hook={}){
        d3d9SceneRequireCandidate(device!=nullptr,"packed lightmap requires device");
        d3d9SceneRequireCandidate(format==D3D9PackedLightmapFormatCandidate::Rgba16Float||
            format==D3D9PackedLightmapFormatCandidate::Rgba16Unorm,"packed lightmap format unsupported");
        const uint64_t count=uint64_t(width)*height,rowBytes=uint64_t(width)*8;
        d3d9SceneRequireCandidate(width&&height&&count==pixels.size()&&count<=16384&&rowBytes<=65535,"packed lightmap dimensions/stride unsupported");
        for(const auto& p:pixels){
            // Integer classification also refuses negative subnormals when the
            // caller enables DAZ/FTZ; validation must not flush them to zero.
            for(size_t k=0;k<3;++k)d3d9SceneRequireCandidate(d3d9_packed_lightmap_detail::finiteNonnegative(d3d9_packed_lightmap_detail::bits(p[k])),"packed lightmap finite nonnegative RGB required");
            const auto alpha=d3d9_packed_lightmap_detail::bits(p[3]);
            d3d9SceneRequireCandidate(d3d9_packed_lightmap_detail::finiteNonnegative(alpha)&&(alpha&0x7fffffffu)<=0x3f800000u,"packed lightmap alpha outside [0,1]");
        }
        const uint32_t stride=uint32_t(rowBytes);std::vector<uint8_t> packed(size_t(count)*8);
        const bool half=format==D3D9PackedLightmapFormatCandidate::Rgba16Float;
        const bool encoded=half?sourcePackLightmapFloatCandidate(packed,pixels,{width,height,0,0,stride},SourceLightmapFloatStorageCandidate::Rgba16Float):
            sourcePackLightmapIntegerCandidate(packed,pixels,{width,height,0,0,stride});
        d3d9SceneRequireCandidate(encoded,"packed lightmap codec refused");stage(hook,"cpu-packed");
        auto next=std::unique_ptr<D3D9PackedLightmapTextureCandidate>(new D3D9PackedLightmapTextureCandidate(width,height,format));
        device->AddRef();next->device_.reset(device);stage(hook,"device-retained");
        const D3DFORMAT storage=half?D3DFMT_A16B16G16R16F:D3DFMT_A16B16G16R16;
        next->texture_.create([&](auto** p){return device->CreateTexture(width,height,1,0,storage,D3DPOOL_MANAGED,p,nullptr);});stage(hook,"texture-created");
        D3DSURFACE_DESC desc{};d3d9SceneCheckCandidate(next->texture_->GetLevelDesc(0,&desc));
        d3d9SceneRequireCandidate(next->texture_->GetLevelCount()==1&&desc.Width==width&&desc.Height==height&&desc.Format==storage&&
            desc.Pool==D3DPOOL_MANAGED&&desc.Usage==0&&next->texture_->GetType()==D3DRTYPE_TEXTURE,"packed lightmap descriptor mismatch");stage(hook,"texture-described");
        D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(next->texture_->LockRect(0,&lock,nullptr,0));bool locked=true;
        try{
            stage(hook,"texture-locked");
            d3d9SceneRequireCandidate(lock.pBits&&lock.Pitch>=int(stride),"packed lightmap row pitch unsupported");
            d3d9_packed_lightmap_detail::copyRows(lock.pBits,size_t(lock.Pitch),packed.data(),stride,height);
            stage(hook,"texture-uploaded");
            const HRESULT result=next->texture_->UnlockRect(0);locked=false;d3d9SceneCheckCandidate(result);stage(hook,"texture-unlocked");
        }catch(...){if(locked)next->texture_->UnlockRect(0);throw;}
        return next;
    }
    // Borrowed texture remains valid only while this owner is retained.
    IDirect3DTexture9* texture()const noexcept{return texture_.get();}
    uint32_t width()const noexcept{return width_;}
    uint32_t height()const noexcept{return height_;}
    D3D9PackedLightmapFormatCandidate format()const noexcept{return format_;}
};
