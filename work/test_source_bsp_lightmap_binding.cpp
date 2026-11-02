#ifdef SFM64_BSP_BINDING_BASELINE
#include "before-source_bsp_inline_brush_candidate.h"
#else
#include "source_bsp_lightmap_binding_candidate.h"
#endif
#include <fstream>
#include <iostream>
#include <type_traits>

static void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
static void word(std::vector<uint8_t>&b,size_t p,uint32_t n){std::memcpy(b.data()+p,&n,4);}
static void number(std::vector<uint8_t>&b,size_t p,float n){std::memcpy(b.data()+p,&n,4);}
static void half(std::vector<uint8_t>&b,size_t p,uint16_t n){std::memcpy(b.data()+p,&n,2);}
static uint32_t wordAt(const std::vector<uint8_t>&b,size_t p){uint32_t n;std::memcpy(&n,b.data()+p,4);return n;}
static std::vector<uint8_t> fixture(bool multi,bool displaced){
    std::vector<uint8_t>b(1036);std::memcpy(b.data(),"VBSP",4);word(b,4,20);
    auto lump=[&](uint32_t id,const std::vector<uint8_t>& data,uint32_t version=0){word(b,8+16*id,uint32_t(b.size()));word(b,12+16*id,uint32_t(data.size()));word(b,16+16*id,version);b.insert(b.end(),data.begin(),data.end());};
    std::vector<std::array<float,3>> positions;
    if(multi)positions={{0,0,0},{2,0,0},{1,1,0},{10,0,0},{12,0,0},{11,1,0},{20,0,0},{21,0,0},{22,0,0}};
    else if(displaced)positions={{0,0,0},{0,2,0},{2,2,0},{2,0,0}};
    else positions={{0,0,0},{2,0,0},{1,1,0}};
    std::vector<uint8_t>vertices(positions.size()*12);for(size_t n=0;n<positions.size();++n)for(size_t c=0;c<3;++c)number(vertices,n*12+c*4,positions[n][c]);lump(3,vertices);
    const uint32_t polygons=multi?3:1,corners=displaced?4:3;
    std::vector<uint8_t>edges((1+polygons*corners)*4),surfedges(polygons*corners*4);
    for(uint32_t p=0;p<polygons;++p)for(uint32_t c=0;c<corners;++c){const auto e=1+p*corners+c;half(edges,e*4,uint16_t(p*corners+c));half(edges,e*4+2,uint16_t(p*corners+(c+1)%corners));word(surfedges,(p*corners+c)*4,e);}lump(12,edges);lump(13,surfedges);
    const uint32_t infos=multi?3:1;
    std::vector<uint8_t>info(infos*72);
    for(uint32_t i=0;i<infos;++i){number(info,i*72,1);number(info,i*72+20,1);const std::array<float,8> axes=i==1?std::array<float,8>{1,-.5f,.25f,-3,.25f,-.25f,.5f,-2}:std::array<float,8>{.5f,.25f,.125f,-2,-.25f,.5f,-.125f,-4};for(size_t n=0;n<8;++n)number(info,i*72+32+n*4,axes[n]);word(info,i*72+64,0x800);}lump(6,info);
    std::vector<uint8_t>td(32);for(size_t p:{16u,20u,24u,28u})word(td,p,64);lump(2,td);lump(43,{'t','e','s','t','/','s','a','m','e',0});lump(44,std::vector<uint8_t>(4));
    const uint32_t faceCount=multi?4:1;
    std::vector<uint8_t>faces(faceCount*56);
    for(uint32_t f=0;f<faceCount;++f){const auto p=f*56;const uint32_t poly=multi&&f?f-1:0;word(faces,p+4,poly*corners);half(faces,p+8,uint16_t(corners));half(faces,p+10,uint16_t(multi&&f?f-1:0));half(faces,p+12,displaced?0:UINT16_MAX);faces[p+16]=0;faces[p+17]=3;faces[p+18]=faces[p+19]=255;word(faces,p+20,16);word(faces,p+28,uint32_t(f==2?-5:-3));word(faces,p+32,uint32_t(f==2?-7:-5));word(faces,p+36,1);word(faces,p+40,1);if(multi&&f==2){word(faces,p+20,UINT32_MAX);faces[p+16]=255;}}
    lump(7,faces,1);
    std::vector<uint8_t>models(multi?96:48);for(size_t p=0;p<models.size();p+=48){number(models,p+12,32);number(models,p+16,4);number(models,p+20,8);word(models,p+40,p?1:0);word(models,p+44,p?3:faceCount);}lump(14,models);
    std::vector<uint8_t>lighting(512);for(size_t n=0;n<128;++n){lighting[4*n]=uint8_t(10+n%20);lighting[4*n+1]=uint8_t(40+n%20);lighting[4*n+2]=uint8_t(70+n%20);lighting[4*n+3]=255;}lump(8,lighting,1);
    if(displaced){std::vector<uint8_t>d(176);word(d,20,1);lump(26,d);std::vector<uint8_t>v(9*20);for(size_t n=0;n<9;++n){number(v,n*20+8,1);number(v,n*20+12,3);number(v,n*20+16,float(n*11));}lump(33,v);std::vector<uint8_t>t(16);half(t,0,1u<<5);lump(48,t);}
    return b;
}
template<class T>static void write(std::ostream&out,T value){static_assert(std::is_arithmetic<T>::value,"field-wise output only");out.write(reinterpret_cast<const char*>(&value),sizeof(value));}
static void manifest(const SourceBspBrushGeometryCandidate& g,const std::filesystem::path&path){
    std::ofstream out(path,std::ios::binary);
    for(const auto* values:{&g.positions,&g.normals,&g.lightColors}){write(out,uint64_t(values->size()));for(const auto&v:*values)for(float f:v)write(out,f);}
    write(out,uint64_t(g.uvs.size()));for(const auto&v:g.uvs)for(float f:v)write(out,f);
    write(out,uint64_t(g.displacementAlphas.size()));for(float f:g.displacementAlphas)write(out,f);
    write(out,uint64_t(g.materials.size()));for(const auto&v:g.materials){write(out,uint64_t(v.size()));out.write(v.data(),std::streamsize(v.size()));}
    write(out,uint64_t(g.triangles.size()));for(const auto&t:g.triangles){for(auto v:t.vertices)write(out,v);write(out,t.material);}
    for(auto n:{g.skippedDegenerateTriangles,g.displacementFaces,g.displacementVertices,g.displacementTrianglesRemoved})write(out,uint64_t(n));
    for(const auto&v:{g.boundsMin,g.boundsMax,g.modelOrigin})for(float f:v)write(out,f);
    const auto model=sourceBspStaticModelCandidate(g);write(out,uint64_t(model.geometry.vertices.size()));for(const auto&v:model.geometry.vertices)out.write(reinterpret_cast<const char*>(v.data()),std::streamsize(v.size()));
    write(out,uint64_t(model.meshes.size()));for(const auto&m:model.meshes){write(out,m.material);write(out,uint64_t(m.triangles.size()));for(const auto&t:m.triangles)for(auto v:t)write(out,v);}
    check(bool(out),"geometry manifest write");
}
#ifndef SFM64_BSP_BINDING_BASELINE
template<class F>static void refuses(F call,const char*message){bool failed=false;try{call();}catch(const std::exception&){failed=true;}check(failed,message);}
static bool sameBindings(const SourceBspLightmappedBrushCandidate&a,const SourceBspLightmappedBrushCandidate&b){
    if(a.lightmaps!=b.lightmaps||a.triangleFaces!=b.triangleFaces||a.vertexLuxels.size()!=b.vertexLuxels.size())return false;
    for(size_t n=0;n<a.vertexLuxels.size();++n)if(a.vertexLuxels[n].faceIndex!=b.vertexLuxels[n].faceIndex||a.vertexLuxels[n].luxel!=b.vertexLuxels[n].luxel)return false;
    return true;
}
static void bindings(const SourceBspLightmappedBrushCandidate&r,const std::filesystem::path&path){
    std::ofstream out(path,std::ios::binary);write(out,uint64_t(r.vertexLuxels.size()));for(const auto&v:r.vertexLuxels){write(out,v.faceIndex);for(double x:v.luxel)write(out,x);}write(out,uint64_t(r.triangleFaces.size()));for(auto f:r.triangleFaces)write(out,f);check(bool(out),"binding output");
}
static size_t controlled(const std::filesystem::path&dir){
    auto b=fixture(true,false);auto result=readSourceBspLightmappedBrushCandidate(b,1);check(result.geometry.positions.size()==9&&result.geometry.triangles.size()==2&&result.geometry.skippedDegenerateTriangles==1,"multi-face geometry fixture");
    check(result.lightmaps->firstFace()==1&&result.geometry.materials.size()==1&&result.triangleFaces==std::vector<uint32_t>{1,2},"global source IDs/same material/degenerate skip");
    for(size_t n=0;n<9;++n){const auto&v=result.vertexLuxels[n];const auto&p=result.geometry.positions[n];const uint32_t face=uint32_t(n/3+1);const std::array<double,2> expected=face==2?std::array<double,2>{2+p[0]-.5*p[1]+.25*p[2],5+.25*p[0]-.25*p[1]+.5*p[2]}:std::array<double,2>{1+.5*p[0]+.25*p[1]+.125*p[2],1-.25*p[0]+.5*p[1]-.125*p[2]};check(v.faceIndex==face&&v.luxel==expected,"independent ordinary face/luxel mapping");}
    check(result.lightmaps->face(1).availability==SourceBspLightmapAvailabilityCandidate::MissingOffset,"unlit vertices lose source identity");
    manifest(result.geometry,dir/"multi-bound.geometry");bindings(result,dir/"multi.bindings");
    auto displaced=readSourceBspLightmappedBrushCandidate(fixture(false,true),0);check(displaced.geometry.positions.size()==9&&displaced.geometry.triangles.size()==7&&displaced.geometry.displacementTrianglesRemoved==1,"removed displacement triangle fixture");
    for(size_t n=0;n<9;++n){const double x=double(n%3),y=double(n/3);check(displaced.vertexLuxels[n].faceIndex==0&&displaced.vertexLuxels[n].luxel==std::array<double,2>{1+.5*x+.25*y,1-.25*x+.5*y},"independent undisplaced-grid luxels");check(displaced.geometry.positions[n][2]==3,"nonzero fixture displacement");check(displaced.vertexLuxels[n].luxel!=displaced.lightmaps->luxel(0,displaced.geometry.positions[n]),"elevated vertex used for luxels");}
    manifest(displaced.geometry,dir/"displaced-bound.geometry");bindings(displaced,dir/"displaced.bindings");
    const auto saved=result;const auto sample=result.lightmaps->sample(0,1,3,1,1);b.assign(b.size(),99);b.clear();b.shrink_to_fit();check(sameBindings(saved,result)&&result.lightmaps->sample(0,1,3,1,1)==sample,"input lifetime invalidates bindings");
    auto broken=fixture(true,false);word(broken,wordAt(broken,8+13*16)+6*4,0x80000000);size_t emitted=0;SourceBspBrushEmissionTraceCandidate trace;trace.vertex=[&](uint32_t,const auto&){++emitted;};refuses([&]{readSourceBspInlineBrushCandidate(broken,1,&trace);},"late geometry failure not reached");check(emitted==6,"failure did not follow partial emission");
    refuses([&]{auto next=readSourceBspLightmappedBrushCandidate(broken,1);result=std::move(next);},"combined bad geometry accepted");check(sameBindings(saved,result),"failed combined prepare replaced committed binding/owner");manifest(result.geometry,dir/"multi-after-failure.geometry");
    size_t rejected=1;
    for(size_t bad=0;bad<7;++bad){auto invalid=result;if(bad==0)invalid.lightmaps.reset();if(bad==1)invalid.vertexLuxels.pop_back();if(bad==2)invalid.triangleFaces.pop_back();if(bad==3)invalid.vertexLuxels[0].faceIndex=0;if(bad==4)invalid.vertexLuxels[0].luxel[0]=NAN;if(bad==5)invalid.triangleFaces[0]=2;if(bad==6)invalid.geometry.triangles[0].vertices[0]=UINT32_MAX;refuses([&]{validateSourceBspLightmapBindingCandidate(invalid);},"invalid binding accepted");++rejected;}
    // Existing default admission remains independent of the stricter opt-in.
    auto partial=fixture(false,false);word(partial,12+8*16,140);(void)readSourceBspInlineBrushCandidate(partial,0);refuses([&]{readSourceBspLightmappedBrushCandidate(partial,0);},"partial directional planes accepted in opt-in");++rejected;
    auto hdr=fixture(false,false);for(size_t n=0;n<4;++n){word(hdr,8+58*16+n*4,wordAt(hdr,8+7*16+n*4));word(hdr,8+53*16+n*4,wordAt(hdr,8+8*16+n*4));}
    check(readSourceBspLightmappedBrushCandidate(hdr,0).lightmaps->faceLump()==7,"LDR-first wrapper selection changed");word(hdr,12+8*16,0);check(readSourceBspLightmappedBrushCandidate(hdr,0).lightmaps->faceLump()==58,"HDR-only wrapper pairing");word(hdr,12+58*16,0);check(!readSourceBspLightmappedBrushCandidate(hdr,0).lightmaps->pairedFaceLightingTables(),"legacy unpaired flag lost");
    return rejected;
}
#endif
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 test required");check(argc==3,"fresh-output read-only-benchmark-map");const std::filesystem::path dir=argv[1];check(std::filesystem::create_directory(dir),"fresh output directory");
    const auto multi=fixture(true,false),displaced=fixture(false,true);manifest(readSourceBspInlineBrushCandidate(multi,1),dir/"multi.geometry");manifest(readSourceBspInlineBrushCandidate(displaced,0),dir/"displaced.geometry");
    {std::ofstream f(dir/"multi.bsp",std::ios::binary);f.write(reinterpret_cast<const char*>(multi.data()),std::streamsize(multi.size()));}{std::ofstream f(dir/"displaced.bsp",std::ios::binary);f.write(reinterpret_cast<const char*>(displaced.data()),std::streamsize(displaced.size()));}
    const auto bytes=readBinaryAssetCandidate(argv[2]);manifest(readSourceBspInlineBrushCandidate(bytes,0),dir/"benchmark-world.geometry");manifest(readSourceBspInlineBrushCandidate(bytes,16),dir/"benchmark-inline.geometry");
#ifndef SFM64_BSP_BINDING_BASELINE
    const auto rejected=controlled(dir);const auto world=readSourceBspLightmappedBrushCandidate(bytes,0),inlined=readSourceBspLightmappedBrushCandidate(bytes,16);manifest(world.geometry,dir/"benchmark-world-bound.geometry");manifest(inlined.geometry,dir/"benchmark-inline-bound.geometry");bindings(world,dir/"benchmark-world.bindings");bindings(inlined,dir/"benchmark-inline.bindings");
    std::cout<<"lightmap_binding=PASS rejected="<<rejected<<" owned_multiface_vertices=9 owned_removed_displacement_triangles=1 owned_degenerate_skips=1 input_lifetime=1 partial_failure_rollback=1 legacy_admission_preserved=1 benchmark_world_vertices="<<world.vertexLuxels.size()<<" benchmark_world_triangles="<<world.triangleFaces.size()<<" inline_vertices="<<inlined.vertexLuxels.size()<<" inline_triangles="<<inlined.triangleFaces.size()<<" original_runtime=ABSENT\n";
#else
    std::cout<<"before_geometry_manifests=PASS original_header_snapshot=1 original_runtime=ABSENT\n";
#endif
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
