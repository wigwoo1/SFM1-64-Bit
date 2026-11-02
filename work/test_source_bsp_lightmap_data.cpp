#include "source_bsp_lightmap_data_candidate.h"
#include <fstream>
#include <iostream>
#include <type_traits>
// Existing owned fixture and preview test are reused unchanged.
#define main legacyLightmapMain
#define fixture legacyLightmapFixture
#define require legacyLightmapRequire
#include "test_bsp_lightmap_addressing.cpp"
#undef require
#undef fixture
#undef main

static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>static void refuses(F call,const char*message){bool refused=false;try{call();}catch(const std::exception&){refused=true;}check(refused,message);}
static void replaceLump(std::vector<uint8_t>& b,uint32_t id,const std::vector<uint8_t>& data,uint32_t version=0){
    word(b,8+id*16,uint32_t(b.size()));word(b,12+id*16,uint32_t(data.size()));word(b,16+id*16,version);word(b,20+id*16,0);b.insert(b.end(),data.begin(),data.end());
}
static Color marker(uint32_t style,uint32_t plane,uint32_t texel){return {uint8_t(3+13*style+7*plane+texel),uint8_t(80+texel),uint8_t(150+plane),uint8_t(int(style)-2)};}
static std::vector<uint8_t> completeFixture(int32_t offset,uint8_t styles,bool bumped){
    auto b=legacyLightmapFixture(offset,styles,false,bumped);const size_t light=readWord(b,8+8*16);
    if(offset>=0)for(uint32_t s=0;s<styles;++s)for(uint32_t p=0;p<(bumped?4u:1u);++p)for(uint32_t t=0;t<4;++t){
        const auto at=light+size_t(offset)+((s*(bumped?4u:1u)+p)*4+t)*4;const auto color=marker(s,p,t);std::copy(color.begin(),color.end(),b.begin()+at);
    }
    return b;
}
static std::vector<uint8_t> displacedFixture(){
    auto b=completeFixture(40,2,true);
    std::vector<uint8_t> v(48);number(v,16,2);number(v,24,2);number(v,28,2);number(v,36,2);replaceLump(b,3,v);
    std::vector<uint8_t> e(20);for(uint16_t n=0;n<4;++n){shortWord(e,4+4*n,n);shortWord(e,6+4*n,(n+1)%4);}replaceLump(b,12,e);
    std::vector<uint8_t> se(16);for(uint32_t n=0;n<4;++n)word(se,4*n,n+1);replaceLump(b,13,se);
    const auto face=readWord(b,8+7*16);shortWord(b,face+8,4);shortWord(b,face+12,0);
    std::vector<uint8_t> info(176);word(info,20,1);shortWord(info,36,0);replaceLump(b,26,info);
    std::vector<uint8_t> dv(9*20);for(size_t n=0;n<9;++n){number(dv,n*20+8,1);number(dv,n*20+12,3);number(dv,n*20+16,float(n*11));}replaceLump(b,33,dv);replaceLump(b,48,std::vector<uint8_t>(16));
    // Z contributes to raw luxels, deliberately distinguishing base positions
    // from the elevated displaced mesh. Preview behavior remains unchanged.
    const auto ti=readWord(b,8+6*16);number(b,ti+40,.25f);number(b,ti+56,-.5f);
    return b;
}
static void sameBrush(const SourceBspBrushGeometryCandidate&a,const SourceBspBrushGeometryCandidate&b){
    check(a.positions==b.positions&&a.normals==b.normals&&a.uvs==b.uvs&&a.lightColors==b.lightColors&&a.displacementAlphas==b.displacementAlphas&&a.materials==b.materials&&a.triangles.size()==b.triangles.size(),"legacy preview/geometry changed");
    for(size_t n=0;n<a.triangles.size();++n)check(a.triangles[n].vertices==b.triangles[n].vertices&&a.triangles[n].material==b.triangles[n].material,"legacy triangle/range identity changed");
}

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"native x64 test");
    static_assert(std::is_same<decltype(readSourceBspLightmapDataCandidate({},0,SourceBspLightmapSelectionCandidate::Ldr)),std::shared_ptr<const SourceBspLightmapDataCandidate>>::value,"immutable owner API");
    check(argc==3,"fresh-output-directory read-only-benchmark-bsp");const std::filesystem::path dir=argv[1];check(std::filesystem::create_directory(dir),"fresh test output only");
    size_t fixtures=0,samples=0,invalid=0;
    for(bool bumped:{false,true})for(uint8_t styles=1;styles<=4;++styles)for(int32_t offset:{0,16,40}){
        auto bytes=completeFixture(offset,styles,bumped);const auto before=bytes;auto preview=readSourceBspInlineBrushCandidate(bytes,0);
        auto owner=readSourceBspLightmapDataCandidate(bytes,0,SourceBspLightmapSelectionCandidate::Ldr);
        check(bytes==before,"raw reader modified input");check(owner->pairedFaceLightingTables()&&owner->faceLump()==7&&owner->lightingLump()==8&&owner->faces().size()==1,"LDR selection and face coverage");
        const auto& f=owner->face(0);check(f.faceIndex==0&&f.texInfoIndex==0&&f.styleCount==styles&&f.planesPerStyle==(bumped?4u:1u)&&f.samplesPerPlane==4&&f.lightByteCount==uint64_t(styles)*f.planesPerStyle*16,"style/plane dimensions");
        const size_t face=readWord(bytes,8+7*16),ti=readWord(bytes,8+6*16),light=readWord(bytes,8+8*16);
        check(std::memcmp(f.rawFace.data(),bytes.data()+face,56)==0&&std::memcmp(f.rawTexInfo.data(),bytes.data()+ti,72)==0,"exact raw face/texinfo records");
        check(owner->rawLighting()==std::vector<uint8_t>(bytes.begin()+light,bytes.begin()+light+512),"complete lighting including padding/average prefix retained");
        for(uint32_t s=0;s<styles;++s)for(uint32_t p=0;p<f.planesPerStyle;++p)for(uint32_t t=0;t<4;++t){check(owner->sample(0,s,p,t%2,t/2)==marker(s,p,t),"exact independent style/plane raw marker");++samples;}
        const auto first=owner->sample(0,0,0,0,0);bytes.assign(bytes.size(),99);bytes.clear();bytes.shrink_to_fit();check(owner->sample(0,0,0,0,0)==first,"owner borrows input memory");
        sameBrush(preview,readSourceBspInlineBrushCandidate(before,0));++fixtures;
    }
    auto b=completeFixture(40,2,true);const auto face=readWord(b,8+7*16),ti=readWord(b,8+6*16);
    word(b,face+28,uint32_t(-3));word(b,face+32,uint32_t(-5));
    const std::array<float,8> axes{{.5f,.25f,.125f,-2.f,-.25f,.5f,-.125f,-4.f}};
    for(size_t n=0;n<8;++n)number(b,ti+32+n*4,axes[n]);
    auto coordinateOwner=readSourceBspLightmapDataCandidate(b,0,SourceBspLightmapSelectionCandidate::Ldr);
    check(coordinateOwner->luxel(0,{2,4,8})==std::array<double,2>{4.,1.5},"unclamped raw luxel axes/minima calculation");
    const auto untouched=coordinateOwner->sample(0,1,3,1,1);
    // Stable global face identity is independent of the model-local slot.
    auto span=completeFixture(40,2,true);const auto oldFace=readWord(span,8+7*16),oldModel=readWord(span,8+14*16);
    std::vector<uint8_t> faces(span.begin()+oldFace,span.begin()+oldFace+56);faces.insert(faces.end(),span.begin()+oldFace,span.begin()+oldFace+56);replaceLump(span,7,faces,1);
    std::vector<uint8_t> models(span.begin()+oldModel,span.begin()+oldModel+48);models.insert(models.end(),span.begin()+oldModel,span.begin()+oldModel+48);word(models,48+40,1);replaceLump(span,14,models);
    auto second=readSourceBspLightmapDataCandidate(span,1,SourceBspLightmapSelectionCandidate::Ldr);check(second->modelIndex()==1&&second->firstFace()==1&&second->face(0).faceIndex==1&&second->sample(0,1,3,1,1)==untouched,"inline model/global face addressing");
    // Distinct unavailable states preserve their authored records, not white.
    for(bool missing:{false,true}){
        auto none=completeFixture(40,2,true);const auto at=readWord(none,8+7*16);
        if(missing)word(none,at+20,UINT32_MAX);else none[at+16]=255;
        auto owner=readSourceBspLightmapDataCandidate(none,0,SourceBspLightmapSelectionCandidate::Ldr);
        check(owner->face(0).availability==(missing?SourceBspLightmapAvailabilityCandidate::MissingOffset:SourceBspLightmapAvailabilityCandidate::NoStyles),"missing offset/styles conflated with available white");
        refuses([&]{owner->sample(0,0,0,0,0);},"unavailable samples exposed");++invalid;
    }
    // Explicit HDR pairing, preference and historical preview-only fallback.
    auto hdr=completeFixture(40,2,true);for(size_t n=0;n<4;++n){word(hdr,8+58*16+n*4,readWord(hdr,8+7*16+n*4));word(hdr,8+53*16+n*4,readWord(hdr,8+8*16+n*4));}
    auto both=readSourceBspLightmapDataCandidate(hdr,0,SourceBspLightmapSelectionCandidate::LegacyPreview);check(both->lightingLump()==8,"legacy LDR preference changed");
    auto nativeHdr=readSourceBspLightmapDataCandidate(hdr,0,SourceBspLightmapSelectionCandidate::Hdr);check(nativeHdr->pairedFaceLightingTables()&&nativeHdr->faceLump()==58&&nativeHdr->lightingLump()==53,"explicit HDR table selection");
    word(hdr,12+8*16,0);auto onlyHdr=readSourceBspLightmapDataCandidate(hdr,0,SourceBspLightmapSelectionCandidate::LegacyPreview);check(onlyHdr->faceLump()==58&&onlyHdr->sample(0,1,3,1,1)==untouched,"HDR-only selection");
    word(hdr,12+58*16,0);auto fallback=readSourceBspLightmapDataCandidate(hdr,0,SourceBspLightmapSelectionCandidate::LegacyPreview);check(!fallback->pairedFaceLightingTables()&&fallback->faceLump()==7&&fallback->lightingLump()==53,"legacy fallback wrongly certified paired");
    refuses([&]{readSourceBspLightmapDataCandidate(hdr,0,SourceBspLightmapSelectionCandidate::Hdr);},"strict HDR silently uses LDR face table");++invalid;
    auto displacement=displacedFixture();const auto oldDisplaced=readSourceBspInlineBrushCandidate(displacement,0);auto displacedOwner=readSourceBspLightmapDataCandidate(displacement,0,SourceBspLightmapSelectionCandidate::Ldr);
    check(oldDisplaced.positions.size()==9&&displacedOwner->face(0).displacementIndex==0&&displacedOwner->face(0).displacementPower==1,"displacement identity");
    check(displacedOwner->luxel(0,{1,1,0})==std::array<double,2>{.5,.5}&&displacedOwner->luxel(0,{1,1,3})!=std::array<double,2>{.5,.5},"base-grid coordinate discriminator");sameBrush(oldDisplaced,readSourceBspInlineBrushCandidate(displacement,0));
    // Every rejection must leave the previously committed immutable owner intact.
    auto committed=coordinateOwner;const auto* identity=committed.get();
    for(size_t bad=0;bad<20;++bad){auto broken=b;
        if(bad==0)broken.resize(100);
        if(bad==1)word(broken,4,99);
        if(bad==2)word(broken,face+20,uint32_t(-2));
        if(bad==3)word(broken,face+36,UINT32_MAX);
        if(bad==4)word(broken,face+36,4097);
        if(bad==5)word(broken,face+36,4096),word(broken,face+40,4096);
        if(bad==6)word(broken,face+20,UINT32_MAX/2);
        if(bad==7)word(broken,12+8*16,40+2*4*16-1); // Last directional plane truncated, base preview still readable.
        if(bad==8)word(broken,20+8*16,123);
        if(bad==9)word(broken,16+7*16,2);
        if(bad==10)shortWord(broken,face+10,99);
        if(bad==11)number(broken,ti+32,NAN);
        if(bad==12)word(broken,readWord(broken,8+14*16)+44,2);
        if(bad==13)word(broken,8+8*16,UINT32_MAX);
        if(bad==14)word(broken,12+8*16,511);
        if(bad==15)shortWord(broken,face+12,0); // Missing displacement table.
        if(bad==16)word(broken,16+8*16,2);
        if(bad==17)shortWord(broken,face+12,uint16_t(-2));
        if(bad==18){broken=displacement;const auto dp=readWord(broken,8+26*16);shortWord(broken,dp+36,1);}
        if(bad==19){broken=displacement;const auto dp=readWord(broken,8+26*16);word(broken,dp+20,5);}
        refuses([&]{auto next=readSourceBspLightmapDataCandidate(broken,0,SourceBspLightmapSelectionCandidate::Ldr);committed=std::move(next);},"invalid raw data accepted");
        check(committed.get()==identity&&committed->sample(0,1,3,1,1)==untouched,"failed construction replaced committed data");++invalid;
    }
    for(size_t bad=0;bad<7;++bad){refuses([&]{if(bad==0)committed->sample(1,0,0,0,0);if(bad==1)committed->sample(0,2,0,0,0);if(bad==2)committed->sample(0,0,4,0,0);if(bad==3)committed->sample(0,0,0,2,0);if(bad==4)committed->sample(0,0,0,0,2);if(bad==5)committed->luxel(0,{NAN,0,0});if(bad==6)readSourceBspLightmapDataCandidate(b,0,SourceBspLightmapSelectionCandidate(99));},"invalid access/selection accepted");++invalid;}
    check(legacyLightmapMain()==0,"original owned lightmap preview tests failed");
    std::cout<<"controlled_lightmap_data=PASS fixtures="<<fixtures<<" raw_samples="<<samples<<" rejection_checks="<<invalid<<" immutable_owner=1 source_storage_destroyed=1 legacy_preview_unchanged=1 hdr_pairing_explicit=1 displacement_base_coordinates=1\n";
    const auto benchmark=readBinaryAssetCandidate(argv[2]);const auto beforeBrush=readSourceBspInlineBrushCandidate(benchmark,0);auto owner=readSourceBspLightmapDataCandidate(benchmark,0,SourceBspLightmapSelectionCandidate::Hdr);sameBrush(beforeBrush,readSourceBspInlineBrushCandidate(benchmark,0));
    size_t available=0,unavailable=0,bumped=0,displaced=0;uint64_t rawSamples=0;
    std::ofstream inventory(dir/"faces.bin",std::ios::binary);
    for(const auto& f:owner->faces()){
        if(f.availability==SourceBspLightmapAvailabilityCandidate::Samples){++available;rawSamples+=f.lightByteCount/4;}else ++unavailable;
        if(f.planesPerStyle==4)++bumped;if(f.displacementIndex>=0)++displaced;
        // Field-wise output avoids struct padding/ABI as an accidental contract.
        for(uint32_t v:{f.faceIndex,f.texInfoIndex,f.flags,uint32_t(f.displacementIndex),uint32_t(f.lightOffset),uint32_t(f.minima[0]),uint32_t(f.minima[1]),uint32_t(f.size[0]),uint32_t(f.size[1]),f.styleCount,f.planesPerStyle,uint32_t(f.availability)})inventory.write(reinterpret_cast<const char*>(&v),4);
        inventory.write(reinterpret_cast<const char*>(f.styles.data()),4);inventory.write(reinterpret_cast<const char*>(f.rawFace.data()),56);inventory.write(reinterpret_cast<const char*>(f.rawTexInfo.data()),72);
        for(uint32_t s=0;s<f.styleCount&&f.availability==SourceBspLightmapAvailabilityCandidate::Samples;++s)for(uint32_t p=0;p<f.planesPerStyle;++p)for(uint32_t y=0;y<=uint32_t(f.size[1]);++y)for(uint32_t x=0;x<=uint32_t(f.size[0]);++x){const auto c=owner->sample(f.faceIndex-owner->firstFace(),s,p,x,y);inventory.write(reinterpret_cast<const char*>(c.data()),4);}
    }
    std::ofstream lighting(dir/"lighting.bin",std::ios::binary);lighting.write(reinterpret_cast<const char*>(owner->rawLighting().data()),std::streamsize(owner->rawLighting().size()));check(bool(inventory)&&bool(lighting),"benchmark inventory outputs");
    std::cout<<"benchmark_lightmap_data=PASS faces="<<owner->faces().size()<<" available="<<available<<" unavailable="<<unavailable<<" bumped="<<bumped<<" displacement_faces="<<displaced<<" lighting_bytes="<<owner->rawLighting().size()<<" raw_samples="<<rawSamples<<" world_vertices="<<beforeBrush.positions.size()<<" world_triangles="<<beforeBrush.triangles.size()<<" original_runtime=ABSENT\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
