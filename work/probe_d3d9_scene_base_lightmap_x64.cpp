// Actual retained BSP/material/VTF -> prepared scene -> indexed GPU bridge.
#define SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "probe_d3d9_prepared_scene_x64.cpp"
#undef SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "retained_source_bsp_world_candidate.h"
#include "fur_scene_upload_candidate.h"

static std::string sceneLightText(std::istream& f){const auto n=word(f);need(n&&n<32768,"owned path length");std::string s(n,'\0');need(bool(f.read(s.data(),n)),"owned path bytes");return s;}
template<class T>static void sceneLightWrite(std::ostream& f,T x){f.write(reinterpret_cast<const char*>(&x),sizeof(x));}
static Graph sceneLightGraph(){Graph g;g.strings={"DmeFilmClip","owned","mapname"};Element e{};e.type=0;e.name=1;Value v;v.text="owned";e.attributes.push_back({2,5,{v}});g.elements.push_back(e);return g;}
static PreparedSessionSceneCandidate sceneLightLoad(const std::filesystem::path& root,const RenderCameraCandidate& camera,uint32_t frame,uint32_t mip){
    PreparedSessionSceneCandidate s;s.camera=camera;s.outputWidth=s.outputHeight=32;
    auto prefix=std::make_shared<ModelRenderInputsCandidate>();ModelRenderMaterialCandidate m;m.source.syntax=parseVmtCandidate("UnlitGeneric { $basetexture owned/prefix }");m.preparedBase=prepareModelTextureSnapshotCandidate("owned/prefix",readBinaryAssetCandidate(root/"materials/owned/prefix.vtf"),0,0);prefix->materials.push_back(m);prefix->indices={0,1,2};prefix->ranges={{0,3,0,0,0,0}};s.materialInputs.appendOwned(prefix);s.materialAlpha={1};s.upload.materials={{0,0}};s.upload.indices={0,1,2};s.upload.opaque={{0,0,0,3,0,0}};
    for(const auto& point:std::array<std::array<float,2>,3>{{{1,26},{5,26},{1,30}}}){s.upload.vertices.push_back({{(point[0]+.5f)/16-1,(point[1]+.5f)/16-1,0},{9,8,7},{.234f,.346f}});SceneVertexShadingCandidate v;v.normal={0,0,1};v.hasNormal=true;v.model=0;v.sourceVertex=uint32_t(s.upload.vertexShading.size());v.origin=SceneShadingOriginCandidate::ModelGeometry;s.upload.vertexShading.push_back(v);}
    {RetainedSourceBspWorldCandidate world({root});world.append(sceneLightGraph(),0,s);}
    // An explicit existing texture-view operation, not a VTF pixel replacement.
    if(frame||mip){auto edited=std::make_shared<ModelRenderInputsCandidate>(*s.materialInputs.ownerAt(1));for(auto& material:edited->materials)material.preparedBase=prepareModelTextureSnapshotCandidate(vmtValuesCandidate(material.source.syntax,"$basetexture").at(0),readBinaryAssetCandidate(material.source.baseTexture),mip,frame);s.materialInputs.owners[1]=edited;s.materialInputs.entries[1]=edited.get();}
    return s;
}
#ifndef SFM64_SCENE_LIGHTMAP_NO_MAIN
int main(int argc,char** argv){try{
    need(argc==3,"owned query fresh-output-directory");const auto out=std::filesystem::absolute(argv[2]);need(std::filesystem::create_directory(out),"fresh scene-lightmap output only");std::ifstream input(argv[1],std::ios::binary);need(bool(input),"query input");const auto count=word(input);need(count&&count<=256,"bounded scene query");
    std::ofstream capture(out/"pixels.bin",std::ios::binary),metadata(out/"metadata.bin",std::ios::binary);size_t preparationFailures=0,submissionFailures=0,invalid=0;
    {SceneDevice context(out);const auto baseline=d3d9SceneLiveResourcesCandidate.load();
        for(uint32_t serial=0;serial<count;++serial){const std::filesystem::path root=sceneLightText(input);const auto frame=word(input),mip=word(input);RenderCameraCandidate camera{};for(float& x:camera.rows)x=number(word(input));std::array<float,3> modulation{};for(float& x:modulation)x=number(word(input));const SceneBaseLightmapOptionsCandidate options(modulation);
            const auto disposable=out/("owned-active-"+std::to_string(serial));need(!std::filesystem::exists(disposable)&&disposable.parent_path()==out,"owned disposable target");std::filesystem::copy(root,disposable,std::filesystem::copy_options::recursive);
            auto scene=sceneLightLoad(disposable,camera,frame,mip);auto plan=prepareSceneBaseLightmapPlanCandidate(scene,options);
            need(scene.upload.bspLightmaps.size()==1&&scene.upload.bspLightmaps[0].firstVertex==3&&scene.upload.bspLightmaps[0].firstIndex==3&&scene.upload.bspLightmaps[0].model==1,"actual retained world prefix association");
            need(plan.planes.size()==3&&plan.opaque.size()==4&&plan.translucent.empty()&&plan.planes[0]->faceIndex()==0&&plan.planes[1]->faceIndex()==2&&plan.planes[2]->faceIndex()==1,"material range split/source face order");
            word(metadata,serial);word(metadata,uint32_t(scene.upload.vertices.size()));word(metadata,uint32_t(scene.upload.indices.size()));for(const auto& v:scene.upload.vertices){for(float x:v.position)sceneLightWrite(metadata,x);for(float x:v.uv)sceneLightWrite(metadata,x);}for(auto index:scene.upload.indices)word(metadata,index);
            word(metadata,uint32_t(plan.opaque.size()));for(const auto& batch:plan.opaque){word(metadata,batch.draw.firstIndex);word(metadata,batch.draw.indexCount);word(metadata,batch.draw.materialSlot);word(metadata,batch.plane==SIZE_MAX?UINT32_MAX:plan.planes.at(batch.plane)->faceIndex());}
            std::vector<std::string> stages,drawStages;auto prepared=D3D9PreparedSceneCandidate::prepare(context.device.get(),scene,[&](const char* s){stages.emplace_back(s);},D3D9ShadingDiagnosticCandidate::Disabled,options);
            need(prepared->lightmapCount()==3,"unique retained GPU lightmap planes");
            // Explicitly bounded disposable fixture deletion; never the input root.
            need(std::filesystem::canonical(disposable).parent_path()==std::filesystem::canonical(out),"resolved disposable remains within fresh output");std::filesystem::remove_all(disposable);
            auto pixels=prepared->render({},[&](const char* s){drawStages.emplace_back(s);});need(prepared->drawCalls()==4,"actual indexed source face draw count");
            if(serial==0){const auto committed=d3d9SceneLiveResourcesCandidate.load();const auto* identity=prepared.get();
                for(bool allocation:{false,true})for(size_t fail=0;fail<stages.size();++fail){size_t at=0;bool refused=false;try{auto next=D3D9PreparedSceneCandidate::prepare(context.device.get(),scene,[&](const char*){if(at++==fail){if(allocation)throw std::bad_alloc();throw std::runtime_error("injected scene-lightmap preparation");}},D3D9ShadingDiagnosticCandidate::Disabled,options);prepared=std::move(next);}catch(const std::exception&){refused=true;}
                    need(refused&&prepared.get()==identity&&d3d9SceneLiveResourcesCandidate==committed&&prepared->render()==pixels,"scene-lightmap preparation transaction");++preparationFailures;}
                for(size_t fail=0;fail<drawStages.size();++fail){size_t at=0;bool refused=false;try{prepared->render({},[&](const char*){if(at++==fail)throw std::runtime_error("injected scene-lightmap submission");});}catch(const std::exception&){refused=true;}
                    need(refused&&d3d9SceneLiveResourcesCandidate==committed,"scene-lightmap failed submission cleanup");for(DWORD n=0;n<2;++n){IDirect3DBaseTexture9* t=nullptr;d3d9SceneCheckCandidate(context.device->GetTexture(n,&t));if(t)t->Release();need(!t,"failed lightmap submission retained texture");}need(prepared->render()==pixels,"failed lightmap submission reusable owner");++submissionFailures;}
                for(int bad=0;bad<28;++bad){auto copy=scene;bool refused=false;try{
                    if(bad==0){auto p=D3D9PreparedSceneCandidate::prepare(context.device.get(),copy);}
                    else if(bad==1){auto p=D3D9PreparedSceneCandidate::prepare(context.device.get(),copy,{},D3D9ShadingDiagnosticCandidate::Normal,options);}
                    else{
                        if(bad==2)copy.upload.bspLightmaps.clear();if(bad==3)copy.upload.vertexShading[3].origin=SceneShadingOriginCandidate::GeneratedGeometry;if(bad==4)copy.upload.vertexShading[3].sourceVertex=99;
                        if(bad==5)copy.upload.bspLightmaps[0].firstIndex=0;if(bad==6)copy.upload.bspLightmaps.push_back(copy.upload.bspLightmaps[0]);
                        if(bad>=7&&bad<=13){auto edited=std::make_shared<ModelRenderInputsCandidate>(*copy.materialInputs.ownerAt(1));auto& m=edited->materials[0];if(bad==7)m.preparedBase.cpu.reset();if(bad==8)m.source.syntax.key="VertexLitGeneric";if(bad==9)m.baseTextureTransform[2]+=.25f;if(bad==10)m.preparedBase.frame=UINT32_MAX;if(bad==11)m.preparedBase.sourceMip=UINT32_MAX;if(bad==12)m.alphaCutoff=.5f;if(bad==13)edited->ranges[0].firstIndex+=3;copy.materialInputs.owners[1]=edited;copy.materialInputs.entries[1]=edited.get();}
                        if(bad==14){auto fur=prepareFurRibbonsCandidate({{{0,0,0},{0,0,1}}},{0,-5,1},.2f,.1f);need(appendOpaqueFurRibbonsCandidate(copy.upload,fur,1),"owned generated fur fixture");}
                        if(bad==15)copy.upload.indices[3]=0;
                        if(bad==16){auto s=std::make_shared<SourceBspLightmapSnapshotCandidate>(*copy.upload.bspLightmaps[0].snapshot);s->state=SourceBspLightmapSnapshotStateCandidate::UnavailableLegacyPreview;s->unavailableReason="owned missing";s->lightmaps.reset();s->vertexLuxels.clear();s->orderedTriangleFaces.clear();copy.upload.bspLightmaps[0].snapshot=s;}
                        if(bad>=17){
                            auto edited=std::make_shared<ModelRenderInputsCandidate>(*copy.materialInputs.ownerAt(1));auto& material=edited->materials[0].source.syntax;
                            VmtNodeCandidate n{"%keywords","owned",{},false};
                            if(bad==17){material.children.push_back(n);material.children.push_back(n);}
                            if(bad==18){n.key="$surfaceprop";material.children.push_back(n);material.children.push_back(n);}
                            if(bad==19){n.block=true;n.children.push_back({"child","value",{},false});}
                            if(bad==20){n.key="$surfaceprop";n.value.clear();}
                            if(bad==21){n.key="$surfaceprop";n.value=std::string("a\0b",3);}
                            if(bad==22)n.key="%compilesky";if(bad==23)n.key="%tooltexture";if(bad==24)n.key="$surfaceprop2";
                            if(bad==25)n.key="HDR?%keywords";if(bad==26)n.value=std::string("\x80",1);if(bad==27)n.value=std::string(4096,'x');
                            if(bad>=19)material.children.push_back(n);
                            copy.materialInputs.owners[1]=edited;copy.materialInputs.entries[1]=edited.get();
                        }
                        auto next=D3D9PreparedSceneCandidate::prepare(context.device.get(),copy,{},D3D9ShadingDiagnosticCandidate::Disabled,options);prepared=std::move(next);
                    }
                }catch(const std::exception&){refused=true;}
                    need(refused&&prepared.get()==identity&&d3d9SceneLiveResourcesCandidate==committed&&prepared->render()==pixels,"invalid scene-lightmap packet preserved commit");++invalid;
                }
            }
            std::weak_ptr<const SourceBspLightmapSnapshotCandidate> source=scene.upload.bspLightmaps[0].snapshot;
            std::weak_ptr<const ShaderTextureCpuPreparationCandidate> texture=scene.materialInputs.at(1).materials[0].preparedBase.cpu;
            plan={};scene={};need(!source.expired()&&!texture.expired()&&prepared->render()==pixels,"source/cache/files lifetime after preparation");
            for(float x:pixels)need(std::isfinite(x),"finite lightmapped scene pixels");word(capture,serial);capture.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*4));
            prepared.reset();need(source.expired()&&texture.expired()&&d3d9SceneLiveResourcesCandidate==baseline,"scene-lightmap resources/source retired");
        }
    }
    need(input.peek()==EOF&&input.eof()&&d3d9SceneLiveResourcesCandidate==0,"complete scene input/COM retirement");capture.flush();metadata.flush();need(bool(capture)&&bool(metadata),"complete scene capture");
    std::cout<<"PASS scene_lightmap_cases="<<count<<" preparation_failures="<<preparationFailures<<" submission_failures="<<submissionFailures<<" invalid_packets="<<invalid<<" actual_indexed_draws=4 unique_planes=3 lifetime_after_scene_cache_files=1 live_resources=0 native_x64=1 original_runtime=ABSENT\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
