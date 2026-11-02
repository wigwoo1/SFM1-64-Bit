// Owned fixtures only. No original engine/runtime/process interaction.
#define SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "probe_d3d9_prepared_scene_x64.cpp"
#undef SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "d3d9_base_lightmap_pass_candidate.h"

static std::string baseText(std::istream& f){const auto n=word(f);need(n&&n<32768,"query path");std::string s(n,'\0');need(bool(f.read(s.data(),n)),"query path bytes");return s;}
int main(int argc,char** argv){try{
    need(argc==3,"query fresh-output-directory");const std::filesystem::path out=argv[2];need(std::filesystem::create_directory(out),"fresh output only");
    std::ifstream input(argv[1],std::ios::binary);need(bool(input),"query open");const auto count=word(input);need(count&&count<=512,"bounded query count");
    std::ofstream capture(out/"pixels.bin",std::ios::binary),metadata(out/"metadata.bin",std::ios::binary);size_t prepFailures=0,renderFailures=0,invalid=0;
    {SceneDevice context(out);const auto baseline=d3d9SceneLiveResourcesCandidate.load();
        for(uint32_t serial=0;serial<count;++serial){
            auto data=bytes(baseText(input));const auto face=word(input);std::array<uint8_t,4> albedo{};need(bool(input.read(reinterpret_cast<char*>(albedo.data()),4)),"query albedo");
            std::array<float,3> modulation{};for(float& x:modulation)x=number(word(input));
            std::array<D3D9BaseLightmapVertexCandidate,3> vertices{};for(auto& v:vertices)for(float& x:v.clip)x=number(word(input));
            auto brush=readSourceBspLightmappedBrushCandidate(data,0);std::vector<uint32_t> indices;
            // Deliberately regroup faces in reverse order before selecting one.
            for(auto it=brush.geometry.triangles.rbegin();it!=brush.geometry.triangles.rend();++it)indices.insert(indices.end(),it->vertices.begin(),it->vertices.end());
            auto snapshot=prepareSourceBspLightmapSnapshotCandidate(std::move(brush),indices);
            auto found=std::find(snapshot->orderedTriangleFaces.begin(),snapshot->orderedTriangleFaces.end(),face);need(found!=snapshot->orderedTriangleFaces.end(),"requested source face");
            const auto triangle=size_t(found-snapshot->orderedTriangleFaces.begin());
            for(size_t k=0;k<3;++k){const auto& v=snapshot->vertexLuxels.at(snapshot->localIndices.at(triangle*3+k));need(v.faceIndex==face,"regrouped face identity");vertices[k].luxel={float(v.luxel[0]),float(v.luxel[1])};}
            auto plane=SourceBspBaseLightmapCandidate::prepare(snapshot,face,parseVmtCandidate("LightmappedGeneric { $basetexture owned/albedo }"),SourceBaseLightmapUnitsCandidate::TexLightLinear);
            word(metadata,serial);word(metadata,face);word(metadata,plane->width());word(metadata,plane->height());for(const auto& v:vertices)metadata.write(reinterpret_cast<const char*>(v.luxel.data()),8);for(const auto& p:plane->pixels())metadata.write(reinterpret_cast<const char*>(p.data()),16);
            std::vector<std::string> prepStages,drawStages;
            auto prepared=D3D9BaseLightmapPassCandidate::prepare(context.device.get(),plane,albedo,modulation,[&](const char* s){prepStages.emplace_back(s);});
            // The committed owner keeps source data alive after all caller data.
            std::weak_ptr<const SourceBspLightmapSnapshotCandidate> lifetime=snapshot;snapshot.reset();plane.reset();data.clear();
            auto pixels=prepared->render(vertices,[&](const char* s){drawStages.emplace_back(s);});need(!lifetime.expired(),"lightmap source lifetime");
            if(serial==0){const auto committed=d3d9SceneLiveResourcesCandidate.load();const auto* identity=prepared.get();
                auto source=lifetime.lock();auto retryPlane=SourceBspBaseLightmapCandidate::prepare(source,face,parseVmtCandidate("LightmappedGeneric { $basetexture owned/albedo }"),SourceBaseLightmapUnitsCandidate::TexLightLinear);source.reset();
                for(bool allocation:{false,true})for(size_t fail=0;fail<prepStages.size();++fail){size_t at=0;bool refused=false;
                    try{auto next=D3D9BaseLightmapPassCandidate::prepare(context.device.get(),retryPlane,albedo,modulation,[&](const char*){if(at++==fail){if(allocation)throw std::bad_alloc();throw std::runtime_error("injected prepare");}});prepared=std::move(next);}catch(const std::exception&){refused=true;}
                    need(refused&&prepared.get()==identity&&d3d9SceneLiveResourcesCandidate==committed,"lightmap staged preparation atomicity");need(prepared->render(vertices)==pixels,"prepare failure changed committed pixels");++prepFailures;}
                for(size_t fail=0;fail<drawStages.size();++fail){size_t at=0;bool refused=false;try{prepared->render(vertices,[&](const char*){if(at++==fail)throw std::runtime_error("injected draw");});}catch(const std::exception&){refused=true;}
                    need(refused&&d3d9SceneLiveResourcesCandidate==committed,"lightmap submission resource cleanup");
                    for(DWORD n=0;n<2;++n){IDirect3DBaseTexture9* texture=nullptr;d3d9SceneCheckCandidate(context.device->GetTexture(n,&texture));if(texture)texture->Release();need(texture==nullptr,"failed draw retained texture binding");}
                    need(prepared->render(vertices)==pixels,"submission failure changed reusable owner");++renderFailures;}
                for(int bad=0;bad<7;++bad){bool refused=false;try{auto v=vertices;if(bad==0)v[0].clip[3]=0;if(bad==1)v[0].luxel[0]=NAN;
                    if(bad<2)prepared->render(v);else{auto m=modulation;auto color=albedo;auto planeInput=retryPlane;
                        if(bad==2)m[0]=-1.f;if(bad==3)m[0]=INFINITY;if(bad==4)m[0]=17.f;if(bad==5)color[3]=128;if(bad==6)planeInput.reset();
                        D3D9BaseLightmapPassCandidate::prepare(context.device.get(),planeInput,color,m);}}
                    catch(const std::exception&){refused=true;}need(refused&&prepared.get()==identity&&d3d9SceneLiveResourcesCandidate==committed&&prepared->render(vertices)==pixels,"invalid lightmap input preserves owner");++invalid;}
                retryPlane.reset();
            }
            for(float x:pixels)need(std::isfinite(x),"finite GPU pixels");word(capture,serial);capture.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*4));
            prepared.reset();need(lifetime.expired()&&d3d9SceneLiveResourcesCandidate==baseline,"lightmap per-case owner cleanup");
        }
    }
    need(input.peek()==EOF&&input.eof()&&d3d9SceneLiveResourcesCandidate==0,"complete query/resources retired");capture.flush();metadata.flush();need(bool(capture)&&bool(metadata),"capture output");
    std::cout<<"PASS lightmap_cases="<<count<<" prepare_failures="<<prepFailures<<" submission_failures="<<renderFailures<<" invalid="<<invalid<<" live_resources=0 native_x64=1 original_runtime=ABSENT\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
