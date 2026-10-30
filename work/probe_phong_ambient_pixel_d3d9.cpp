// Disposable native x64 D3D9 pixel oracle: installed PS bytecode is unchanged.
// Owned VS supplies constant interpolants; 1x1 float textures; no SFM process/DLL.
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "phong_ambient_pixel_fixture.h"
#include <iostream>
#include <iterator>
#include <string>
template<class T>struct PixelCom {
    T*p=nullptr;PixelCom()=default;PixelCom(const PixelCom&)=delete;PixelCom&operator=(const PixelCom&)=delete;
    ~PixelCom(){if(p)p->Release();}T**out(){if(p)throw std::runtime_error("COM output owned");return &p;}T*operator->()const{return p;}
};
static void check(HRESULT hr,const char*what){if(FAILED(hr))throw std::runtime_error(std::string(what)+" HRESULT="+std::to_string(uint32_t(hr)));}
static constexpr char vertexSource[]=R"(
float4 t:register(c0);float4 b:register(c1);float4 n:register(c2);float4 p:register(c3);
struct Output{float4 clip:POSITION;float2 uv:TEXCOORD0;float3 t:TEXCOORD2;float3 b:TEXCOORD3;float3 n:TEXCOORD4;float3 p:TEXCOORD6;};
Output main(float4 clip:POSITION){Output o;o.clip=clip;o.uv=float2(.5,.5);o.t=t.xyz;o.b=b.xyz;o.n=n.xyz;o.p=p.xyz;return o;}
)";
int main(int argc,char**argv){
    static_assert(sizeof(void*)==8,"native x64 required");HWND window=nullptr;
    try{
        if(argc!=4)throw std::runtime_error("usage: installed-ps20b.bin queries.bin fresh-output-dir");
        const auto queries=phongAmbientPixelQueries(argv[2]);
        std::ifstream program(argv[1],std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(program)),{});
        if(bytes.size()<8||bytes.size()%4||bytes.size()>262144)throw std::runtime_error("pixel shader framing");
        std::vector<DWORD>words(bytes.size()/4);std::memcpy(words.data(),bytes.data(),bytes.size());
        if(words.front()!=0xffff0201u||words.back()!=0xffffu)throw std::runtime_error("admitted ps2x program required");
        const std::filesystem::path output=argv[3];if(!std::filesystem::create_directory(output))throw std::runtime_error("fresh directory required");
        window=CreateWindowExW(0,L"STATIC",L"SFM64 private Phong pixel test",WS_POPUP,0,0,16,16,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        if(!window)throw std::runtime_error("hidden fixture window");
        {
            PixelCom<IDirect3D9>api;api.p=Direct3DCreate9(D3D_SDK_VERSION);if(!api.p)throw std::runtime_error("D3D9 unavailable");
            PixelCom<IDirect3DDevice9>device;UINT adapter=0;bool found=false;
            for(;adapter<api->GetAdapterCount();++adapter){
                D3DDISPLAYMODE mode{};D3DCAPS9 caps{};
                if(FAILED(api->GetAdapterDisplayMode(adapter,&mode))||FAILED(api->GetDeviceCaps(adapter,D3DDEVTYPE_HAL,&caps)))continue;
                if(caps.PixelShaderVersion<D3DPS_VERSION(3,0)||caps.VertexShaderVersion<D3DVS_VERSION(2,0))continue;
                if(FAILED(api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_SURFACE,D3DFMT_A32B32G32R32F))||
                   FAILED(api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,0,D3DRTYPE_TEXTURE,D3DFMT_A32B32G32R32F)))continue;
                D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=4;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.Windowed=TRUE;
                pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
                if(SUCCEEDED(api->CreateDevice(adapter,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,device.out()))){found=true;break;}
            }
            if(!found)throw std::runtime_error("no D3D9 HAL with float sampling/render target");
            D3DADAPTER_IDENTIFIER9 id{};check(api->GetAdapterIdentifier(adapter,0,&id),"adapter identity");
            std::ofstream metadata(output/"device.txt");metadata<<"description="<<id.Description<<"\nvendor="<<id.VendorId<<"\ndevice="<<id.DeviceId<<"\ndriver="<<id.Driver<<"\ndriver_version="<<id.DriverVersion.QuadPart<<"\nwindow_visible="<<IsWindowVisible(window)<<"\n";
            if(!metadata)throw std::runtime_error("metadata write");
            PixelCom<ID3DBlob>vsCode,errors;
            check(D3DCompile(vertexSource,sizeof(vertexSource)-1,"owned-constant-interpolants",nullptr,nullptr,"main","vs_2_0",D3DCOMPILE_ENABLE_STRICTNESS,0,vsCode.out(),errors.out()),"owned vertex compile");
            if(errors.p){std::ofstream log(output/"vertex-compile.log",std::ios::binary);log.write(static_cast<const char*>(errors->GetBufferPointer()),std::streamsize(errors->GetBufferSize()));}
            {std::ofstream code(output/"owned-vertex.bin",std::ios::binary);code.write(static_cast<const char*>(vsCode->GetBufferPointer()),std::streamsize(vsCode->GetBufferSize()));if(!code)throw std::runtime_error("owned vertex bytecode write");}
            PixelCom<IDirect3DVertexShader9>vertex;check(device->CreateVertexShader(static_cast<const DWORD*>(vsCode->GetBufferPointer()),vertex.out()),"owned vertex shader");
            PixelCom<IDirect3DPixelShader9>pixel;check(device->CreatePixelShader(words.data(),pixel.out()),"original pixel shader");
            const D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END()};
            PixelCom<IDirect3DVertexDeclaration9>declaration;check(device->CreateVertexDeclaration(elements,declaration.out()),"declaration");
            PixelCom<IDirect3DSurface9>target,readback;
            check(device->CreateRenderTarget(4,4,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,target.out(),nullptr),"float target");
            check(device->CreateOffscreenPlainSurface(4,4,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,readback.out(),nullptr),"float readback");
            check(device->SetRenderTarget(0,target.p),"target bind");check(device->SetDepthStencilSurface(nullptr),"depth off");
            D3DVIEWPORT9 viewport{0,0,4,4,0,1};check(device->SetViewport(&viewport),"viewport");
            const std::pair<D3DRENDERSTATETYPE,DWORD>states[]={{D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},
                {D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_FOGENABLE,FALSE},{D3DRS_LIGHTING,FALSE},
                {D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_DITHERENABLE,FALSE},{D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_COLORWRITEENABLE,15}};
            for(const auto&s:states)check(device->SetRenderState(s.first,s.second),"render state");
            std::array<PixelCom<IDirect3DTexture9>,2>textures;
            for(unsigned i=0;i<2;++i){const DWORD slot=i?3u:0u;
                check(device->CreateTexture(1,1,1,0,D3DFMT_A32B32G32R32F,D3DPOOL_MANAGED,textures[i].out(),nullptr),"float texture");
                check(device->SetTexture(slot,textures[i].p),"texture bind");
                for(auto type:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER})check(device->SetSamplerState(slot,type,D3DTEXF_POINT),"point sampling");
                check(device->SetSamplerState(slot,D3DSAMP_MIPFILTER,D3DTEXF_NONE),"mip off");
                check(device->SetSamplerState(slot,D3DSAMP_SRGBTEXTURE,FALSE),"linear sampling");
                check(device->SetSamplerState(slot,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP),"clamp U");check(device->SetSamplerState(slot,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP),"clamp V");
            }
            check(device->SetVertexDeclaration(declaration.p),"declaration bind");check(device->SetVertexShader(vertex.p),"vertex bind");check(device->SetPixelShader(pixel.p),"pixel bind");
            const float quad[4][4]{{-1,-1,.5f,1},{-1,1,.5f,1},{1,-1,.5f,1},{1,1,.5f,1}};
            std::ofstream samples(output/"native-pixels.bin",std::ios::binary);size_t cases=0;
            for(const auto&q:queries){
                for(unsigned i=0;i<2;++i){D3DLOCKED_RECT lock{};check(textures[i]->LockRect(0,&lock,nullptr,0),"texture lock");std::memcpy(lock.pBits,i?q.normalSample.data():q.base.data(),16);check(textures[i]->UnlockRect(0),"texture unlock");}
                std::array<std::array<float,4>,32> constants{};constants[1]=q.diffuse;for(unsigned i=0;i<6;++i)constants[4+i]=q.cube[i];
                constants[11]=q.eye;constants[12]=q.fog;constants[27]=q.controls;constants[29]=q.fogColor;constants[30]=q.lightScale;
                check(device->SetPixelShaderConstantF(0,constants[0].data(),32),"pixel constants");
                const std::array<std::array<float,4>,4> interpolants{{q.tangent,q.binormal,q.normal,q.position}};
                check(device->SetVertexShaderConstantF(0,interpolants[0].data(),4),"vertex interpolants");
                check(device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"clear");check(device->BeginScene(),"begin");
                check(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,16),"native pixel draw");check(device->EndScene(),"end");
                check(device->GetRenderTargetData(target.p,readback.p),"readback");D3DLOCKED_RECT lock{};check(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY),"readback lock");
                std::array<float,64> values{};for(unsigned y=0;y<4;++y)std::memcpy(values.data()+y*16,static_cast<const uint8_t*>(lock.pBits)+size_t(y)*lock.Pitch,64);
                check(readback->UnlockRect(),"readback unlock");
                for(unsigned i=0;i<16;++i)for(unsigned j=0;j<4;++j)if(!std::isfinite(values[i*4+j])||values[i*4+j]!=values[j])throw std::runtime_error("constant quad coverage/value mismatch");
                samples.write(reinterpret_cast<const char*>(values.data()),256);++cases;
            }
            samples.flush();if(!samples)throw std::runtime_error("sample write");
            std::cout<<"PASS native_pixel_cases="<<cases<<" pixels_per_case=16 x64=1 adapter="<<id.Description<<" hidden_window="<<!IsWindowVisible(window)<<'\n';
        }
        DestroyWindow(window);window=nullptr;return 0;
    }catch(const std::exception&e){if(window)DestroyWindow(window);std::cerr<<e.what()<<'\n';return 1;}
}
