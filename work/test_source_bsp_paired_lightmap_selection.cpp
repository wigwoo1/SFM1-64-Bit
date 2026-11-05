#include "source_bsp_lightmap_binding_candidate.h"
#include <iostream>

static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void u16(std::vector<uint8_t>& b,size_t p,uint16_t v){std::memcpy(b.data()+p,&v,2);}
static void u32(std::vector<uint8_t>& b,size_t p,uint32_t v){std::memcpy(b.data()+p,&v,4);}
static void f32(std::vector<uint8_t>& b,size_t p,float v){std::memcpy(b.data()+p,&v,4);}
static uint32_t get32(const std::vector<uint8_t>& b,size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;}

static std::vector<uint8_t> fixture(){
    std::vector<uint8_t>b(1036);std::memcpy(b.data(),"VBSP",4);u32(b,4,20);
    auto lump=[&](uint32_t id,const std::vector<uint8_t>& data,int32_t version=0){u32(b,8+16*id,uint32_t(b.size()));u32(b,12+16*id,uint32_t(data.size()));u32(b,16+16*id,uint32_t(version));b.insert(b.end(),data.begin(),data.end());};
    std::vector<uint8_t> vertices(5*12);const float xyz[5][3]={{0,0,0},{2,0,0},{0,2,0},{2,2,0},{3,2,0}};
    for(size_t n=0;n<5;++n)for(size_t c=0;c<3;++c)f32(vertices,n*12+c*4,xyz[n][c]);lump(3,vertices);
    std::vector<uint8_t> edges(7*4),surf(7*4);const uint16_t ends[7][2]={{0,1},{1,2},{2,0},{0,1},{1,3},{3,4},{4,0}};
    for(size_t n=0;n<7;++n){u16(edges,n*4,ends[n][0]);u16(edges,n*4+2,ends[n][1]);u32(surf,n*4,uint32_t(n));}lump(12,edges);lump(13,surf);
    std::vector<uint8_t> ti(2*72);const float axes[2][8]={{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};
    (void)axes;
    f32(ti,32,1);f32(ti,44,10);f32(ti,52,1);f32(ti,60,20);u32(ti,68,0);
    f32(ti,72+32,2);f32(ti,72+44,100);f32(ti,72+52,3);f32(ti,72+60,200);u32(ti,72+68,0);lump(6,ti);
    std::vector<uint8_t> td(32);u32(td,12,0);u32(td,16,64);u32(td,20,64);u32(td,24,64);u32(td,28,64);lump(2,td);
    lump(43,{'m','a','t',0});std::vector<uint8_t> names(4);lump(44,names);
    auto faces=[&](bool hdr){std::vector<uint8_t> out(2*56);const size_t p=56;u32(out,p+4,hdr?3:0);u16(out,p+8,hdr?4:3);u16(out,p+10,hdr?1:0);u16(out,p+12,uint16_t(-1));out[p+16]=0;out[p+17]=255;u32(out,p+20,hdr?8:4);u32(out,p+28,hdr?4:uint32_t(-1));u32(out,p+32,hdr?uint32_t(-2):2);u32(out,p+36,hdr?1:1);u32(out,p+40,hdr?1:0);return out;};
    lump(7,faces(false),1);std::vector<uint8_t> models(48);f32(models,12,4);f32(models,16,4);f32(models,20,1);u32(models,40,1);u32(models,44,1);lump(14,models);
    std::vector<uint8_t> ldr(4+2*4);const uint8_t ldrSamples[8]={11,22,33,0,44,55,66,1};std::memcpy(ldr.data()+4,ldrSamples,8);lump(8,ldr,1);
    std::vector<uint8_t> hdrFaces=faces(true);lump(58,hdrFaces,1);
    std::vector<uint8_t> hdr(8+4*4);const uint8_t hdrSamples[16]={91,82,73,3,64,55,46,4,37,28,19,5,10,20,30,6};std::memcpy(hdr.data()+8,hdrSamples,16);lump(53,hdr,1);
    return b;
}
static bool sameBrush(const SourceBspBrushGeometryCandidate&a,const SourceBspBrushGeometryCandidate&b){
    if(a.positions!=b.positions||a.normals!=b.normals||a.uvs!=b.uvs||a.lightColors!=b.lightColors||a.materials!=b.materials||a.triangles.size()!=b.triangles.size())return false;
    for(size_t i=0;i<a.triangles.size();++i)if(a.triangles[i].vertices!=b.triangles[i].vertices||a.triangles[i].material!=b.triangles[i].material)return false;return true;
}
template<class F>static void refuses(F f,const char* message){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,message);}

int main(){try{
    static_assert(sizeof(void*)==8,"x64 fixture required");auto b=fixture();const auto original=b;
    auto legacy=readSourceBspInlineBrushCandidate(b,0);auto legacyExplicit=readSourceBspInlineBrushCandidate(b,0,nullptr,SourceBspLightmapSelectionCandidate::LegacyPreview);check(sameBrush(legacy,legacyExplicit),"default/explicit legacy geometry mismatch");
    SourceBspBrushEmissionTraceCandidate ldrTrace,hdrTrace;std::vector<uint32_t> ldrFaces,hdrFaces;std::vector<std::array<float,3>> ldrPositions,hdrPositions;
    ldrTrace.vertex=[&](uint32_t f,const auto&p){ldrFaces.push_back(f);ldrPositions.push_back(p);};hdrTrace.vertex=[&](uint32_t f,const auto&p){hdrFaces.push_back(f);hdrPositions.push_back(p);};
    auto ldr=readSourceBspInlineBrushCandidate(b,0,&ldrTrace,SourceBspLightmapSelectionCandidate::Ldr);auto hdr=readSourceBspInlineBrushCandidate(b,0,&hdrTrace,SourceBspLightmapSelectionCandidate::Hdr);
    check(ldr.positions.size()==3&&ldr.triangles.size()==1&&hdr.positions.size()==4&&hdr.triangles.size()==2,"different LDR/HDR topology not selected");
    check(ldrFaces==std::vector<uint32_t>({1,1,1})&&hdrFaces==std::vector<uint32_t>({1,1,1,1}),"global face prefix trace IDs");
    check(ldrPositions[1]==std::array<float,3>{2,0,0}&&hdrPositions[2]==std::array<float,3>{2,2,0},"source base position selection");
    auto ldrBound=readSourceBspLightmappedBrushCandidate(b,0,SourceBspLightmapSelectionCandidate::Ldr);auto hdrBound=readSourceBspLightmappedBrushCandidate(b,0,SourceBspLightmapSelectionCandidate::Hdr);
    check(ldrBound.lightmaps->faceLump()==7&&ldrBound.lightmaps->lightingLump()==8&&hdrBound.lightmaps->faceLump()==58&&hdrBound.lightmaps->lightingLump()==53,"bound readers did not share selected table");
    check(ldrBound.geometry.positions.size()==3&&hdrBound.geometry.positions.size()==4,"bound geometry table mismatch");
    check(ldrBound.lightmaps->face(0).texInfoIndex==0&&ldrBound.lightmaps->face(0).minima==std::array<int32_t,2>{-1,2}&&ldrBound.lightmaps->face(0).axes[0][3]==10,"LDR axes/minima independent expectation");
    check(hdrBound.lightmaps->face(0).texInfoIndex==1&&hdrBound.lightmaps->face(0).minima==std::array<int32_t,2>{4,-2}&&hdrBound.lightmaps->face(0).axes[0][0]==2&&hdrBound.lightmaps->face(0).axes[0][3]==100,"HDR axes/minima independent expectation");
    check(ldrBound.lightmaps->sample(0,0,0,0,0)==std::array<uint8_t,4>{11,22,33,0}&&ldrBound.lightmaps->sample(0,0,0,1,0)==std::array<uint8_t,4>{44,55,66,1},"LDR raw RGBExp32 bytes/offset");
    check(hdrBound.lightmaps->sample(0,0,0,0,0)==std::array<uint8_t,4>{91,82,73,3}&&hdrBound.lightmaps->sample(0,0,0,1,1)==std::array<uint8_t,4>{10,20,30,6},"HDR raw RGBExp32 bytes/offset");
    check(ldrBound.vertexLuxels[0].luxel==std::array<double,2>{11,18}&&hdrBound.vertexLuxels[0].luxel==std::array<double,2>{96,202},"selected source-base luxel table");
    check(readSourceBspLightmappedBrushCandidate(b,0).lightmaps->faceLump()==7&&readSourceBspLightmappedBrushCandidate(b,0,SourceBspLightmapSelectionCandidate::LegacyPreview).lightmaps->faceLump()==7,"legacy LDR preference");
    auto hdrOnly=b;u32(hdrOnly,12+8*16,0);check(readSourceBspInlineBrushCandidate(hdrOnly,0,nullptr,SourceBspLightmapSelectionCandidate::LegacyPreview).positions.size()==4,"legacy HDR-only pairing");
    auto ldrOnly=b;u32(ldrOnly,12+58*16,0);u32(ldrOnly,12+53*16,0);check(readSourceBspLightmappedBrushCandidate(ldrOnly,0,SourceBspLightmapSelectionCandidate::Ldr).geometry.positions.size()==3,"LDR-only explicit selection");refuses([&]{readSourceBspLightmappedBrushCandidate(ldrOnly,0,SourceBspLightmapSelectionCandidate::Hdr);},"strict HDR accepted absent face table");
    auto missingHdrFaces=b;u32(missingHdrFaces,12+8*16,0);u32(missingHdrFaces,12+58*16,0);check(readSourceBspLightmappedBrushCandidate(missingHdrFaces,0).lightmaps->faceLump()==7&&!readSourceBspLightmappedBrushCandidate(missingHdrFaces,0).lightmaps->pairedFaceLightingTables(),"legacy HDR lighting without HDR faces");
    refuses([&]{readSourceBspInlineBrushCandidate(missingHdrFaces,0,nullptr,SourceBspLightmapSelectionCandidate::Hdr);},"strict HDR used LDR faces");refuses([&]{readSourceBspLightmapDataCandidate(missingHdrFaces,0,SourceBspLightmapSelectionCandidate::Hdr);},"raw strict HDR used LDR faces");
    auto invalidHdrTable=b;u32(invalidHdrTable,16+58*16,0);refuses([&]{readSourceBspInlineBrushCandidate(invalidHdrTable,0,nullptr,SourceBspLightmapSelectionCandidate::Hdr);},"strict HDR accepted invalid face table version");
    auto shortHdr=b;u32(shortHdr,12+58*16,56);refuses([&]{readSourceBspLightmappedBrushCandidate(shortHdr,0,SourceBspLightmapSelectionCandidate::Hdr);},"selected HDR model span outside face table accepted");
    refuses([&]{readSourceBspInlineBrushCandidate(b,0,nullptr,SourceBspLightmapSelectionCandidate(77));},"geometry accepted unknown selection");refuses([&]{readSourceBspLightmappedBrushCandidate(b,0,SourceBspLightmapSelectionCandidate(77));},"binding accepted unknown selection");
    auto noStyles=b;const auto f=get32(noStyles,8+7*16)+56;noStyles[f+16]=255;u32(noStyles,f+20,uint32_t(-1));const auto ldrNoStyle=readSourceBspLightmappedBrushCandidate(noStyles,0,SourceBspLightmapSelectionCandidate::Ldr);check(ldrNoStyle.lightmaps->face(0).availability==SourceBspLightmapAvailabilityCandidate::MissingOffset,"no-light/no-style raw availability changed");
    auto emptyLighting=b;const auto emptyFace=get32(emptyLighting,8+7*16)+56;emptyLighting[emptyFace+16]=255;u32(emptyLighting,12+8*16,0);const auto empty=readSourceBspLightmappedBrushCandidate(emptyLighting,0,SourceBspLightmapSelectionCandidate::Ldr);check(empty.geometry.positions.size()==3&&empty.lightmaps->face(0).availability==SourceBspLightmapAvailabilityCandidate::NoStyles,"empty light table/no-style geometry incorrectly rejected");
    auto saved=hdrBound;auto bad=b;u32(bad,12+58*16,0);refuses([&]{auto next=readSourceBspLightmappedBrushCandidate(bad,0,SourceBspLightmapSelectionCandidate::Hdr);saved=std::move(next);},"failed selected prepare committed");check(saved.lightmaps==hdrBound.lightmaps&&saved.geometry.positions.size()==4&&saved.lightmaps->sample(0,0,0,0,0)==std::array<uint8_t,4>{91,82,73,3},"failed prepare damaged prior owner");
    check(b==original,"reader changed input BSP bytes");b.clear();b.shrink_to_fit();check(hdrBound.lightmaps->sample(0,0,0,0,0)==std::array<uint8_t,4>{91,82,73,3}&&hdrBound.vertexLuxels[0].luxel==std::array<double,2>{96,202},"bound result borrowed input storage");
    std::cout<<"paired_bsp_dataset=PASS ldr_vertices=3 hdr_vertices=4 global_face_prefix=1 topology_diff=1 offsets_axes_minima_samples=independent legacy_default=1 strict_hdr=1 rollback=1 input_immutable=1 owned_fixture_only=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
