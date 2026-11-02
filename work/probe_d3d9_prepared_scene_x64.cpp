#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "d3d9_prepared_scene_candidate.h"
#include <iostream>
#include <fstream>
#include <set>

using ComScene=D3D9SceneComCandidate<IDirect3DDevice9>;
static void need(bool b,const char*s){d3d9SceneRequireCandidate(b,s);}
static uint32_t word(std::istream&f){uint32_t x=0;need(bool(f.read(reinterpret_cast<char*>(&x),4)),"query word");return x;}
static void word(std::ostream&f,uint32_t x){f.write(reinterpret_cast<const char*>(&x),4);}
#ifndef SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
static std::string text(std::istream&f){const auto n=word(f);need(n&&n<32768,"query string");std::string s(n,'\0');need(bool(f.read(s.data(),n)),"query string bytes");return s;}
#endif
static std::vector<uint8_t>bytes(const std::string&p){std::ifstream f(p,std::ios::binary);need(bool(f),"owned fixture read");return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});}
static float number(uint32_t x){float f;std::memcpy(&f,&x,4);need(std::isfinite(f),"query float");return f;}
struct SceneDevice {
    HWND window=nullptr;D3D9SceneComCandidate<IDirect3D9>api;ComScene device;
    explicit SceneDevice(const std::filesystem::path&out){
        window=CreateWindowExW(0,L"STATIC",L"Owned x64 prepared scene proof",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);need(window&&!IsWindowVisible(window),"hidden proof window");
        try{api.reset(Direct3DCreate9(D3D_SDK_VERSION));need(api.get()!=nullptr,"system D3D9");D3DADAPTER_IDENTIFIER9 identity{};UINT selected=UINT_MAX;D3DDISPLAYMODE mode{};
            for(UINT n=0;n<api->GetAdapterCount();++n){d3d9SceneCheckCandidate(api->GetAdapterIdentifier(n,0,&identity));if(identity.VendorId==4318&&identity.DeviceId==11544){selected=n;break;}}
            need(selected!=UINT_MAX,"measured NVIDIA adapter unavailable");d3d9SceneCheckCandidate(api->GetAdapterDisplayMode(selected,&mode));
            for(auto usage:{DWORD(D3DUSAGE_RENDERTARGET),DWORD(D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)})d3d9SceneCheckCandidate(api->CheckDeviceFormat(selected,D3DDEVTYPE_HAL,mode.Format,usage,D3DRTYPE_SURFACE,D3DFMT_A32B32G32R32F));
            d3d9SceneCheckCandidate(api->CheckDepthStencilMatch(selected,D3DDEVTYPE_HAL,mode.Format,D3DFMT_A32B32G32R32F,D3DFMT_D24S8));
            D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
            device.create([&](auto**p){return api->CreateDevice(selected,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,p);});
            std::ofstream info(out/"device.txt");D3DCAPS9 caps{};d3d9SceneCheckCandidate(device->GetDeviceCaps(&caps));info<<"vendor="<<identity.VendorId<<"\ndevice="<<identity.DeviceId<<"\ndriver="<<identity.Driver<<"\ndriver_version="<<identity.DriverVersion.QuadPart<<"\npointer_bits="<<sizeof(void*)*8<<"\nmax_aniso="<<caps.MaxAnisotropy<<"\nmin_aniso="<<bool(caps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC)<<"\nmag_aniso="<<bool(caps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFANISOTROPIC)<<"\nwindow_visible=0\n";
            for(const auto*name:{L"d3d9.dll",L"d3dcompiler_47.dll"}){auto module=GetModuleHandleW(name);need(module!=nullptr,"system loaded module");wchar_t path[32768]{};need(GetModuleFileNameW(module,path,32768)!=0,"module path");const auto*base=reinterpret_cast<const uint8_t*>(module);const auto*dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);const auto*file=reinterpret_cast<const IMAGE_FILE_HEADER*>(base+dos->e_lfanew+4);need(file->Machine==IMAGE_FILE_MACHINE_AMD64,"native loaded module");info<<std::filesystem::path(name).string()<<"="<<std::filesystem::path(path).string()<<"\n";}
            need(bool(info),"device identity output");
        }catch(...){device.reset();api.reset();DestroyWindow(window);window=nullptr;throw;}
    }
    ~SceneDevice(){device.reset();api.reset();if(window)DestroyWindow(window);}
};

#ifndef SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
static PreparedSessionSceneCandidate fixture(const std::array<uint32_t,8>&q,const std::string&first,const std::string&second){
    auto a=bytes(first),b=bytes(second);PreparedSessionSceneCandidate s;s.outputWidth=s.outputHeight=32;
    s.camera.rows={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};if(q[3]==3)s.camera.rows[3]=4;
    if(q[3]==4)s.camera.rows={1,0,0,0,0,1,0,0,0,0,.5f,0,0,0,1,0};
    if(q[3]==7)s.camera.rows[11]=-2;if(q[3]==8)s.camera.rows[11]=2;
    if(q[3]==9)s.camera.rows={0,-.5f,0,.25f,.75f,0,0,-.125f,0,0,1,0,0,0,0,1};
    const std::array<MaterialUvTransformCandidate,4>uv{{{1,0,.0625f,0,0,1,.0625f,0},{16,0,.0625f,0,0,16,.0625f,0},{16,-1,.0625f,0,16,1,.0625f,0},{8,-.5f,.0625f,0,4,1,.0625f,0}}};
    need(q[6]<uv.size(),"fixture UV pattern");std::vector<SceneGeometryModelCandidate>models(3);
    for(size_t n=0;n<3;++n){auto input=std::make_shared<ModelRenderInputsCandidate>();ModelRenderMaterialCandidate m;
        m.source.syntax=parseVmtCandidate(q[3]==5?"UnlitTwoTexture { $basetexture fixture/a $texture2 fixture/b $additive 1 }":"UnlitGeneric { $basetexture fixture/a }");
        m.colorSpace={q[2]!=0,false};m.preparedBase=prepareModelTextureSnapshotCandidate("fixture/base",n==2?b:a,q[1],q[0]);need(bool(m.preparedBase.cpu),"fixture prepared base");m.baseTextureFlags=m.preparedBase.cpu->image()->sourceVtfFlags();m.baseTextureTransform=uv[q[6]];
        m.alphaCutoff=number(q[5]);m.translucent=n==2&&q[3]==2;m.unlitTwoTexture=q[3]==5;m.additive=m.unlitTwoTexture;
        if(m.unlitTwoTexture){m.preparedSecond=prepareModelTextureSnapshotCandidate("fixture/second",b,q[1],q[0]^1u);m.secondTextureFlags=m.preparedSecond.cpu->image()->sourceVtfFlags();m.secondTextureTransform=m.baseTextureTransform;m.image=decodeVtfMipCandidate(a,q[1],q[0]);m.secondBaseImage=decodeVtfMipCandidate(b,q[1],q[0]^1u);}
        input->materials={m,m};input->indices={0,0,0,0,1,2,0,2,3};input->ranges.push_back({3,6,n==2?0u:1u,0,0,0});
        s.materialInputs.appendOwned(input);auto&model=models[n];model.model.visible=n==1||(n==2&&(q[3]==1||q[3]==2));
        const float extent=n==2?.5f:1.f,z=n==2?.25f:.75f;
        for(auto c:std::array<std::array<float,2>,4>{{{-1,-1},{1,-1},{1,1},{-1,1}}}){GeometryDeformedVertexCandidate v;v.position={extent*c[0],extent*c[1],q[3]==4?c[0]+3:z};v.normal={0,0,1};v.uv={(c[0]+1)/2,(c[1]+1)/2};model.vertices.push_back(v);}
    }
    // Use the existing opacity planner; the foreground retained alpha selects
    // the translucent list even when its shader was authored opaque.
    auto views=prepareSceneMaterialDrawInputsCandidate(s.materialInputs,{});
    if(q[3]==2){for(auto&x:views.models[2].materials)x.translucent=true;}
    s.upload=prepareSceneGpuUploadCandidate(models,views,[&](const auto&v){return s.camera.depth(v.position);});
    for(const auto&addr:s.upload.materials)s.materialAlpha.push_back(addr.model==2?number(q[4]):1.f);
    need(s.upload.opaque.front().firstIndex==3&&s.upload.opaque.front().materialSlot==1&&s.upload.opaque.front().model==1,"existing planner nonzero indexed/material source");
    return s;
}

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"native x64 prepared scene proof");need(argc==4||argc==6,"queries fresh-output-dir owned | session-copy fresh-output-dir benchmark content-dir tick");
    const std::filesystem::path out=argv[2];need(std::filesystem::create_directory(out),"fresh output directory");
    if(std::string(argv[3])=="benchmark"){
        need(argc==6,"benchmark arguments");auto graph=loadGraph(argv[1]);auto root=sessionRenderRootCandidate(graph);const auto content=std::filesystem::path(argv[4]);auto source=bytes((content/"gameinfo.txt").string());std::string syntax(source.begin(),source.end());auto mounts=gameinfoMountsCandidate(syntax,content,content.parent_path());RetainedSessionSceneCandidate scene(std::move(graph),root,mounts,2,true,true);auto frame=scene.sample(int32_t(std::stol(argv[5])),320,180);std::ofstream report(out/"admission.txt");report<<"vertices="<<frame.upload.vertices.size()<<" indices="<<frame.upload.indices.size()<<" materials="<<frame.upload.materials.size()<<" scale_and_name_mapping=existing_editor_optins\n";std::set<std::string>shaders;
        for(const auto&a:frame.upload.materials){const auto&m=frame.materialInputs.at(a.model).materials.at(a.material);report<<"model="<<a.model<<" material="<<a.material<<" shader="<<m.source.syntax.key<<" path="<<m.source.vmt.string()<<"\n";shaders.insert(m.source.syntax.key);}
        try{D3D9PreparedSceneCandidate::validate(frame);report<<"admission=accepted\n";}catch(const std::exception&e){report<<"admission=refused reason="<<e.what()<<"\n";std::cout<<"BENCHMARK_REFUSED "<<e.what()<<" scene_evaluated=1 no_shader_substitution=1\n";return 0;}
        report.close();SceneDevice device(out);auto prepared=D3D9PreparedSceneCandidate::prepare(device.device.get(),frame);auto pixels=prepared->render();std::ofstream image(out/"pixels.bin",std::ios::binary);image.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*4));need(bool(image),"benchmark image output");return 0;
    }
    need(argc==4&&std::string(argv[3])=="owned","owned mode");std::ifstream input(argv[1],std::ios::binary);need(bool(input),"query input");const auto count=word(input);need(count&&count<=4096,"query count");std::ofstream result(out/"pixels.bin",std::ios::binary),states(out/"samplers.bin",std::ios::binary);size_t rollbackChecks=0,invalidChecks=0;
    {SceneDevice device(out);const auto baseline=d3d9SceneLiveResourcesCandidate.load();
        for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,8>q{};for(auto&n:q)n=word(input);const auto first=text(input),second=text(input);auto s=fixture(q,first,second);std::vector<std::string>stages;
            auto prepared=D3D9PreparedSceneCandidate::prepare(device.device.get(),s,[&](const char*stage){stages.push_back(stage);});D3D9PreparedSceneCandidate::Options options{};if(q[7]){const auto&uv=s.materialInputs.at(1).materials[1].baseTextureTransform;options={1,uv[0]/32,uv[4]/32,uv[1]/32,uv[5]/32};}
            auto pixels=prepared->render(options);const size_t expectedDraws=q[3]==1||q[3]==2?2:1;need(prepared->drawCalls()==expectedDraws,"actual indexed draw count");for(auto f:pixels)need(std::isfinite(f),"finite actual pixel");
            word(result,serial);word(result,uint32_t(prepared->drawCalls()));result.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*4));word(states,serial);word(states,uint32_t(s.upload.materials.size()));for(size_t n=0;n<s.upload.materials.size();++n)for(size_t channel=0;channel<2;++channel)for(auto state:prepared->sampler(n,channel))word(states,state);
            if(serial==0){const auto committed=d3d9SceneLiveResourcesCandidate.load();const auto*identity=prepared.get();
                for(size_t fail=0;fail<stages.size();++fail){size_t at=0;bool failed=false;try{auto next=D3D9PreparedSceneCandidate::prepare(device.device.get(),s,[&](const char*){if(at++==fail)throw std::runtime_error("injected preparation failure");});prepared=std::move(next);}catch(const std::exception&){failed=true;}need(failed&&prepared.get()==identity&&d3d9SceneLiveResourcesCandidate==committed,"transaction preserves committed resources");need(prepared->render(options)==pixels,"failed prepare preserves committed pixels");++rollbackChecks;}
                for(size_t bad=0;bad<15;++bad){auto copy=s;if(bad==0)copy.upload.indices.back()=UINT_MAX;if(bad==1)copy.upload.opaque[0].firstIndex=UINT_MAX;if(bad==2)copy.upload.opaque[0].materialSlot=UINT_MAX;if(bad==3)copy.materialAlpha[0]=2;if(bad==4)copy.camera.rows[0]=std::numeric_limits<float>::quiet_NaN();if(bad==5)copy.upload.opaque[0].materialIndex=0;if(bad==6)copy.outputWidth=0;
                    if(bad>=7){auto edited=std::make_shared<ModelRenderInputsCandidate>(*copy.materialInputs.ownerAt(1));auto&m=edited->materials[1];if(bad==7)m.source.syntax.key="VertexLitGeneric";if(bad==8)m.source.syntax.key="UnlitTwoTexture";if(bad==9)m.source.syntax.children.push_back({"$phong","1",{},false});if(bad==10)m.source.syntax.children[0].block=true;if(bad==11)m.preparedBase.frame=UINT_MAX;if(bad==12)m.preparedBase.sourceMip=UINT_MAX;if(bad==13)m.preparedBase.cpu.reset();if(bad==14)copy.upload.vertices[0].uv[0]=std::numeric_limits<float>::infinity();copy.materialInputs.owners[1]=edited;copy.materialInputs.entries[1]=edited.get();}
                    bool failed=false;try{auto next=D3D9PreparedSceneCandidate::prepare(device.device.get(),copy);prepared=std::move(next);}catch(const std::exception&){failed=true;}need(failed&&prepared.get()==identity&&d3d9SceneLiveResourcesCandidate==committed,"invalid packet preserves owner");need(prepared->render(options)==pixels,"invalid packet preserves pixels");++invalidChecks;}
            }
            prepared.reset();need(d3d9SceneLiveResourcesCandidate==baseline,"per-scene COM resources retired");
            if(serial%64==0)std::cout<<"indexed_cases_completed="<<serial+1<<std::endl;
        }
    }
    need(input.peek()==EOF&&input.eof()&&d3d9SceneLiveResourcesCandidate==0,"input framing/all COM resources retired");result.flush();states.flush();need(bool(result)&&bool(states),"complete proof output");std::cout<<"PASS prepared_indexed_cases="<<count<<" rollback_checks="<<rollbackChecks<<" invalid_packet_checks="<<invalidChecks<<" live_resources=0 native_x64=1 original_runtime=ABSENT output=LINEAR_OFFSCREEN\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
