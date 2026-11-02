#include "source_bsp_base_lightmap_candidate.h"
#include <iostream>
#define main legacyBaseLightmapMain
#include "test_bsp_lightmap_addressing.cpp"
#undef main

static auto baseSnapshot(const std::vector<uint8_t>& b){
    auto brush=readSourceBspLightmappedBrushCandidate(b,0);std::vector<uint32_t> indices;
    for(const auto& t:brush.geometry.triangles)indices.insert(indices.end(),t.vertices.begin(),t.vertices.end());
    return prepareSourceBspLightmapSnapshotCandidate(std::move(brush),indices);
}
static auto hdrFixture(int32_t offset=40,uint8_t styles=1,bool bumped=false){
    auto b=fixture(offset,styles,false,bumped);
    for(size_t k=0;k<4;++k){word(b,8+58*16+k*4,readWord(b,8+7*16+k*4));word(b,8+53*16+k*4,readWord(b,8+8*16+k*4));}
    word(b,12+8*16,0);return b;
}
int main(){try{
    static_assert(sizeof(void*)==8,"native x64 test");size_t samples=0,negative=0;
    for(int e=-128;e<=127;++e)for(int c=0;c<=255;++c){
        const auto decoded=decodeSourceTexLightLinearCandidate({uint8_t(c),uint8_t(255-c),uint8_t(c/2),uint8_t(e<0?e+256:e)});
        const int channels[]{c,255-c,c/2};
        for(size_t k=0;k<3;++k){const auto want=float(std::ldexp(static_cast<long double>(channels[k])/255.L,e));
            require(std::isfinite(decoded[k])&&std::abs(double(decoded[k])-want)<=std::abs(double(want))*2e-7+std::numeric_limits<float>::denorm_min(),"signed TexLightLinear decoder");++samples;}
        require(decoded[3]==1,"opaque decoded lightmap");
    }
    const auto material=parseVmtCandidate("LightmappedGeneric { $basetexture owned/albedo }");
    auto make=[&](auto snapshot,const auto& m){return SourceBspBaseLightmapCandidate::prepare(snapshot,0,m,SourceBaseLightmapUnitsCandidate::TexLightLinear);};
    auto source=baseSnapshot(hdrFixture());std::weak_ptr<const SourceBspLightmapSnapshotCandidate> lifetime=source;
    auto plane=make(source,material);source.reset();require(!lifetime.expired()&&plane->source().use_count()==1&&plane->width()==2&&plane->height()==2,"retained decoded plane ownership");
    for(size_t k=0;k<4;++k)require(plane->pixels()[k]==decodeSourceTexLightLinearCandidate(firstStyle[k]),"raw byte offset and plane order");
    for(int32_t offset:{0,16}){auto relocated=make(baseSnapshot(hdrFixture(offset)),material);require(relocated->pixels()==plane->pixels(),"raw offset relocation");}
    auto refuses=[&](auto call){bool failed=false;try{call();}catch(const std::exception&){failed=true;}require(failed,"unsupported input accepted");++negative;};
    refuses([&]{make(nullptr,material);});refuses([&]{SourceBspBaseLightmapCandidate::prepare(plane->source(),0,material,SourceBaseLightmapUnitsCandidate(99));});
    refuses([&]{SourceBspBaseLightmapCandidate::prepare(plane->source(),1,material,SourceBaseLightmapUnitsCandidate::TexLightLinear);});
    refuses([&]{make(baseSnapshot(fixture(40,1)),material);});
    auto unpaired=hdrFixture();word(unpaired,12+58*16,0);refuses([&]{make(baseSnapshot(unpaired),material);});
    auto unavailable=unavailableSourceBspLightmapSnapshotCandidate(3,{0,1,2},"owned missing data");refuses([&]{make(unavailable,material);});
    refuses([&]{make(baseSnapshot(hdrFixture(40,2)),material);});refuses([&]{make(baseSnapshot(hdrFixture(40,1,true)),material);});
    for(int kind=0;kind<4;++kind){auto b=hdrFixture();const auto f=readWord(b,8+58*16),ti=readWord(b,8+6*16);
        if(kind==0)b[f+16]=2;if(kind==1)word(b,ti+64,0x400);if(kind==2)word(b,f+20,UINT32_MAX);if(kind==3)b[f+16]=255;
        refuses([&]{make(baseSnapshot(b),material);});}
    for(int kind=0;kind<3;++kind){auto bad=std::make_shared<SourceBspLightmapSnapshotCandidate>(*plane->source());
        if(kind==0){bad->localIndices.clear();bad->orderedTriangleFaces.clear();}if(kind==1)bad->vertexLuxels[0].luxel[0]=NAN;if(kind==2)bad->localIndices[0]=UINT32_MAX;
        refuses([&]{make(bad,material);});}
    for(const char* text:{"UnlitGeneric { $basetexture a }","VertexLitGeneric { $basetexture a }","LightmappedGeneric { }","LightmappedGeneric { $basetexture a $bumpmap b }","LightmappedGeneric { $basetexture a Proxies { } }","LightmappedGeneric { $basetexture \"\" }","LightmappedGeneric { $basetexture { } }","LightmappedGeneric { $basetexture a $basetexture b }"})
        refuses([&]{make(plane->source(),parseVmtCandidate(text));});
    // Explicit displacement rejection: the raw record is valid, but unsupported
    // by this operator. No fabricated accepted displacement coordinates.
    auto disp=hdrFixture();const auto face=readWord(disp,8+58*16);shortWord(disp,face+12,0);
    word(disp,8+26*16,uint32_t(disp.size()));word(disp,12+26*16,176);const auto at=disp.size();disp.resize(at+176);word(disp,at+20,1);
    auto displaced=std::make_shared<SourceBspLightmapSnapshotCandidate>(*plane->source());displaced->lightmaps=readSourceBspLightmapDataCandidate(disp,0,SourceBspLightmapSelectionCandidate::Hdr);
    refuses([&]{make(displaced,material);});
    plane.reset();require(lifetime.expired(),"decoded plane retained snapshot after release");
    std::cout<<"PASS decoder_components="<<samples<<" refusal_checks="<<negative<<" lifetime=1 offsets=3 native_x64=1\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
