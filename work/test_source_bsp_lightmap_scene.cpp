#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_scene_candidate.h"
#include "retained_source_bsp_world_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "fur_scene_upload_candidate.h"
// Reuse only owned fixtures/helpers; the former entry point is not executed.
#define main bindingFixtureTestMain
#include "test_source_bsp_lightmap_binding.cpp"
#undef main
#ifdef SFM64_SCENE_LIGHTMAP_BASELINE
#define RetainedSourceBspWorldCandidate BeforeRetainedSourceBspWorldCandidate
#include "before-retained_source_bsp_world_candidate.h"
#undef RetainedSourceBspWorldCandidate
using TestedWorld=BeforeRetainedSourceBspWorldCandidate;
#else
using TestedWorld=RetainedSourceBspWorldCandidate;
#endif

static void replaceSceneLump(std::vector<uint8_t>&b,uint32_t id,const std::vector<uint8_t>&data,uint32_t version=0){word(b,8+16*id,uint32_t(b.size()));word(b,12+16*id,uint32_t(data.size()));word(b,16+16*id,version);word(b,20+16*id,0);b.insert(b.end(),data.begin(),data.end());}
static std::vector<uint8_t> sceneFixture(){
    auto b=fixture(true,false);const auto ti=wordAt(b,8+6*16),faces=wordAt(b,8+7*16);
    // Source triangle faces0(B),1(A),2(B) become0,2,1 after material grouping.
    word(b,ti+68,0);word(b,ti+72+68,1);word(b,ti+144+68,0);half(b,faces+56+10,1);half(b,faces+112+10,0);
    const std::string a="test/same",c="test/other";std::vector<uint8_t>names(a.begin(),a.end());names.push_back(0);names.insert(names.end(),c.begin(),c.end());names.push_back(0);replaceSceneLump(b,43,names);
    std::vector<uint8_t>table(8);word(table,4,uint32_t(a.size()+1));replaceSceneLump(b,44,table);
    std::vector<uint8_t>data(64);for(size_t n=0;n<2;++n){word(data,n*32+12,uint32_t(n));for(size_t p:{16u,20u,24u,28u})word(data,n*32+p,64);}replaceSceneLump(b,2,data);return b;
}
static void sceneFile(const std::filesystem::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));check(bool(f),"owned file write");}
static Graph sceneGraph(const std::string&name){Graph g;g.strings={"DmeFilmClip","world","mapname"};Element e{};e.type=0;e.name=1;Value v;v.text=name;e.attributes.push_back({2,5,{v}});g.elements.push_back(e);return g;}
static PreparedSessionSceneCandidate sceneSeed(){
    PreparedSessionSceneCandidate s;s.camera.rows={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};auto inputs=std::make_shared<ModelRenderInputsCandidate>();inputs->indices={0,1,2};inputs->ranges={{0,3,0,0,0,0}};inputs->materials.resize(1);s.materialInputs.appendOwned(inputs);s.materialAlpha={1};s.upload.materials={{0,0}};s.upload.indices={0,1,2};s.upload.opaque={{0,0,0,3,0,0}};for(size_t n=0;n<3;++n)s.upload.vertices.push_back({{float(n),-1,.5},{0,0,1},{0,0}});return s;
}
static void sceneManifest(const PreparedSessionSceneCandidate&s,const std::filesystem::path&path){
    std::ofstream out(path,std::ios::binary);for(float v:s.camera.rows)write(out,v);write(out,uint64_t(s.upload.vertices.size()));for(const auto&v:s.upload.vertices)out.write(reinterpret_cast<const char*>(&v),36);write(out,uint64_t(s.upload.indices.size()));for(auto n:s.upload.indices)write(out,n);
    write(out,uint64_t(s.upload.materials.size()));for(const auto&m:s.upload.materials){write(out,uint64_t(m.model));write(out,m.material);}for(const auto*list:{&s.upload.opaque,&s.upload.translucent}){write(out,uint64_t(list->size()));for(const auto&d:*list){write(out,uint64_t(d.model));write(out,uint64_t(d.range));for(auto n:{d.firstIndex,d.indexCount,d.materialIndex,d.materialSlot})write(out,n);}}
    for(float a:s.materialAlpha)write(out,a);for(const auto&v:s.upload.vertexShading){for(float f:v.normal)write(out,f);for(float f:v.tangent)write(out,f);for(float f:v.previewLightColor)write(out,f);write(out,uint64_t(v.model));write(out,v.sourceVertex);write(out,uint32_t(v.origin));write(out,uint8_t(v.hasNormal));write(out,uint8_t(v.hasTangent));write(out,uint8_t(v.hasPreviewLightColor));}
    for(size_t i=0;i<s.materialInputs.size();++i){const auto&input=s.materialInputs.at(i);for(const auto&m:input.materials){const auto relative=m.source.vmt.lexically_relative(path.parent_path());const auto text=(relative.empty()?m.source.vmt:relative).generic_string();write(out,uint64_t(text.size()));out.write(text.data(),std::streamsize(text.size()));}for(auto index:input.indices)write(out,index);for(const auto&r:input.ranges){for(auto n:{r.firstIndex,r.indexCount,r.materialIndex,r.body,r.model,r.mesh})write(out,n);write(out,r.eyeball);}}
    check(bool(out),"scene manifest write");
}
#ifndef SFM64_SCENE_LIGHTMAP_BASELINE
static void sameScene(const PreparedSessionSceneCandidate&a,const PreparedSessionSceneCandidate&b,const std::filesystem::path&dir){
    sceneManifest(a,dir/"same-a.bin");sceneManifest(b,dir/"same-b.bin");check(readBinaryAssetCandidate(dir/"same-a.bin")==readBinaryAssetCandidate(dir/"same-b.bin"),"failed scene legacy payload changed");check(a.materialInputs.owners==b.materialInputs.owners&&a.upload.bspLightmaps.size()==b.upload.bspLightmaps.size(),"failed scene ownership changed");
    for(size_t n=0;n<a.upload.bspLightmaps.size();++n){const auto&x=a.upload.bspLightmaps[n];const auto&y=b.upload.bspLightmaps[n];check(x.model==y.model&&x.firstVertex==y.firstVertex&&x.vertexCount==y.vertexCount&&x.firstIndex==y.firstIndex&&x.indexCount==y.indexCount&&x.snapshot==y.snapshot,"failed scene association changed");}
}
static void snapshotFile(const SourceBspLightmapSnapshotCandidate&s,const std::filesystem::path&path){
    std::ofstream out(path,std::ios::binary);write(out,s.vertexCount);write(out,uint64_t(s.localIndices.size()));for(auto v:s.localIndices)write(out,v);write(out,uint64_t(s.vertexLuxels.size()));for(const auto&v:s.vertexLuxels){write(out,v.faceIndex);for(double x:v.luxel)write(out,x);}write(out,uint64_t(s.orderedTriangleFaces.size()));for(auto f:s.orderedTriangleFaces)write(out,f);check(bool(out),"snapshot output");
}
static size_t sceneControlled(const std::filesystem::path&dir,const std::vector<uint8_t>&owned){
    const auto graph=sceneGraph("a");PreparedSessionSceneCandidate retained;std::shared_ptr<const SourceBspLightmapSnapshotCandidate>identity;std::weak_ptr<const SourceBspLightmapSnapshotCandidate>lifetime;const SourceBspLightmapSnapshotCandidate*expectedIdentity=nullptr;size_t failures=0;
    {TestedWorld world({dir});const auto prefix=sceneSeed();retained=prefix;world.append(graph,0,retained);validateSceneBspLightmapsCandidate(retained.upload);const auto&a=retained.upload.bspLightmaps.at(0);identity=a.snapshot;lifetime=identity;expectedIdentity=identity.get();check(a.model==1&&a.firstVertex==3&&a.firstIndex==3&&identity->state==SourceBspLightmapSnapshotStateCandidate::Available,"nonzero prefix/world association");check(identity->orderedTriangleFaces==std::vector<uint32_t>{0,2,1},"material-regrouped triangle source IDs");check(identity->vertexLuxels[6].faceIndex==2&&identity->lightmaps->face(2).availability==SourceBspLightmapAvailabilityCandidate::MissingOffset,"unlit face lost identity");
        sceneFile(dir/"maps/a.bsp",{1,2,3});auto repeat=prefix;world.append(graph,0,repeat);check(repeat.upload.bspLightmaps[0].snapshot==identity,"world data reparsed/reallocated between samples");sameScene(retained,repeat,dir);sceneFile(dir/"maps/a.bsp",owned);
        std::vector<std::string>stages;{TestedWorld discovery({dir});auto s=sceneSeed();discovery.append(sceneGraph("b"),0,s,[&](const char*name){stages.emplace_back(name);});}
        check(stages.size()==13,"load/append stage coverage changed");
        for(bool allocation:{false,true})for(size_t fail=0;fail<stages.size();++fail){auto packet=retained;const auto before=packet;size_t stage=0;std::weak_ptr<const SourceBspLightmapSnapshotCandidate>discarded;
            refuses([&]{world.append(sceneGraph("b"),0,packet,[&](const char*name){if(std::string(name)=="append-associated")discarded=packet.upload.bspLightmaps.back().snapshot;if(stage++==fail){if(allocation)throw std::bad_alloc();throw std::runtime_error("injected world prepare/append failure");}});},"injected failure not propagated");sameScene(packet,before,dir);check(discarded.expired(),"failed append retained new metadata");auto after=sceneSeed();world.append(graph,0,after);check(after.upload.bspLightmaps[0].snapshot==identity,"failed staged map change published cache");++failures;
        }
        auto bad=sceneSeed();bad.camera.rows[0]=NAN;const auto saved=bad;refuses([&]{world.append(graph,0,bad);},"late transparency sort did not refuse");sameScene(bad,saved,dir);++failures;
        auto broken=owned;word(broken,wordAt(broken,8+13*16),0x80000000);sceneFile(dir/"maps/bad.bsp",broken);auto packet=sceneSeed();const auto before=packet;refuses([&]{world.append(sceneGraph("bad"),0,packet);},"bad map accepted");sameScene(packet,before,dir);auto after=sceneSeed();world.append(graph,0,after);check(after.upload.bspLightmaps[0].snapshot==identity,"failed map load replaced cache");++failures;
        auto multiple=retained;world.append(sceneGraph("b"),0,multiple);check(multiple.upload.bspLightmaps.size()==2&&multiple.upload.bspLightmaps[0].snapshot==identity&&multiple.upload.bspLightmaps[1].snapshot!=identity,"multiple world associations/lifetime");validateSceneBspLightmapsCandidate(multiple.upload);
        auto incomplete=owned;word(incomplete,12+8*16,140);sceneFile(dir/"maps/partial.bsp",incomplete);auto legacy=sceneSeed();world.append(sceneGraph("partial"),0,legacy);const auto&missing=*legacy.upload.bspLightmaps[0].snapshot;check(missing.state==SourceBspLightmapSnapshotStateCandidate::UnavailableLegacyPreview&&!missing.unavailableReason.empty()&&!missing.lightmaps&&missing.vertexLuxels.empty(),"incomplete legacy metadata falsely available");validateSceneBspLightmapsCandidate(legacy.upload);sceneManifest(legacy,dir/"partial.scene");identity.reset();
    }
    validateSceneBspLightmapsCandidate(retained.upload);check(!lifetime.expired()&&retained.upload.bspLightmaps[0].snapshot.get()==expectedIdentity&&retained.upload.bspLightmaps[0].snapshot.use_count()==1&&retained.upload.bspLightmaps[0].snapshot->lightmaps->sample(0,0,0,0,0)[0]!=0,"snapshot expired with source cache");
    for(size_t bad=0;bad<13;++bad){auto s=retained;auto&a=s.upload.bspLightmaps[0];if(bad==0)a.snapshot.reset();if(bad==1)a.firstVertex=UINT32_MAX;if(bad==2)a.indexCount-=1;if(bad==3)a.vertexCount-=1;if(bad==4)s.upload.indices[a.firstIndex]=0;if(bad==5)a.model=99;if(bad==6)s.upload.bspLightmaps.push_back(a);if(bad==7)s.upload.opaque.back().firstIndex=0;
        if(bad>=8){auto corrupt=std::make_shared<SourceBspLightmapSnapshotCandidate>(*a.snapshot);if(bad==8)corrupt->vertexLuxels[0].faceIndex=UINT32_MAX;if(bad==9)corrupt->vertexLuxels[0].luxel[0]=NAN;if(bad==10)corrupt->orderedTriangleFaces[0]=1;if(bad==11)corrupt->localIndices[0]=UINT32_MAX;if(bad==12)corrupt->state=SourceBspLightmapSnapshotStateCandidate::UnavailableLegacyPreview;a.snapshot=std::move(corrupt);}
        refuses([&]{validateSceneBspLightmapsCandidate(s.upload);},"invalid shared association accepted");++failures;
    }
    auto bound=readSourceBspLightmappedBrushCandidate(owned,0);auto indices=retained.upload.bspLightmaps[0].snapshot->localIndices;std::copy(indices.begin(),indices.begin()+3,indices.begin()+3);refuses([&]{prepareSourceBspLightmapSnapshotCandidate(std::move(bound),indices);},"duplicated regrouped triangle accepted");++failures;
    auto disp=fixture(false,true);sceneFile(dir/"maps/disp.bsp",disp);{TestedWorld world({dir});auto s=sceneSeed();world.append(sceneGraph("disp"),0,s);const auto&snap=*s.upload.bspLightmaps[0].snapshot;check(snap.vertexCount==9&&snap.orderedTriangleFaces.size()==7&&snap.vertexLuxels[4].luxel==std::array<double,2>{1.75,1.25},"displacement base-grid snapshot transport");validateSceneBspLightmapsCandidate(s.upload);}
    auto generated=retained;auto fur=prepareFurRibbonsCandidate({{{0,0,0},{0,0,2}}},{0,-5,1},.2f,.1f);check(appendOpaqueFurRibbonsCandidate(generated.upload,fur,1),"generated fur reuse of world material");validateSceneBspLightmapsCandidate(generated.upload);check(generated.upload.bspLightmaps[0].snapshot==retained.upload.bspLightmaps[0].snapshot,"generated geometry changed world metadata");{TestedWorld world({dir});world.append(sceneGraph("b"),0,generated);validateSceneBspLightmapsCandidate(generated.upload);check(generated.upload.bspLightmaps.size()==2,"generated/world append association scope");}
    std::cout<<"controlled_lightmap_scene=PASS stage_failures=26 validation_failures="<<failures-26<<" cache_identity_reused=1 lifetime_after_cache=1 nonzero_prefix=1 material_reorder=1 unavailable_legacy=1 generated_world_material=1 world_atomic_publish=1\n";return failures;
}
#endif
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8&&sizeof(SceneGpuVertexCandidate)==36,"x64/legacy ABI");check(argc==6,"fresh-output benchmark-map content-directory benchmark-name owned-vtf");const std::filesystem::path dir=argv[1];check(std::filesystem::create_directory(dir),"fresh scene test output");std::filesystem::create_directories(dir/"maps");std::filesystem::create_directories(dir/"materials/test");
    sceneFile(dir/"materials/test/pixel.vtf",readBinaryAssetCandidate(argv[5]));
    {std::ofstream f(dir/"materials/test/same.vmt");f<<"UnlitGeneric { $basetexture test/pixel }";}{std::ofstream f(dir/"materials/test/other.vmt");f<<"UnlitGeneric { $basetexture test/pixel $translucent 1 }";}
    const auto owned=sceneFixture();sceneFile(dir/"maps/a.bsp",owned);sceneFile(dir/"maps/b.bsp",owned);auto partial=owned;word(partial,12+8*16,140);sceneFile(dir/"maps/partial.bsp",partial);
    {TestedWorld world({dir});auto s=sceneSeed();world.append(sceneGraph("a"),0,s);sceneManifest(s,dir/"owned.scene");auto p=sceneSeed();world.append(sceneGraph("partial"),0,p);sceneManifest(p,dir/"partial.scene");}
    const auto content=std::filesystem::path(argv[3]);auto gameinfo=readBinaryAssetCandidate(content/"gameinfo.txt");auto mounts=gameinfoMountsCandidate(std::string(gameinfo.begin(),gameinfo.end()),content,content.parent_path());PreparedSessionSceneCandidate benchmark;
    {TestedWorld world(mounts);const auto prefix=sceneSeed();benchmark=prefix;world.append(sceneGraph(argv[4]),0,benchmark);sceneManifest(benchmark,dir/"benchmark.scene");
#ifndef SFM64_SCENE_LIGHTMAP_BASELINE
        const auto owner=benchmark.upload.bspLightmaps.at(0).snapshot;auto repeat=prefix;world.append(sceneGraph(argv[4]),0,repeat);check(repeat.upload.bspLightmaps[0].snapshot==owner,"benchmark snapshot not reused");sameScene(benchmark,repeat,dir);
#endif
    }
#ifndef SFM64_SCENE_LIGHTMAP_BASELINE
    sceneControlled(dir,owned);validateSceneBspLightmapsCandidate(benchmark.upload);const auto&snapshot=*benchmark.upload.bspLightmaps.at(0).snapshot;check(snapshot.state==SourceBspLightmapSnapshotStateCandidate::Available&&snapshot.vertexCount==42680&&snapshot.localIndices.size()==81441,"benchmark retained metadata counts");snapshotFile(snapshot,dir/"benchmark.snapshot");auto raw=readSourceBspLightmappedBrushCandidate(readBinaryAssetCandidate(argv[2]),0);bindings(raw,dir/"benchmark-source.bindings");
    size_t ranges=0,covered=0;for(const auto*list:{&benchmark.upload.opaque,&benchmark.upload.translucent})for(const auto&draw:*list)if(draw.model==benchmark.upload.bspLightmaps[0].model){++ranges;covered+=draw.indexCount;}
    check(ranges==59&&covered==81441,"benchmark per-range association coverage");std::cout<<"benchmark_lightmap_scene=PASS vertices="<<snapshot.vertexCount<<" triangles="<<snapshot.orderedTriangleFaces.size()<<" faces="<<snapshot.lightmaps->faces().size()<<" raw_lighting_bytes="<<snapshot.lightmaps->rawLighting().size()<<" ranges="<<ranges<<" snapshot_shared=1 source_indices_exact=1 original_runtime=ABSENT\n";
#else
    std::cout<<"before_world_scene=PASS original_world_cache_snapshot=1 original_runtime=ABSENT\n";
#endif
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
