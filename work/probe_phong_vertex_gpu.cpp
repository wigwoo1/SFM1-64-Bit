// Isolated x64 D3D9 vertex-output oracle. Never loads/attaches to SFM.
// Installed vertex programs are unmodified; an owned pixel shader copies four
// TEXCOORD outputs to four float render targets. Explicit controlled state only.
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "studio_skin_declaration_candidate.h"
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

template<class T>struct PhongGpuCom {
    T*p=nullptr;PhongGpuCom()=default;PhongGpuCom(const PhongGpuCom&)=delete;PhongGpuCom&operator=(const PhongGpuCom&)=delete;
    ~PhongGpuCom(){if(p)p->Release();}T**out(){if(p)throw std::runtime_error("COM output already owned");return &p;}T*operator->()const{return p;}
};
static void gpuCheck(HRESULT hr,const char*operation){if(FAILED(hr))throw std::runtime_error(std::string(operation)+" HRESULT="+std::to_string(uint32_t(hr)));}
static std::vector<DWORD>gpuProgram(const std::filesystem::path&path,unsigned version){
    std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("missing vertex program");std::vector<char>b((std::istreambuf_iterator<char>(in)),{});
    if(b.size()<8||b.size()%4||b.size()>262144)throw std::runtime_error("shader byte framing");std::vector<DWORD>words(b.size()/4);std::memcpy(words.data(),b.data(),b.size());
    if(words.front()!=(version==20?0xfffe0200u:0xfffe0300u)||words.back()!=0x0000ffffu)throw std::runtime_error("shader version/end");return words;
}
static constexpr char capturePixelSource[]=R"(
struct Capture {float4 p:COLOR0;float4 n:COLOR1;float4 t:COLOR2;float4 b:COLOR3;};
Capture main(float3 t:TEXCOORD2,float3 b:TEXCOORD3,float3 n:TEXCOORD4,float3 p:TEXCOORD6){
    Capture o;o.p=float4(p,1);o.n=float4(n,1);o.t=float4(t,1);o.b=float4(b,1);return o;
})";
int main(int argc,char**argv){
    static_assert(sizeof(void*)==8,"native x64 required");HWND window=nullptr;
    try{
        const bool capsOnly=argc==3&&std::string(argv[1])=="--caps";
        if(!capsOnly&&argc!=4)throw std::runtime_error("usage: --caps fresh-dir OR shader-dir query-file fresh-dir");
        const std::filesystem::path output=capsOnly?argv[2]:argv[3];if(!std::filesystem::create_directory(output))throw std::runtime_error("fresh GPU output directory required");
        window=CreateWindowExW(0,L"STATIC",L"SFM64 isolated Phong vertex test",WS_POPUP,0,0,16,16,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        if(!window)throw std::runtime_error("hidden fixture window creation");
        {
            PhongGpuCom<IDirect3D9>api;api.p=Direct3DCreate9(D3D_SDK_VERSION);if(!api.p)throw std::runtime_error("Direct3D9 unavailable");
            PhongGpuCom<IDirect3DDevice9>device;UINT adapter=0;bool found=false;D3DCAPS9 caps{};
            for(;adapter<api->GetAdapterCount();++adapter){
                D3DDISPLAYMODE mode{};D3DCAPS9 candidate{};const auto modeHr=api->GetAdapterDisplayMode(adapter,&mode),capsHr=api->GetDeviceCaps(adapter,D3DDEVTYPE_HAL,&candidate);
                if(FAILED(modeHr)||FAILED(capsHr))continue;
                const auto formatHr=api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_SURFACE,D3DFMT_A32B32G32R32F);
                std::cout<<"adapter_capability="<<adapter<<" vs="<<candidate.VertexShaderVersion<<" ps="<<candidate.PixelShaderVersion<<" mrt="<<candidate.NumSimultaneousRTs<<" float_rt="<<uint32_t(formatHr)<<'\n';
                if(FAILED(formatHr)||candidate.VertexShaderVersion<D3DVS_VERSION(3,0)||candidate.PixelShaderVersion<D3DPS_VERSION(3,0)||candidate.NumSimultaneousRTs<4||candidate.MaxVertexShaderConst<256)continue;
                D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=4;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
                const auto hr=api->CreateDevice(adapter,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,device.out());
                std::cout<<"device_attempt="<<adapter<<" result="<<uint32_t(hr)<<'\n';if(SUCCEEDED(hr)){caps=candidate;found=true;break;}
            }
            if(!found)throw std::runtime_error("no compatible D3D9 HAL device with SM3, four float targets and256 constants");
            D3DADAPTER_IDENTIFIER9 id{};gpuCheck(api->GetAdapterIdentifier(adapter,0,&id),"adapter identity");
            std::ofstream metadata(output/"device.txt");metadata<<"adapter="<<adapter<<"\ndescription="<<id.Description<<"\nvendor="<<id.VendorId<<"\ndevice="<<id.DeviceId<<"\ndriver="<<id.Driver<<"\ndriver_version="<<HIWORD(id.DriverVersion.HighPart)<<'.'<<LOWORD(id.DriverVersion.HighPart)<<'.'<<HIWORD(id.DriverVersion.LowPart)<<'.'<<LOWORD(id.DriverVersion.LowPart)<<"\nvs="<<caps.VertexShaderVersion<<"\nps="<<caps.PixelShaderVersion<<"\nmrt="<<caps.NumSimultaneousRTs<<"\nvertex_constants="<<caps.MaxVertexShaderConst<<"\nwindow_visible="<<IsWindowVisible(window)<<"\n";metadata.flush();if(!metadata)throw std::runtime_error("device metadata write");
            std::cout<<"adapter="<<id.Description<<" hidden_window="<<!IsWindowVisible(window)<<" x64=1\n";
            if(!capsOnly){
                std::array<PhongGpuCom<IDirect3DPixelShader9>,2>pixel;
                for(unsigned i=0;i<2;++i){PhongGpuCom<ID3DBlob>code,errors;
                    const auto hr=D3DCompile(capturePixelSource,sizeof(capturePixelSource)-1,"owned-phong-capture",nullptr,nullptr,"main",i?"ps_3_0":"ps_2_0",D3DCOMPILE_ENABLE_STRICTNESS,0,code.out(),errors.out());
                    if(errors.p){std::ofstream log(output/(i?"ps30-compile.log":"ps20-compile.log"),std::ios::binary);log.write(static_cast<const char*>(errors->GetBufferPointer()),std::streamsize(errors->GetBufferSize()));}
                    gpuCheck(hr,"owned capture pixel shader compile");gpuCheck(device->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()),pixel[i].out()),"capture pixel shader creation");
                    std::ofstream bytecode(output/(i?"capture-ps30.bin":"capture-ps20.bin"),std::ios::binary);bytecode.write(static_cast<const char*>(code->GetBufferPointer()),std::streamsize(code->GetBufferSize()));if(!bytecode)throw std::runtime_error("capture pixel bytecode write");
                }
                std::array<PhongGpuCom<IDirect3DSurface9>,4>targets,readbacks;
                for(unsigned i=0;i<4;++i){gpuCheck(device->CreateRenderTarget(4,4,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,targets[i].out(),nullptr),"float target");gpuCheck(device->CreateOffscreenPlainSurface(4,4,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,readbacks[i].out(),nullptr),"float readback");gpuCheck(device->SetRenderTarget(i,targets[i].p),"float target bind");}
                gpuCheck(device->SetDepthStencilSurface(nullptr),"no depth target");const D3DVIEWPORT9 viewport{0,0,4,4,0,1};gpuCheck(device->SetViewport(&viewport),"capture viewport");
                const std::array<std::pair<D3DRENDERSTATETYPE,DWORD>,17>states{{
                    {D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_SEPARATEALPHABLENDENABLE,FALSE},
                    {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_FOGENABLE,FALSE},{D3DRS_LIGHTING,FALSE},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_DITHERENABLE,FALSE},
                    {D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_POINTSCALEENABLE,FALSE},{D3DRS_POINTSPRITEENABLE,FALSE},
                    {D3DRS_COLORWRITEENABLE,15},{D3DRS_COLORWRITEENABLE1,15},{D3DRS_COLORWRITEENABLE2,15},{D3DRS_COLORWRITEENABLE3,15}}};
                for(const auto&s:states)gpuCheck(device->SetRenderState(s.first,s.second),"capture render state");float pointSize=1;DWORD pointBits=0;std::memcpy(&pointBits,&pointSize,4);gpuCheck(device->SetRenderState(D3DRS_POINTSIZE,pointBits),"point size");
                std::array<PhongGpuCom<IDirect3DVertexDeclaration9>,2>declarations;
                for(unsigned compressed=0;compressed<2;++compressed){const auto elements=studioSkinDeclarationCandidate(0x83ull|(2ull<<12)|(4ull<<15)|(2ull<<18)|(uint64_t(compressed)<<10),95,true);
                    gpuCheck(device->CreateVertexDeclaration(reinterpret_cast<const D3DVERTEXELEMENT9*>(elements.data()),declarations[compressed].out()),"verified declaration creation");}
                PhongGpuCom<IDirect3DVertexBuffer9>vertices,flex;
                gpuCheck(device->CreateVertexBuffer(60,0,0,D3DPOOL_MANAGED,vertices.out(),nullptr),"vertex buffer");gpuCheck(device->CreateVertexBuffer(28,0,0,D3DPOOL_MANAGED,flex.out(),nullptr),"flex buffer");
                using Key=std::tuple<unsigned,unsigned,unsigned>;std::map<Key,std::unique_ptr<PhongGpuCom<IDirect3DVertexShader9>>>shaders;
                std::ifstream queries(argv[2],std::ios::binary);std::ofstream samples(output/"gpu-world-frames.bin",std::ios::binary);if(!queries||!samples)throw std::runtime_error("GPU fixture IO");
                std::array<uint8_t,288>q{};size_t cases=0;std::array<float,64>captured{};
                while(queries.read(reinterpret_cast<char*>(q.data()),288)){
                    std::array<uint32_t,4>header{};std::memcpy(header.data(),q.data(),16);const unsigned version=header[0],s=header[1],dynamic=header[2],compressed=dynamic&1;
                    if((version!=20&&version!=30)||s>=8||dynamic>=(version==20?24u:48u)||!(dynamic&2))throw std::runtime_error("GPU query variant domain");
                    const uint8_t*record=q.data()+16;uint32_t profile=0;std::memcpy(&profile,record,4);if(profile!=(version==30?2u:0u)+compressed)throw std::runtime_error("GPU profile mismatch");
                    std::array<uint8_t,3>indices{};std::memcpy(indices.data(),&header[3],3);if((header[3]>>24)||indices[0]>=53||indices[1]>=53||indices[2]>=53||indices[0]==indices[1]||indices[0]==indices[2]||indices[1]==indices[2])throw std::runtime_error("GPU palette indices");
                    const Key key{version,s,dynamic};auto foundShader=shaders.find(key);
                    if(foundShader==shaders.end()){auto shader=std::make_unique<PhongGpuCom<IDirect3DVertexShader9>>();auto bytes=gpuProgram(std::filesystem::path(argv[1])/("phong-vs"+std::to_string(version)+"-s"+std::to_string(s)+"-d"+std::to_string(dynamic)+".bin"),version);gpuCheck(device->CreateVertexShader(bytes.data(),shader->out()),"installed vertex shader creation");foundShader=shaders.emplace(key,std::move(shader)).first;}
                    gpuCheck(device->SetVertexShader(foundShader->second->p),"installed vertex shader bind");gpuCheck(device->SetPixelShader(pixel[version==30].p),"capture pixel shader bind");gpuCheck(device->SetVertexDeclaration(declarations[compressed].p),"verified vertex declaration bind");
                    std::array<uint8_t,60>vertex{};size_t at=0;auto append=[&](const void*p,size_t n){if(at+n>vertex.size())throw std::runtime_error("vertex packing extent");std::memcpy(vertex.data()+at,p,n);at+=n;};
                    append(record+184,12);append(record+148,compressed?4:8);append(&header[3],4);append(record+156,compressed?4:12);const float uv[2]{0,0};append(uv,8);if(!compressed)append(record+168,16);
                    if(at!=(compressed?32u:60u))throw std::runtime_error("vertex stride");void*mapped=nullptr;gpuCheck(vertices->Lock(0,60,&mapped,0),"vertex lock");std::memcpy(mapped,vertex.data(),60);gpuCheck(vertices->Unlock(),"vertex unlock");
                    std::array<float,7>deltas{};std::memcpy(deltas.data(),record+196,12);std::memcpy(deltas.data()+4,record+208,12);gpuCheck(flex->Lock(0,28,&mapped,0),"flex lock");std::memcpy(mapped,deltas.data(),28);gpuCheck(flex->Unlock(),"flex unlock");
                    gpuCheck(device->SetStreamSource(0,vertices.p,0,UINT(at)),"vertex stream bind");gpuCheck(device->SetStreamSource(2,flex.p,0,28),"flex stream bind");
                    std::array<float,1024>constants{};constants[1]=1;constants[2]=2;constants[3]=0.5f;constants[2*4+2]=100;
                    constants[10*4+3]=0.5f;constants[11*4+3]=1;constants[13*4+2]=1;constants[16*4+2]=1;
                    constants[48*4]=constants[49*4+1]=constants[52*4]=constants[53*4+1]=1;
                    for(unsigned i=0;i<3;++i)std::memcpy(constants.data()+(58+3*indices[i])*4,record+4+48*i,48);
                    std::memcpy(constants.data()+225*4,record+220,4);gpuCheck(device->SetVertexShaderConstantF(0,constants.data(),256),"controlled vertex constants");std::array<BOOL,16>disabled{};gpuCheck(device->SetVertexShaderConstantB(0,disabled.data(),16),"controlled light booleans");
                    gpuCheck(device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"float target clear");gpuCheck(device->BeginScene(),"begin capture");gpuCheck(device->DrawPrimitive(D3DPT_POINTLIST,0,1),"draw installed Phong vertex");gpuCheck(device->EndScene(),"end capture");
                    std::array<float,16>result{};int chosen=-1;
                    for(unsigned target=0;target<4;++target){gpuCheck(device->GetRenderTargetData(targets[target].p,readbacks[target].p),"float GPU readback");D3DLOCKED_RECT lock{};gpuCheck(readbacks[target]->LockRect(&lock,nullptr,D3DLOCK_READONLY),"float readback lock");
                        for(unsigned y=0;y<4;++y)std::memcpy(captured.data()+y*16,static_cast<const uint8_t*>(lock.pBits)+size_t(y)*lock.Pitch,64);
                        gpuCheck(readbacks[target]->UnlockRect(),"float readback unlock");unsigned coverage=0;int foundPixel=-1;
                        for(unsigned i=0;i<16;++i)if(captured[i*4+3]==1){++coverage;foundPixel=int(i);}
                        if(coverage!=1||(chosen!=-1&&chosen!=foundPixel))throw std::runtime_error("capture point coverage/target mismatch at case "+std::to_string(cases)+" target="+std::to_string(target)+" pixels="+std::to_string(coverage));chosen=foundPixel;
                        for(unsigned c=0;c<4;++c){const auto value=captured[size_t(foundPixel)*4+c];if(!std::isfinite(value))throw std::runtime_error("nonfinite native GPU frame");result[target*4+c]=value;}
                    }
                    samples.write(reinterpret_cast<const char*>(result.data()),64);++cases;
                    if(cases%512==0)std::cout<<"gpu_cases_completed="<<cases<<'\n'<<std::flush;
                }
                if(!queries.eof()||queries.gcount()||!cases)throw std::runtime_error("GPU query framing");samples.flush();if(!samples)throw std::runtime_error("GPU sample write failure");
                std::cout<<"PASS gpu_vertex_cases="<<cases<<" shader_slots="<<shaders.size()<<" float_targets=4 exact_point_pixels=1\n";
            }
        }
        DestroyWindow(window);window=nullptr;std::cout<<(capsOnly?"PASS capabilities only\n":"PASS resources released\n");return 0;
    }catch(const std::exception&e){if(window)DestroyWindow(window);std::cerr<<e.what()<<'\n';return 1;}
}
