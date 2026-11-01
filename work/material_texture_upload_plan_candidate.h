#pragma once
#include "material_texture_converted_image_candidate.h"

enum class MaterialTextureUploadCommandKindCandidate {SelectFrame,Upload2D};
struct MaterialTextureUploadCommandCandidate {
    MaterialTextureUploadCommandKindCandidate kind=MaterialTextureUploadCommandKindCandidate::SelectFrame;
    uint32_t frame=0;
    // mip, face, destination format, slice, width, height, source format, zero.
    std::array<uint32_t,8>arguments{};
    std::optional<MaterialConvertedTextureMipLeaseCandidate>pixels;
};
// Ordinary face0/slice0 upload requests and retained immutable CPU pixels.
// Frame indices are resolved by a renderer, never truncated into x86 handles.
// This is neither a GPU submission nor a completed texture/cache/material state.
class MaterialTextureUploadPlanCandidate {
    std::vector<MaterialTextureUploadCommandCandidate>operations;
    uint32_t frames=0,mips=0,source=0,destination=0;
    MaterialTextureUploadPlanCandidate()=default;
public:
    static bool prepare(const std::shared_ptr<const MaterialTextureConvertedImageCandidate>&image,
        uint32_t faceCount,uint32_t mipCount,uint32_t destinationFormat,
        std::shared_ptr<const MaterialTextureUploadPlanCandidate>&output,
        size_t maxCommands=size_t(1)<<20)noexcept{
        try{
            const auto retained=image;if(!retained||faceCount>1||destinationFormat>63)return false;
            const auto plan=retained->plan();const auto format=retained->format();
            if(!plan[10]||plan[10]>2048||plan[8]!=1||mipCount>plan[9]||
                (format!=0&&format!=3&&format!=12&&format!=13&&format!=15))return false;
            const uint64_t count=uint64_t(plan[10])*(1+uint64_t(faceCount)*mipCount);
            if(count>maxCommands)return false;
            auto next=std::shared_ptr<MaterialTextureUploadPlanCandidate>(new MaterialTextureUploadPlanCandidate);
            next->frames=plan[10];next->mips=mipCount;next->source=format;next->destination=destinationFormat;
            next->operations.reserve(size_t(count));
            for(uint32_t frame=0;frame<plan[10];++frame){
                next->operations.push_back({MaterialTextureUploadCommandKindCandidate::SelectFrame,frame,{},{}});
                if(!faceCount)continue;
                for(uint32_t mip=0;mip<mipCount;++mip){
                    const auto lease=retained->lease(frame,mip);if(!lease||!lease->data()||!lease->width()||!lease->height())return false;
                    const uint32_t w=lease->width(),h=lease->height();const uint64_t size=(format==13||format==15)?uint64_t((w+3)/4)*((h+3)/4)*(format==15?16u:8u):uint64_t(w)*h*(format==3?3u:4u);
                    if(size!=lease->size())return false;
                    next->operations.push_back({MaterialTextureUploadCommandKindCandidate::Upload2D,frame,{mip,0,destinationFormat,0,w,h,format,0},lease});
                }
            }
            output=std::move(next);return true;
        }catch(...){return false;}
    }
    uint32_t frameCount()const noexcept{return frames;}
    uint32_t mipCount()const noexcept{return mips;}
    uint32_t sourceFormat()const noexcept{return source;}
    uint32_t destinationFormat()const noexcept{return destination;}
    const std::vector<MaterialTextureUploadCommandCandidate>&commands()const noexcept{return operations;}
};
