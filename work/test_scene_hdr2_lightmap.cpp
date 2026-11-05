#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "prepared_session_scene_candidate.h"
#include "retained_source_bsp_world_candidate.h"
#include "scene_hdr2_lightmap_candidate.h"
#define main pairedReaderFixtureMain
#include "test_source_bsp_paired_lightmap_selection.cpp"
#undef main
#include <fstream>

static void putFile(const std::filesystem::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));check(bool(f),"owned fixture write");}
static void replace(std::vector<uint8_t>&b,uint32_t id,const std::vector<uint8_t>&v,uint32_t version=0){u32(b,8+id*16,uint32_t(b.size()));u32(b,12+id*16,uint32_t(v.size()));u32(b,16+id*16,version);u32(b,20+id*16,0);b.insert(b.end(),v.begin(),v.end());}
static std::vector<uint8_t> threeFaces(){
 auto b=fixture();const size_t ti=get32(b,8+6*16);for(size_t n=0;n<2;++n){for(size_t k=0;k<8;++k)f32(b,ti+n*72+32+k*4,0);f32(b,ti+n*72+32,.5f);f32(b,ti+n*72+52,.5f);}u32(b,ti+68,1);u32(b,ti+72+68,0);
 const size_t original=get32(b,8+58*16);std::vector<uint8_t> faces(4*56);for(size_t n=1;n<4;++n){std::copy_n(b.data()+original+56,56,faces.data()+56*n);const size_t p=n*56;u16(faces,p+10,n==2?0:1);u32(faces,p+4,n==2?0:3);u16(faces,p+8,n==2?3:4);u32(faces,p+28,0);u32(faces,p+32,0);u32(faces,p+36,n==3?0:1);u32(faces,p+40,1);u32(faces,p+20,uint32_t((n-1)*16));}replace(b,58,faces,1);
 std::vector<uint8_t> colors(48);for(size_t n=0;n<12;++n){colors[n*4+(n%3)]=255;colors[n*4+3]=0;}replace(b,53,colors,1);
 const size_t model=get32(b,8+14*16);u32(b,model+44,3);
 std::vector<uint8_t> td(64),names={'m','a','t',0,'o','t','h','e','r',0},table(8);u32(table,4,4);for(size_t n=0;n<2;++n){u32(td,n*32+12,uint32_t(n));for(size_t k:{16u,20u,24u,28u})u32(td,n*32+k,64);}replace(b,2,td);replace(b,43,names);replace(b,44,table);return b;
}
static PreparedSessionSceneCandidate load(const std::filesystem::path&root,const char*name){
 Graph g;g.strings={"DmeFilmClip","world","mapname"};Element e{};e.type=0;e.name=1;Value v;v.text=name;e.attributes.push_back({2,5,{v}});g.elements.push_back(e);
 PreparedSessionSceneCandidate s;s.camera.rows={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};auto inputs=std::make_shared<ModelRenderInputsCandidate>();inputs->indices={0,1,2};inputs->ranges={{0,3,0,0,0,0}};inputs->materials.resize(1);s.materialInputs.appendOwned(inputs);s.materialAlpha={1};s.upload.materials={{0,0}};s.upload.indices={0,1,2};s.upload.opaque={{0,0,0,3,0,0}};for(size_t n=0;n<3;++n)s.upload.vertices.push_back({{float(n),-1,.5f},{0,0,1},{0,0}});
 RetainedSourceBspWorldCandidate world({root},SourceBspLightmapSelectionCandidate::Hdr);world.append(g,0,s);return s;
}
#ifndef SFM64_HDR2_PLAN_NO_MAIN
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");check(argc==3,"fresh owned fixture directory and VTF");const std::filesystem::path root=argv[1];check(std::filesystem::create_directory(root),"fresh output directory");std::filesystem::create_directories(root/"maps");std::filesystem::create_directories(root/"materials/owned");putFile(root/"materials/owned/pixel.vtf",readBinaryAssetCandidate(argv[2]));for(const char*name:{"mat","other"}){std::ofstream f(root/"materials"/(std::string(name)+".vmt"));f<<"LightmappedGeneric { $basetexture owned/pixel }";}
 auto bytes=threeFaces();putFile(root/"maps/owned.bsp",bytes);auto scene=load(root,"owned");const auto savedIndices=scene.upload.indices;const auto savedVertices=scene.upload.vertices;
 const SourceHdr2ScenePolicyCandidate policy{2,113,.5f,{.5f,.75f,1,1},0,2};
 const SceneHdr2PagePlacementFunctionCandidate layout=[](const auto&,uint32_t,uint32_t,uint32_t){return SceneHdr2PagePlacementCandidate{4,4,1,1,0xa5};};
 auto plan=prepareSceneHdr2LightmapPlanCandidate(scene,policy,layout);
 check(plan->faces.size()==3&&plan->opaque.size()==4&&plan->translucent.empty(),"retained multi-face plan counts");
 const uint32_t ids[]{1,3,2};const uint32_t starts[]{3,9,15};for(size_t n=0;n<3;++n){check(plan->faces[n].accumulated->faceIndex()==ids[n]&&plan->opaque[n+1].draw.firstIndex==starts[n]&&plan->opaque[n+1].faceResource==n,"material regrouped triangle/face identity");}
 check(plan->opaque[0].faceResource==SIZE_MAX&&!plan->opaque[0].unadmittedReason.empty()&&plan->opaque[1].draw.materialSlot==plan->opaque[2].draw.materialSlot&&plan->opaque[3].draw.materialSlot!=plan->opaque[1].draw.materialSlot,"legacy prefix and shared material faces");
 check(plan->lightUvs[3]==std::array<float,2>{.375f,.375f}&&plan->lightUvs[4]==std::array<float,2>{.625f,.375f}&&plan->lightUvs[10]==std::array<float,2>{.375f,.375f}&&plan->lightUvs[13]==std::array<float,2>{.375f,.375f},"literal source-float UV and zero-S both-axis centering");
 check(plan->hdrScale==1&&plan->policy.exposure==2&&plan->modulation.constants.size()==3&&plan->modulation.constants[1].value==std::array<float,4>{.5f,.75f,1,1},"separate style/HDR/instance/exposure units");
 for(const auto& face:plan->faces)check(face.bytes.size()==128,"exact encoded page length");
 size_t checked=0;for(size_t n=0;n<plan->faces.size();++n){const auto&face=plan->faces[n];const auto firstTexel=(ids[n]-1)*4;for(size_t y=0;y<4;++y)for(size_t x=0;x<4;++x)for(size_t k=0;k<8;++k){uint8_t expected=0xa5;if(x>=1&&x<1+face.accumulated->width()&&y>=1&&y<1+face.accumulated->height()){const auto texel=firstTexel+(y-1)*face.accumulated->width()+x-1;const size_t channel=k/2;const uint16_t word=channel==3?0x3c00:(channel==texel%3?0x3800:0);expected=uint8_t(word>>((k%2)*8));}check(face.bytes[(y*4+x)*8+k]==expected,"independent literal half words/guard page bytes");++checked;}}
 check(scene.upload.indices==savedIndices&&scene.upload.vertices.size()==savedVertices.size()&&std::memcmp(scene.upload.vertices.data(),savedVertices.data(),savedVertices.size()*sizeof(savedVertices[0]))==0,"planner mutated scene geometry");
 auto numericPolicy=policy;numericPolicy.encoding=36;auto numeric=prepareSceneHdr2LightmapPlanCandidate(scene,numericPolicy,layout);for(const auto&f:numeric->faces)for(size_t y=0;y<f.accumulated->height();++y)for(size_t x=0;x<f.accumulated->width();++x)for(size_t k=0;k<8;++k)check(f.bytes[((y+1)*4+x+1)*8+k]==0,"format36 low-word literal storage");
 auto unavailable=bytes;const auto fp=get32(unavailable,8+58*16)+2*56;unavailable[fp+16]=255;u32(unavailable,fp+20,uint32_t(-1));putFile(root/"maps/unavailable.bsp",unavailable);auto fallback=prepareSceneHdr2LightmapPlanCandidate(load(root,"unavailable"),policy,layout);check(fallback->faces.size()==2&&fallback->opaque.back().faceResource==SIZE_MAX&&!fallback->opaque.back().unadmittedReason.empty(),"unavailable face preserves legacy batch");
 size_t failures=0;auto committed=plan;std::vector<std::string> stages;prepareSceneHdr2LightmapPlanCandidate(scene,policy,layout,[&](const char*s){stages.emplace_back(s);});for(bool allocation:{false,true})for(size_t fail=0;fail<stages.size();++fail){size_t at=0;refuses([&]{auto next=prepareSceneHdr2LightmapPlanCandidate(scene,policy,layout,[&](const char*){if(at++==fail){if(allocation)throw std::bad_alloc();throw std::runtime_error("owned plan failure");}});committed=next;},"plan failure injection");check(committed==plan&&committed->faces[0].bytes==plan->faces[0].bytes,"failed plan replaced commit");++failures;}
 for(int bad=0;bad<8;++bad){auto broken=scene;auto p=policy;if(bad==0)p.selectedHdrType=1;if(bad==1)p.encoding=99;if(bad==2)p.styleScale=-1;if(bad==3)broken.upload.vertexShading[3].sourceVertex=99;if(bad==4)broken.upload.indices[3]=0;if(bad==5)broken.upload.vertices[3].position[0]=1;if(bad==6)broken.materialAlpha.clear();if(bad==7)broken.upload.opaque[1].range=99;refuses([&]{prepareSceneHdr2LightmapPlanCandidate(broken,p,layout);},"corrupt scene/profile accepted");++failures;}
 const auto control=_mm_getcsr();_mm_setcsr(control|0x8000u);bool fpRejected=false;try{prepareSceneHdr2LightmapPlanCandidate(scene,policy,layout);}catch(const std::exception&){fpRejected=true;}_mm_setcsr(control);check(fpRejected,"unsupported FP control accepted");
 refuses([&]{prepareSceneHdr2LightmapPlanCandidate(scene,policy,[](const auto&,uint32_t,uint32_t,uint32_t){return SceneHdr2PagePlacementCandidate{1,1,1,1,0};});},"invalid page accepted");
 std::weak_ptr<const SourceBspLightmapSnapshotCandidate> lifetime=scene.upload.bspLightmaps[0].snapshot;scene={};check(!lifetime.expired()&&plan->faces[0].accumulated->pixels()[0]==std::array<float,4>{.5f,0,0,1},"source lifetime/accumulated literal");
 numeric.reset();committed.reset();plan.reset();check(lifetime.expired(),"CPU plan failed to release original source");
 std::cout<<"PASS hdr2_cpu_face_plan=1 selected_retained_world=1 regrouped_faces=1,3,2 shared_material=1 encoded_bytes="<<checked<<" rollback_refusals="<<failures<<" repeated_triangles_reuse=1 source_float_uv=1 zero_S_centers_both=1 legacy_unavailable=1 input_geometry_unchanged=1 native_x64=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
