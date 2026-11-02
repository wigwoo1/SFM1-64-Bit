// Owned sampling experiment: native x64 by default; explicit x86 research build.
// No original SFM process, shader, machine code or runtime DLL is consumed.
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "vtf_texture_layout.h"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

template<class T>struct SamplingCom {
    T*p=nullptr;SamplingCom()=default;SamplingCom(const SamplingCom&)=delete;
    ~SamplingCom(){if(p)p->Release();}
    T**out(){if(p)throw std::runtime_error("COM output already owned");return &p;}
    T*operator->()const{return p;}
};
static void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void check(HRESULT r,const char*s){if(FAILED(r))throw std::runtime_error(std::string(s)+" HRESULT="+std::to_string(uint32_t(r)));}
static uint32_t word(std::istream&f){uint32_t n;require(bool(f.read(reinterpret_cast<char*>(&n),4)),"query word");return n;}
static std::string text(std::istream&f){const auto n=word(f);require(n&&n<4096,"query text length");std::string s(n,'\0');require(bool(f.read(s.data(),n)),"query text");return s;}
static void word(std::ostream&f,uint32_t n){f.write(reinterpret_cast<const char*>(&n),4);}
struct Request{uint32_t frame;float u,v,lod,dxu,dxv,dyu,dyv;};
static_assert(sizeof(Request)==32,"request ABI");
static constexpr char shaderSource[]=R"(
struct Input {float4 position:POSITION;float4 lookup:TEXCOORD0;float frame:TEXCOORD1;float4 grad:TEXCOORD2;};
struct Output {float4 position:POSITION;float4 lookup:TEXCOORD0;float frame:TEXCOORD1;float4 grad:TEXCOORD2;};
Output vertexMain(Input i){Output o;o.position=i.position;o.lookup=i.lookup;o.frame=i.frame;o.grad=i.grad;return o;}
sampler2D first:register(s0);sampler2D second:register(s1);
float4 pixelMain(Output i):COLOR0 {float4 a=tex2Dgrad(first,i.lookup.xy,i.grad.xy,i.grad.zw);float4 b=tex2Dgrad(second,i.lookup.xy,i.grad.xy,i.grad.zw);return i.frame<0.5?a:b;}
)";
struct Vertex{float x,y,z,w,u,v,unused,lod,frame,dxu,dxv,dyu,dyv;};
int main(int argc,char**argv){
#if defined(SFM64_OWNED_D3D9_X86_RESEARCH)
    static_assert(sizeof(void*)==4,"explicit private x86 research only");
#else
    static_assert(sizeof(void*)==8,"native x64 only");
#endif
    HWND window=nullptr;
    try{
        require(argc==5,"usage: queries.bin fresh-output-directory vendor-id device-id");
        const std::filesystem::path output=argv[2];require(std::filesystem::create_directory(output),"fresh output directory required");
        const auto vendor=uint32_t(std::stoul(argv[3])),deviceId=uint32_t(std::stoul(argv[4]));
        std::ifstream in(argv[1],std::ios::binary);require(bool(in),"query file");const auto count=word(in);require(count&&count<=256,"case count");
        window=CreateWindowExW(0,L"STATIC",L"SFM64 owned sampling experiment",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        require(window&&!IsWindowVisible(window),"hidden experiment window");
        {
            SamplingCom<IDirect3D9>api;api.p=Direct3DCreate9(D3D_SDK_VERSION);require(api.p!=nullptr,"system D3D9");
            SamplingCom<IDirect3DDevice9>device;D3DADAPTER_IDENTIFIER9 identity{};UINT adapter=0;D3DDISPLAYMODE mode{};D3DCAPS9 selectedCaps{};bool found=false;
            for(;adapter<api->GetAdapterCount();++adapter){
                check(api->GetAdapterIdentifier(adapter,0,&identity),"adapter identity");
                if(identity.VendorId!=vendor||identity.DeviceId!=deviceId)continue;
                check(api->GetAdapterDisplayMode(adapter,&mode),"adapter display mode");D3DCAPS9 caps{};check(api->GetDeviceCaps(adapter,D3DDEVTYPE_HAL,&caps),"device capabilities");
                require(caps.PixelShaderVersion>=D3DPS_VERSION(3,0)&&caps.VertexShaderVersion>=D3DVS_VERSION(3,0),"shader model3 required");
                check(api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_SURFACE,D3DFMT_A32B32G32R32F),"float target capability");
                D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
                check(api->CreateDevice(adapter,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,device.out()),"D3D9 HAL creation");selectedCaps=caps;found=true;break;
            }
            require(found,"requested Vulkan-matching D3D9 adapter unavailable");
            std::cout<<"D3D9 max_aniso="<<selectedCaps.MaxAnisotropy<<" min_aniso="<<bool(selectedCaps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC)<<" mag_aniso="<<bool(selectedCaps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFANISOTROPIC)<<"\n";
            // Observe the system device default; do not manufacture the reference
            // by first writing BORDERCOLOR. Every pixel sampler starts at zero.
            for(DWORD stage=0;stage<16;++stage){DWORD color=0xffffffffu;check(device->GetSamplerState(stage,D3DSAMP_BORDERCOLOR,&color),"default border query");require(color==0,"transparent black device default");}
            std::cout<<"d3d9_pixel_border_defaults_checked=16 value=0 no_border_color_writes=1\n";
            std::ofstream metadata(output/"device.txt");metadata<<"description="<<identity.Description<<"\nvendor="<<identity.VendorId<<"\ndevice="<<identity.DeviceId<<"\ndriver="<<identity.Driver<<"\ndriver_version="<<identity.DriverVersion.QuadPart<<"\nwindow_visible="<<IsWindowVisible(window)<<"\n";require(bool(metadata),"device metadata");
            metadata<<"pointer_bits="<<sizeof(void*)*8<<"\nmax_anisotropy="<<selectedCaps.MaxAnisotropy<<"\nfilter_caps="<<selectedCaps.TextureFilterCaps<<"\n";
            for(const auto*name:{L"d3d9.dll",L"d3dcompiler_47.dll"}){
                const auto module=GetModuleHandleW(name);require(module!=nullptr,"system sampling module loaded");
                std::array<wchar_t,32768>path{};const auto size=GetModuleFileNameW(module,path.data(),DWORD(path.size()));
                require(size&&size<path.size(),"system sampling module path");
                metadata<<std::filesystem::path(name).string()<<"="<<std::filesystem::path(path.data()).string()<<"\n";
                const auto*image=reinterpret_cast<const uint8_t*>(module);const auto*dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
                require(dos->e_magic==IMAGE_DOS_SIGNATURE&&dos->e_lfanew>0,"loaded system module DOS header");
                const auto*nt=image+dos->e_lfanew;require(*reinterpret_cast<const DWORD*>(nt)==IMAGE_NT_SIGNATURE,"loaded system module NT header");
                const auto*file=reinterpret_cast<const IMAGE_FILE_HEADER*>(nt+4);
                metadata<<std::filesystem::path(name).string()<<"_loaded_machine="<<file->Machine<<"\n";
                metadata<<std::filesystem::path(name).string()<<"_loaded_optional_magic="<<*reinterpret_cast<const WORD*>(nt+4+sizeof(IMAGE_FILE_HEADER))<<"\n";
            }
            require(bool(metadata),"extended device metadata");
            SamplingCom<ID3DBlob>vertexCode,pixelCode;
            auto compile=[&](const char*entry,const char*profile,SamplingCom<ID3DBlob>&code){SamplingCom<ID3DBlob>errors;auto hr=D3DCompile(shaderSource,sizeof(shaderSource)-1,"owned-texture-sampling",nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,0,code.out(),errors.out());if(errors.p){std::ofstream f(output/(std::string(entry)+"-compiler.log"),std::ios::binary);f.write(static_cast<const char*>(errors->GetBufferPointer()),std::streamsize(errors->GetBufferSize()));}check(hr,"owned shader compilation");std::ofstream f(output/(std::string(entry)+".bin"),std::ios::binary);f.write(static_cast<const char*>(code->GetBufferPointer()),std::streamsize(code->GetBufferSize()));require(bool(f),"owned shader output");};
            compile("vertexMain","vs_3_0",vertexCode);compile("pixelMain","ps_3_0",pixelCode);
            for(const auto&entry:std::array<std::pair<const char*,ID3DBlob*>,2>{{{"vertexMain",vertexCode.p},{"pixelMain",pixelCode.p}}}){
                SamplingCom<ID3DBlob>assembly;check(D3DDisassemble(entry.second->GetBufferPointer(),entry.second->GetBufferSize(),0,nullptr,assembly.out()),"owned shader disassembly");
                std::ofstream file(output/(std::string(entry.first)+".asm"),std::ios::binary);file.write(static_cast<const char*>(assembly->GetBufferPointer()),std::streamsize(assembly->GetBufferSize()));require(bool(file),"owned shader assembly output");
            }
            SamplingCom<IDirect3DVertexShader9>vertex;SamplingCom<IDirect3DPixelShader9>pixel;
            check(device->CreateVertexShader(static_cast<const DWORD*>(vertexCode->GetBufferPointer()),vertex.out()),"vertex creation");check(device->CreatePixelShader(static_cast<const DWORD*>(pixelCode->GetBufferPointer()),pixel.out()),"pixel creation");
            const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,16,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},{0,32,D3DDECLTYPE_FLOAT1,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,1},{0,36,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,2},D3DDECL_END()};
            SamplingCom<IDirect3DVertexDeclaration9>declaration;check(device->CreateVertexDeclaration(elements,declaration.out()),"declaration");
            check(device->SetVertexDeclaration(declaration.p),"declaration bind");check(device->SetVertexShader(vertex.p),"vertex bind");check(device->SetPixelShader(pixel.p),"pixel bind");
            SamplingCom<IDirect3DSurface9>target,readback;check(device->CreateRenderTarget(64,64,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,target.out(),nullptr),"float target");check(device->CreateOffscreenPlainSurface(64,64,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,readback.out(),nullptr),"float readback");check(device->SetRenderTarget(0,target.p),"target bind");check(device->SetDepthStencilSurface(nullptr),"no depth");D3DVIEWPORT9 viewport{0,0,64,64,0,1};check(device->SetViewport(&viewport),"viewport");
            const std::pair<D3DRENDERSTATETYPE,DWORD>states[]{{D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_FOGENABLE,FALSE},{D3DRS_LIGHTING,FALSE},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_DITHERENABLE,FALSE},{D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_COLORWRITEENABLE,15}};
            for(const auto&s:states)check(device->SetRenderState(s.first,s.second),"explicit render state");
            std::ofstream out(output/"samples.bin",std::ios::binary),samplers(output/"sampler-states.bin",std::ios::binary);uint32_t total=0;
            for(uint32_t serial=0;serial<count;++serial){
                std::array<uint32_t,10>q{};for(auto&n:q)n=word(in);const auto name=text(in),path=text(in);(void)name;const auto n=word(in);require(n&&n<=4096,"sample count");std::vector<Request>requests(n);require(bool(in.read(reinterpret_cast<char*>(requests.data()),std::streamsize(n*sizeof(Request)))),"sample requests");
                require(q[0]<=1&&q[1]<=1&&q[2]<=1&&q[3]<=1&&(q[4]==0||q[4]==2||q[4]==3)&&(q[5]==0||q[5]==2||q[5]==3)&&q[7]==0xffffffffu,"explicit sampler/no CPU conversion contract");
                std::ifstream source(path,std::ios::binary);require(bool(source),"authored VTF");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(source)),{});const auto layout=readVtfTextureLayoutCandidate(bytes);require(layout.frameCount==2&&layout.mips.size()==4&&(layout.format==13||layout.format==15||layout.format==0),"fixture format/extent");
                float factor;std::memcpy(&factor,&q[8],4);require(q[9]<4&&std::isfinite(factor)&&factor>=0&&factor<=float(selectedCaps.MaxAnisotropy)&&std::floor(factor)==factor,"aniso factor/base mip");if(factor)require((selectedCaps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC)!=0,"D3D9 min anisotropy capability");const auto format=layout.format==13?D3DFMT_DXT1:layout.format==15?D3DFMT_DXT5:D3DFMT_A8R8G8B8;
                check(api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,0,D3DRTYPE_TEXTURE,format),"texture format capability");
                if(q[0])check(api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_QUERY_SRGBREAD,D3DRTYPE_TEXTURE,format),"sRGB read capability");
                if(q[1]||q[2]||q[3])check(api->CheckDeviceFormat(adapter,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_QUERY_FILTER,D3DRTYPE_TEXTURE,format),"linear filtering capability");
                std::array<SamplingCom<IDirect3DTexture9>,2>textures;
                for(uint32_t frame=0;frame<2;++frame){
                    check(device->CreateTexture(layout.mips[0].width,layout.mips[0].height,4,0,format,D3DPOOL_MANAGED,textures[frame].out(),nullptr),"authored texture create");
                    for(uint32_t level=0;level<4;++level){const auto&m=layout.mips[level];D3DLOCKED_RECT lock{};check(textures[frame]->LockRect(level,&lock,nullptr,0),"texture lock");const auto*src=bytes.data()+m.offset+size_t(frame)*m.size;const uint32_t rows=layout.format==0?m.height:(m.height+3)/4;const uint32_t stride=layout.format==0?m.width*4:((m.width+3)/4)*(layout.format==13?8:16);require(lock.Pitch>=int(stride),"locked row pitch");
                        for(uint32_t y=0;y<rows;++y){auto*dst=static_cast<uint8_t*>(lock.pBits)+size_t(y)*size_t(lock.Pitch);std::memcpy(dst,src+size_t(y)*stride,stride);if(layout.format==0)for(uint32_t x=0;x<m.width;++x)std::swap(dst[4*x],dst[4*x+2]);}
                        check(textures[frame]->UnlockRect(level),"texture unlock");}
                    check(device->SetTexture(frame,textures[frame].p),"frame texture bind");
                    for(auto state:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW})check(device->SetSamplerState(frame,state,state==D3DSAMP_ADDRESSV?(q[5]==3?D3DTADDRESS_BORDER:q[5]==2?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP):(q[4]==3?D3DTADDRESS_BORDER:q[4]==2?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP)),"address mode");
                    check(device->SetSamplerState(frame,D3DSAMP_MINFILTER,factor?D3DTEXF_ANISOTROPIC:q[1]?D3DTEXF_LINEAR:D3DTEXF_POINT),"min filter");check(device->SetSamplerState(frame,D3DSAMP_MAGFILTER,factor&&(selectedCaps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFANISOTROPIC)?D3DTEXF_ANISOTROPIC:q[2]?D3DTEXF_LINEAR:D3DTEXF_POINT),"mag filter");check(device->SetSamplerState(frame,D3DSAMP_MIPFILTER,q[3]?D3DTEXF_LINEAR:D3DTEXF_POINT),"mip filter");check(device->SetSamplerState(frame,D3DSAMP_SRGBTEXTURE,q[0]),"sRGB input");check(device->SetSamplerState(frame,D3DSAMP_MIPMAPLODBIAS,0),"LOD bias zero");check(device->SetSamplerState(frame,D3DSAMP_MAXMIPLEVEL,q[9]),"largest mip zero");check(device->SetSamplerState(frame,D3DSAMP_MAXANISOTROPY,factor?DWORD(factor):1),"anisotropy one");DWORD border=0xffffffffu;check(device->GetSamplerState(frame,D3DSAMP_BORDERCOLOR,&border),"retained border query");require(border==0,"border default retained");
                }
                word(samplers,serial);
                for(DWORD stage=0;stage<2;++stage)for(DWORD state=1;state<=13;++state){DWORD value=0;check(device->GetSamplerState(stage,D3DSAMPLERSTATETYPE(state),&value),"actual sampler state capture");word(samplers,value);}
                require(bool(samplers),"sampler capture output");
                std::vector<Vertex>vertices;vertices.reserve(size_t(n)*6);
                const std::array<std::array<float,2>,6>corners{{{-.5f,-.5f},{.5f,-.5f},{-.5f,.5f},{-.5f,.5f},{.5f,-.5f},{.5f,.5f}}};
                for(uint32_t i=0;i<n;++i){const auto&r=requests[i];require(r.frame<2&&std::isfinite(r.u)&&std::isfinite(r.v)&&std::isfinite(r.lod),"request values");for(const auto&corner:corners){float x=float(i%64)+corner[0],y=float(i/64)+corner[1];vertices.push_back({2*x/64-1,1-2*y/64,.5f,1,r.u,r.v,0,r.lod,float(r.frame),r.dxu,r.dxv,r.dyu,r.dyv});}}
                check(device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"clear");check(device->BeginScene(),"begin");check(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,n*2,vertices.data(),sizeof(Vertex)),"batched owned texture draws");check(device->EndScene(),"end");check(device->GetRenderTargetData(target.p,readback.p),"float readback");D3DLOCKED_RECT lock{};check(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY),"readback lock");word(out,serial);word(out,n);
                for(uint32_t i=0;i<n;++i){std::array<float,4>value{};std::memcpy(value.data(),static_cast<const uint8_t*>(lock.pBits)+size_t(i/64)*size_t(lock.Pitch)+size_t(i%64)*16,16);for(auto v:value)require(std::isfinite(v),"nonfinite sampled component");out.write(reinterpret_cast<const char*>(value.data()),16);}
                check(readback->UnlockRect(),"readback unlock");for(uint32_t frame=0;frame<2;++frame)check(device->SetTexture(frame,nullptr),"texture unbind before owner release");total+=n;
            }
            require(in.peek()==EOF&&in.eof(),"complete query framing");out.flush();samplers.flush();require(bool(out)&&bool(samplers),"sample/state output");std::cout<<"PASS d3d9_sampling_cases="<<count<<" actual_shader_samples="<<total<<" pointer_bits="<<sizeof(void*)*8<<" original_runtime=ABSENT owned_shaders=1 hidden_window=1\n";
        }
        DestroyWindow(window);return 0;
    }catch(const std::exception&e){if(window)DestroyWindow(window);std::cerr<<e.what()<<'\n';return 1;}
}
