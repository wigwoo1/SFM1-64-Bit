#pragma once
// Independently authored, checked Source BSP lightmap records. Format references:
// Valve Source SDK 2013 public/bspfile.h, public/bspflags.h and
// utils/vrad/{lightmap,radial}.cpp. No original binary/shader data is embedded.
// Raw RGBExp32 is intentionally NOT converted to preview colors or GPU textures.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include "source_bsp_lightmap_selection_candidate.h"

enum class SourceBspLightmapAvailabilityCandidate {MissingOffset, NoStyles, Samples};

struct SourceBspLightmapFaceCandidate {
    uint32_t faceIndex=0,texInfoIndex=0,flags=0;
    int16_t displacementIndex=-1;
    int32_t displacementPower=0;
    int32_t lightOffset=-1;
    std::array<int32_t,2> minima{},size{};
    std::array<std::array<float,4>,2> axes{};
    std::array<uint8_t,4> styles{};
    uint32_t styleCount=0,planesPerStyle=1;
    uint64_t samplesPerPlane=0,lightByteCount=0;
    SourceBspLightmapAvailabilityCandidate availability=SourceBspLightmapAvailabilityCandidate::MissingOffset;
    std::array<uint8_t,56> rawFace{};
    std::array<uint8_t,72> rawTexInfo{};
};

class SourceBspLightmapDataCandidate {
    std::vector<uint8_t> lighting_;
    std::vector<SourceBspLightmapFaceCandidate> faces_;
    uint32_t modelIndex_=0,firstFace_=0,faceLump_=0,lightingLump_=0;
    int32_t bspVersion_=0,lightingVersion_=0;
    bool pairedTables_=true;
public:
    SourceBspLightmapDataCandidate(const std::vector<uint8_t>& bytes,uint32_t modelIndex,
                                  SourceBspLightmapSelectionCandidate selection):modelIndex_(modelIndex){
        auto require=[](bool valid,const char* message){if(!valid)throw std::runtime_error(message);};
        auto range=[&](uint64_t offset,uint64_t size){require(offset<=bytes.size()&&size<=bytes.size()-offset,"lightmap BSP byte range");};
        auto u32=[&](size_t at){range(at,4);uint32_t value;std::memcpy(&value,bytes.data()+at,4);return value;};
        auto i32=[&](size_t at){range(at,4);int32_t value;std::memcpy(&value,bytes.data()+at,4);return value;};
        auto i16=[&](size_t at){range(at,2);int16_t value;std::memcpy(&value,bytes.data()+at,2);return value;};
        auto scalar=[&](size_t at){range(at,4);float value;std::memcpy(&value,bytes.data()+at,4);require(std::isfinite(value),"nonfinite lightmap axis");return value;};
        require(bytes.size()>=1036&&std::memcmp(bytes.data(),"VBSP",4)==0,"lightmap VBSP header");
        bspVersion_=i32(4);require(bspVersion_==19||bspVersion_==20,"unsupported lightmap BSP version");
        struct Lump {uint32_t offset,size;int32_t version,compression;};
        auto lump=[&](uint32_t id,size_t stride){
            const size_t at=8+16*size_t(id);
            Lump result{u32(at),u32(at+4),i32(at+8),i32(at+12)};
            require(result.offset<=INT32_MAX&&result.size<=INT32_MAX,"negative lightmap lump span");
            range(result.offset,result.size);
            require((!result.size||result.offset>=1036)&&result.size%stride==0&&!result.compression,"unsupported lightmap lump layout/compression");
            return result;
        };
        const auto models=lump(14,48),texInfo=lump(6,72);
        require(models.version==0&&texInfo.version==0,"unsupported lightmap model/texinfo version");
        require(modelIndex<models.size/48,"lightmap inline model index");
        const auto modelOffset=models.offset+size_t(modelIndex)*48;
        const auto first=i32(modelOffset+40),count=i32(modelOffset+44);
        require(first>=0&&count>=0,"negative lightmap face span");firstFace_=uint32_t(first);
        if(selection==SourceBspLightmapSelectionCandidate::Ldr){faceLump_=7;lightingLump_=8;}
        else if(selection==SourceBspLightmapSelectionCandidate::Hdr){
            faceLump_=58;lightingLump_=53;
            require(u32(12+58*16)!=0,"HDR lightmaps require the HDR face table");
        }else if(selection==SourceBspLightmapSelectionCandidate::LegacyPreview){
            const bool hdr=!u32(12+8*16)&&u32(12+53*16);
            lightingLump_=hdr?53:8;faceLump_=hdr&&u32(12+58*16)?58:7;
            pairedTables_=!hdr||faceLump_==58;
        }else throw std::runtime_error("invalid lightmap dataset selection");
        const auto faceData=lump(faceLump_,56),lightData=lump(lightingLump_,4);
        require(faceData.version==1,"unsupported lightmap face version");
        // Existing owned fixtures use version0; benchmark/compiler files use1.
        // Both retain the same four-byte raw record. Other versions are refused.
        require(lightData.version==0||lightData.version==1,"unsupported lighting record version");
        lightingVersion_=lightData.version;
        require(uint64_t(firstFace_)+uint32_t(count)<=faceData.size/56,"lightmap model faces outside table");
        // Copy once, including padding and the compiler's reverse-ordered average
        // colors before lightofs. No span borrows the caller's input allocation.
        lighting_.assign(bytes.begin()+lightData.offset,bytes.begin()+lightData.offset+lightData.size);
        faces_.reserve(size_t(count));
        for(uint32_t slot=0;slot<uint32_t(count);++slot){
            SourceBspLightmapFaceCandidate face;face.faceIndex=firstFace_+slot;
            const size_t at=faceData.offset+size_t(face.faceIndex)*56;
            const auto ti=i16(at+10);require(ti>=0&&uint32_t(ti)<texInfo.size/72,"lightmap face texinfo outside table");
            face.texInfoIndex=uint32_t(ti);face.displacementIndex=i16(at+12);
            require(face.displacementIndex>=-1,"invalid lightmap displacement identity");
            if(face.displacementIndex>=0){
                const auto info=lump(26,176);
                require(info.version==0&&uint32_t(face.displacementIndex)<info.size/176,"lightmap displacement reference outside table");
                const size_t dp=info.offset+size_t(face.displacementIndex)*176;
                uint16_t mapFace;range(dp+36,2);std::memcpy(&mapFace,bytes.data()+dp+36,2);
                require(mapFace==face.faceIndex,"lightmap displacement face identity mismatch");
                face.displacementPower=i32(dp+20);
                require(face.displacementPower>=1&&face.displacementPower<=4,"unsupported lightmap displacement power");
                for(size_t c=0;c<3;++c)(void)scalar(dp+c*4);
            }
            face.lightOffset=i32(at+20);require(face.lightOffset>=-1,"invalid negative lightmap offset");
            for(size_t axis=0;axis<2;++axis){face.minima[axis]=i32(at+28+axis*4);face.size[axis]=i32(at+36+axis*4);}
            std::memcpy(face.rawFace.data(),bytes.data()+at,face.rawFace.size());
            std::memcpy(face.styles.data(),bytes.data()+at+16,4);
            while(face.styleCount<4&&face.styles[face.styleCount]!=255)++face.styleCount;
            const size_t tiOffset=texInfo.offset+size_t(face.texInfoIndex)*72;
            std::memcpy(face.rawTexInfo.data(),bytes.data()+tiOffset,face.rawTexInfo.size());
            face.flags=u32(tiOffset+64);face.planesPerStyle=(face.flags&0x800)?4:1;
            for(size_t axis=0;axis<2;++axis)for(size_t c=0;c<4;++c)face.axes[axis][c]=scalar(tiOffset+32+axis*16+c*4);
            if(face.lightOffset<0)face.availability=SourceBspLightmapAvailabilityCandidate::MissingOffset;
            else if(!face.styleCount)face.availability=SourceBspLightmapAvailabilityCandidate::NoStyles;
            else{
                for(auto size:face.size)require(size>=0&&size<=4096,"unsupported lightmap dimensions");
                face.samplesPerPlane=(uint64_t(face.size[0])+1)*(uint64_t(face.size[1])+1);
                require(face.samplesPerPlane<=16u*1024u*1024u,"lightmap plane sample limit");
                face.lightByteCount=face.samplesPerPlane*face.planesPerStyle*face.styleCount*4;
                const auto offset=uint64_t(face.lightOffset);
                require(offset<=lighting_.size()&&face.lightByteCount<=lighting_.size()-offset,"complete lightmap style/plane span outside lighting lump");
                face.availability=SourceBspLightmapAvailabilityCandidate::Samples;
            }
            faces_.push_back(face);
        }
    }
    uint32_t modelIndex()const noexcept{return modelIndex_;}
    uint32_t firstFace()const noexcept{return firstFace_;}
    uint32_t faceLump()const noexcept{return faceLump_;}
    uint32_t lightingLump()const noexcept{return lightingLump_;}
    int32_t bspVersion()const noexcept{return bspVersion_;}
    int32_t lightingVersion()const noexcept{return lightingVersion_;}
    // False only for explicitly requested legacy HDR-with-LDR-faces preview.
    // That historical fallback must never imply verified native HDR addressing.
    bool pairedFaceLightingTables()const noexcept{return pairedTables_;}
    const std::vector<uint8_t>& rawLighting()const noexcept{return lighting_;}
    const std::vector<SourceBspLightmapFaceCandidate>& faces()const noexcept{return faces_;}
    const SourceBspLightmapFaceCandidate& face(size_t slot)const{return faces_.at(slot);}
    std::array<uint8_t,4> sample(size_t faceSlot,uint32_t styleSlot,uint32_t plane,uint32_t x,uint32_t y)const{
        const auto& f=face(faceSlot);
        if(f.availability!=SourceBspLightmapAvailabilityCandidate::Samples||styleSlot>=f.styleCount||plane>=f.planesPerStyle||x>uint32_t(f.size[0])||y>uint32_t(f.size[1]))throw std::runtime_error("lightmap sample outside face/style/plane");
        const uint64_t sampleIndex=(uint64_t(styleSlot)*f.planesPerStyle+plane)*f.samplesPerPlane+uint64_t(y)*(uint32_t(f.size[0])+1)+x;
        const auto offset=size_t(uint64_t(f.lightOffset)+sampleIndex*4);
        return {lighting_[offset],lighting_[offset+1],lighting_[offset+2],lighting_[offset+3]};
    }
    // Raw luxels, no atlas normalization, half texel, V flip or clamp. For
    // displacements callers must supply the undisplaced base-grid position.
    std::array<double,2> luxel(size_t faceSlot,const std::array<float,3>& sourceBasePosition)const{
        const auto& f=face(faceSlot);std::array<double,2> result{};
        for(float p:sourceBasePosition)if(!std::isfinite(p))throw std::runtime_error("nonfinite lightmap source position");
        for(size_t axis=0;axis<2;++axis){
            result[axis]=double(f.axes[axis][3])-f.minima[axis];
            for(size_t c=0;c<3;++c)result[axis]+=double(sourceBasePosition[c])*f.axes[axis][c];
            if(!std::isfinite(result[axis]))throw std::runtime_error("nonfinite raw luxel");
        }
        return result;
    }
};

inline std::shared_ptr<const SourceBspLightmapDataCandidate> readSourceBspLightmapDataCandidate(
    const std::vector<uint8_t>& bytes,uint32_t modelIndex,SourceBspLightmapSelectionCandidate selection){
    return std::make_shared<const SourceBspLightmapDataCandidate>(bytes,modelIndex,selection);
}
