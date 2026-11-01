#pragma once
#include "material_texture_cpu_image_candidate.h"
#include "texture_format_policy_candidate.h"
#include <limits>

// A lease owns either converted immutable storage or the original validated
// file mip lease. Releasing the image/provider/material cannot invalidate it.
class MaterialConvertedTextureMipLeaseCandidate {
    std::shared_ptr<const std::vector<uint8_t>>pixels;
    std::optional<MaterialTextureMipLeaseCandidate>original;
    size_t offset=0,length=0;
    uint32_t w=0,h=0,level=0,frameIndex=0;
    friend class MaterialTextureConvertedImageCandidate;
public:
    const uint8_t*data()const noexcept{return pixels?pixels->data()+offset:original?original->data():nullptr;}
    size_t size()const noexcept{return pixels?length:original?original->size():0;}
    uint32_t width()const noexcept{return w;}
    uint32_t height()const noexcept{return h;}
    uint32_t sourceLevel()const noexcept{return level;}
    uint32_t frame()const noexcept{return frameIndex;}
};

// Opt-in CPU conversion stage. No filesystem, original runtime, GPU objects,
// error-image fabrication, serialization or production editor publication.
class MaterialTextureConvertedImageCandidate {
    struct Block {size_t offset,length;uint32_t width,height,sourceLevel,frame;};
    std::shared_ptr<const MaterialTextureCpuImageCandidate>source;
    std::shared_ptr<const std::vector<uint8_t>>pixels;
    std::vector<Block>blocks;
    TextureImagePlanCandidate description{};
    TextureFormatPolicyResultCandidate decision{};
    uint32_t finalVtfFlags=0;
    MaterialTextureConvertedImageCandidate()=default;
    static std::array<uint8_t,4>color565(uint16_t v)noexcept{
        const uint32_t r=(v>>11)&31,g=(v>>5)&63,b=v&31;
        return {uint8_t((r<<3)|(r>>2)),uint8_t((g<<2)|(g>>4)),uint8_t((b<<3)|(b>>2)),255};
    }
    static bool convert(const MaterialTextureMipLeaseCandidate&input,uint32_t from,uint32_t to,std::vector<uint8_t>&out,size_t start){
        const auto*data=input.data();const auto w=input.width(),h=input.height();
        if(!data||!w||!h)return false;
        const uint64_t expected=(from==13||from==15)?uint64_t((w+3)/4)*((h+3)/4)*(from==15?16u:8u):uint64_t(w)*h*(from==3?3u:4u);
        if(expected!=input.size())return false;
        const auto put=[&](uint32_t x,uint32_t y,const std::array<uint8_t,4>&p){
            const auto step=to==3?3u:4u;auto*d=out.data()+start+(size_t(y)*w+x)*step;
            d[0]=p[to==0?0:2];d[1]=p[1];d[2]=p[to==0?2:0];if(step==4)d[3]=p[3];
        };
        if(from!=13&&from!=15){
            const uint32_t step=from==3?3u:4u;
            for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){const auto*p=data+(size_t(y)*w+x)*step;
                put(x,y,{p[from==0?0:2],p[1],p[from==0?2:0],step==3?uint8_t(255):p[3]});}
        }else for(uint32_t by=0;by<(h+3)/4;++by)for(uint32_t bx=0;bx<(w+3)/4;++bx){
            const auto*block=data+(size_t(by)*((w+3)/4)+bx)*(from==15?16u:8u);const auto*p=block+(from==15?8:0);uint16_t a,b;uint32_t selectors;
            std::memcpy(&a,p,2);std::memcpy(&b,p+2,2);std::memcpy(&selectors,p+4,4);
            std::array<std::array<uint8_t,4>,4>palette{};palette[0]=color565(a);palette[1]=color565(b);
            for(size_t k=0;k<3;++k){palette[2][k]=uint8_t(a>b?(2*palette[0][k]+palette[1][k])/3:(palette[0][k]+palette[1][k])/2);palette[3][k]=uint8_t(a>b?(palette[0][k]+2*palette[1][k])/3:(k?255:0));}
            palette[2][3]=255;palette[3][3]=a>b?255:0;
            // The measured native CPU DXT5 path shares this DXT1 colour
            // palette, including reversed endpoints. GPU BC3 is a separate rule.
            std::array<uint8_t,8>alpha{};uint64_t alphaSelectors=0;
            if(from==15){alpha[0]=block[0];alpha[1]=block[1];
                if(alpha[0]>alpha[1])for(uint32_t i=2;i<8;++i)alpha[i]=uint8_t(((8-i)*alpha[0]+(i-1)*alpha[1])/7);
                else{for(uint32_t i=2;i<6;++i)alpha[i]=uint8_t(((6-i)*alpha[0]+(i-1)*alpha[1])/5);alpha[6]=0;alpha[7]=255;}
                for(size_t i=0;i<6;++i)alphaSelectors|=uint64_t(block[2+i])<<(8*i);
            }
            for(uint32_t y=0;y<4;++y)for(uint32_t x=0;x<4;++x)if(bx*4+x<w&&by*4+y<h){const auto i=y*4+x;auto color=palette[(selectors>>(2*i))&3];if(from==15)color[3]=alpha[(alphaSelectors>>(3*i))&7];put(bx*4+x,by*4+y,color);}
        }
        return true;
    }
public:
    static bool prepare(const std::shared_ptr<const MaterialTextureCpuImageCandidate>&input,
        uint32_t compressionConfig,const TextureFormatPolicyServicesCandidate&services,
        std::shared_ptr<const MaterialTextureConvertedImageCandidate>&output,
        size_t maxOutputBytes=size_t(1)<<29)noexcept{
        try{
            const auto retained=input;if(!retained)return false;const auto plan=retained->plan();const uint32_t from=plan[2];
            if(from!=0&&from!=3&&from!=12&&from!=13&&from!=15)return false;
            TextureFormatPolicyResultCandidate policy;
            if(!textureFormatPolicyCandidate({from,from,plan[0],compressionConfig},services,policy))return false;
            // The only measured pixel operations admitted here are one direct
            // conversion to0/3/12, or no conversion. HDR/compression stay explicit.
            if(policy.conversionCount>1||(policy.conversionCount==1&&(policy.conversions[0]!=0&&policy.conversions[0]!=3&&policy.conversions[0]!=12)))return false;
            auto next=std::shared_ptr<MaterialTextureConvertedImageCandidate>(new MaterialTextureConvertedImageCandidate);
            next->source=retained;next->description=plan;next->description[2]=policy.textureFormat;next->decision=policy;next->finalVtfFlags=retained->effectiveVtfFlags();
            if(policy.conversionCount){
                const auto target=policy.conversions[0];auto bytes=std::make_shared<std::vector<uint8_t>>();
                for(uint32_t mip=0;mip<plan[9];++mip)for(uint32_t frame=0;frame<plan[10];++frame){
                    const auto lease=retained->lease(frame,mip);if(!lease)return false;
                    const uint64_t size=uint64_t(lease->width())*lease->height()*(target==3?3u:4u);
                    if(size>maxOutputBytes||bytes->size()>maxOutputBytes-size||size>std::numeric_limits<size_t>::max()-bytes->size())return false;
                    const auto offset=bytes->size();bytes->resize(offset+size_t(size));
                    if(!convert(*lease,from,target,*bytes,offset))return false;
                    next->blocks.push_back({offset,size_t(size),lease->width(),lease->height(),lease->sourceLevel(),frame});
                }
                next->pixels=std::move(bytes);
                // Native CTexture flags stay unchanged. Only effective VTF
                // alpha flags are rewritten by actual conversion, not a no-op.
                next->finalVtfFlags=target==3?(next->finalVtfFlags&~0x3000u):((next->finalVtfFlags&~0x1000u)|0x2000u);
            }
            output=std::move(next);return true;
        }catch(...){return false;}
    }
    const TextureImagePlanCandidate&plan()const noexcept{return description;}
    const TextureFormatPolicyResultCandidate&policy()const noexcept{return decision;}
    uint32_t rawFlags()const noexcept{return source->rawFlags();}
    uint32_t sourceVtfFlags()const noexcept{return source->effectiveVtfFlags();}
    uint32_t effectiveVtfFlags()const noexcept{return finalVtfFlags;}
    uint32_t requestedFlags()const noexcept{return source->requestedFlags();}
    uint32_t format()const noexcept{return decision.vtfFormat;}
    bool ownsConvertedPixels()const noexcept{return bool(pixels);}
    std::optional<MaterialConvertedTextureMipLeaseCandidate>lease(uint32_t frame,uint32_t mip)const noexcept{
        if(frame>=description[10]||mip>=description[9])return {};
        MaterialConvertedTextureMipLeaseCandidate next;
        if(pixels){const auto&b=blocks[size_t(mip)*description[10]+frame];next.pixels=pixels;next.offset=b.offset;next.length=b.length;next.w=b.width;next.h=b.height;next.level=b.sourceLevel;next.frameIndex=b.frame;}
        else{auto original=source->lease(frame,mip);if(!original)return {};next.w=original->width();next.h=original->height();next.level=original->sourceLevel();next.frameIndex=original->frame();next.original=std::move(original);}
        return next;
    }
};
