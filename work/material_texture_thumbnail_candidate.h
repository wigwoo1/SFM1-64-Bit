#pragma once
#include "material_texture_resource_candidate.h"
#include <array>

// Isolated CPU RGB thumbnail conversion. The DXT1 transparent selector is cyan
// on this measured native path; do not use this rule for general RGBA images.
class MaterialTextureThumbnailCandidate {
    uint32_t w=0,h=0;
    std::vector<uint8_t>pixels;
    MaterialTextureThumbnailCandidate(uint32_t width,uint32_t height,std::vector<uint8_t>rgb)noexcept:
        w(width),h(height),pixels(std::move(rgb)){}
public:
    static bool prepare(const MaterialTextureReferenceCandidate&input,
        std::shared_ptr<const MaterialTextureThumbnailCandidate>&output)noexcept{
        try{
            if(!input.valid()||input.isEnvironment()||!input.resource())return false;
            const auto retained=input.resource();const auto&b=retained->bytes();
            const auto word=[&](size_t at){uint32_t n;std::memcpy(&n,b.data()+at,4);return n;};
            const uint32_t width=b[61],height=b[62],format=word(57);std::vector<uint8_t>rgb;
            if(width&&height){
                if(format!=0&&format!=12&&format!=13)return false;
                // Native compressed storage floors block counts while the RGB
                // path may round temporary dimensions up. Reject that mismatch.
                if(format==13&&((width>4&&(width&3u))||(height>4&&(height&3u))))return false;
                size_t offset=word(12);
                if(word(8)>=3){bool found=false;for(uint32_t i=0;i<word(68);++i)
                    if(word(80+size_t(i)*8)==1){offset=word(84+size_t(i)*8);found=true;}
                    if(!found)return false;}
                const size_t n=format==13?size_t((width+3)/4)*((height+3)/4)*8:size_t(width)*height*4;
                if(offset>b.size()||n>b.size()-offset)return false;
                const auto*data=b.data()+offset;rgb.resize(size_t(width)*height*3);
                if(format!=13){for(size_t i=0;i<size_t(width)*height;++i){rgb[i*3]=data[i*4+(format==12?2:0)];rgb[i*3+1]=data[i*4+1];rgb[i*3+2]=data[i*4+(format==12?0:2)];}}
                else for(uint32_t by=0;by<(height+3)/4;++by)for(uint32_t bx=0;bx<(width+3)/4;++bx){
                    const auto*block=data+(size_t(by)*((width+3)/4)+bx)*8;uint16_t a,c;uint32_t selectors;
                    std::memcpy(&a,block,2);std::memcpy(&c,block+2,2);std::memcpy(&selectors,block+4,4);
                    const auto expand=[](uint16_t v){const uint32_t r=(v>>11)&31,g=(v>>5)&63,b=v&31;
                        return std::array<uint8_t,3>{uint8_t((r<<3)|(r>>2)),uint8_t((g<<2)|(g>>4)),uint8_t((b<<3)|(b>>2))};};
                    std::array<std::array<uint8_t,3>,4>p{};p[0]=expand(a);p[1]=expand(c);
                    for(size_t k=0;k<3;++k){p[2][k]=uint8_t(a>c?(2*p[0][k]+p[1][k])/3:(p[0][k]+p[1][k])/2);p[3][k]=uint8_t(a>c?(p[0][k]+2*p[1][k])/3:(k?255:0));}
                    for(uint32_t y=0;y<4;++y)for(uint32_t x=0;x<4;++x){const uint32_t xx=bx*4+x,yy=by*4+y;
                        if(xx<width&&yy<height){const auto&color=p[(selectors>>(2*(y*4+x)))&3];std::memcpy(rgb.data()+(size_t(yy)*width+xx)*3,color.data(),3);}}
                }
            }
            auto next=std::shared_ptr<const MaterialTextureThumbnailCandidate>(new MaterialTextureThumbnailCandidate(width,height,std::move(rgb)));
            output=std::move(next);return true;
        }catch(...){return false;}
    }
    uint32_t width()const noexcept{return w;}
    uint32_t height()const noexcept{return h;}
    const std::vector<uint8_t>&rgb()const noexcept{return pixels;}
};
