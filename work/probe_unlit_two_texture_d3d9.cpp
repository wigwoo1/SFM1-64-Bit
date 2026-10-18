// Isolated native x64 GPU oracle. Uses extracted installed shader bytecode,
// synthetic textures and explicit states; never loads SFM or its shader DLLs.
#include <windows.h>
#include <d3d9.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

template<class T> struct Com {
    T* p=nullptr;
    ~Com(){if(p)p->Release();}
    T** out(){if(p)throw std::runtime_error("COM output already owned");return &p;}
    T* operator->()const{return p;}
};
void check(HRESULT hr,const char* operation){
    if(FAILED(hr))throw std::runtime_error(std::string(operation)+" HRESULT="+std::to_string(uint32_t(hr)));
}
std::vector<DWORD> program(const std::filesystem::path& path){
    std::ifstream file(path,std::ios::binary);
    if(!file)throw std::runtime_error("shader bytecode missing");
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});
    if(bytes.empty()||bytes.size()%4)throw std::runtime_error("unaligned shader bytecode");
    std::vector<DWORD> result(bytes.size()/4);
    std::memcpy(result.data(),bytes.data(),bytes.size());return result;
}
struct Vertex{float x,y,z,w,u,v,q,r;};
int main(int argc,char** argv){
    static_assert(sizeof(void*)==8,"x64 required");
    HWND window=nullptr;
    try{
        if(argc!=3)throw std::runtime_error("usage: probe shader-directory fresh-output-directory");
        const std::filesystem::path shaders=argv[1], output=argv[2];
        if(!std::filesystem::create_directory(output))throw std::runtime_error("fresh output directory required");
        auto vsBytes=program(shaders/"unlittwotexture_vs20-s0-d2.bin");
        auto psBytes=program(shaders/"unlittwotexture_ps20-s0-d0.bin");
        window=CreateWindowExW(0,L"STATIC",L"SFM64 isolated two-texture oracle",WS_POPUP,
                              0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        if(!window)throw std::runtime_error("hidden window creation failed");
        {
            Com<IDirect3D9> api;api.p=Direct3DCreate9(D3D_SDK_VERSION);
            if(!api.p)throw std::runtime_error("D3D9 unavailable");
            D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;
            pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;
            pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
            pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
            Com<IDirect3DDevice9> device;UINT adapter=0;HRESULT created=E_FAIL;
            for(;adapter<api->GetAdapterCount();++adapter){
                // CreateDevice treats presentation parameters as in/out. Do
                // not carry an unsuccessful adapter's mutations to the next.
                auto attemptParameters=pp;
                created=api->CreateDevice(adapter,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                          &attemptParameters,device.out());
                std::cout<<"device_attempt_adapter="<<adapter<<" result="<<uint32_t(created)<<'\n';
                if(SUCCEEDED(created))break;
            }
            check(created,"CreateDevice HAL");
            D3DADAPTER_IDENTIFIER9 identifier{};check(api->GetAdapterIdentifier(adapter,0,&identifier),"adapter identity");
            std::cout<<"adapter="<<identifier.Description<<" x64=1\n";
            Com<IDirect3DVertexShader9> vs;Com<IDirect3DPixelShader9> ps;
            check(device->CreateVertexShader(vsBytes.data(),vs.out()),"installed vertex shader creation");
            check(device->CreatePixelShader(psBytes.data(),ps.out()),"installed pixel shader creation");
            check(device->SetVertexShader(vs.p),"vertex shader bind");
            check(device->SetPixelShader(ps.p),"pixel shader bind");
            const D3DVERTEXELEMENT9 elements[]{
                {0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
                {0,16,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
            Com<IDirect3DVertexDeclaration9> decl;
            check(device->CreateVertexDeclaration(elements,decl.out()),"vertex declaration");
            check(device->SetVertexDeclaration(decl.p),"vertex declaration bind");
            const float identity[]{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
            const float c0[]{0,1,0,0},white[]{1,1,1,1},zrow[]{0,0,1,0};
            check(device->SetVertexShaderConstantF(0,c0,1),"c0");
            check(device->SetVertexShaderConstantF(8,identity,4),"view projection");
            check(device->SetVertexShaderConstantF(13,zrow,1),"view projection Z");
            check(device->SetVertexShaderConstantF(47,white,1),"vertex modulation");
            check(device->SetVertexShaderConstantF(58,identity,3),"model");
            check(device->SetPixelShaderConstantF(1,white,1),"pixel modulation");
            check(device->SetPixelShaderConstantF(30,white,1),"light scale");
            Com<IDirect3DSurface9> target,depth,readback;
            check(device->CreateRenderTarget(64,64,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,target.out(),nullptr),"RGBA target");
            check(device->CreateDepthStencilSurface(64,64,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,depth.out(),nullptr),"depth target");
            check(device->SetRenderTarget(0,target.p),"target bind");
            check(device->SetDepthStencilSurface(depth.p),"depth bind");
            check(device->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,readback.out(),nullptr),"readback surface");
            Com<IDirect3DTexture9> texture0,texture1;
            auto fill=[&](Com<IDirect3DTexture9>& texture,const std::array<DWORD,4>& colors){
                check(device->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,texture.out(),nullptr),"texture");
                D3DLOCKED_RECT lock{};check(texture->LockRect(0,&lock,nullptr,0),"texture lock");
                for(size_t row=0;row<2;++row)std::memcpy(static_cast<char*>(lock.pBits)+row*lock.Pitch,colors.data()+row*2,8);
                check(texture->UnlockRect(0),"texture unlock");
            };
            // Deliberately different RGB and alpha in each quadrant.
            fill(texture0,{0x00808040,0xff408080,0xff204060,0xff608020});
            fill(texture1,{0x00804040,0xff408080,0xff806020,0xff206080});
            check(device->SetTexture(0,texture0.p),"texture0 bind");
            check(device->SetTexture(1,texture1.p),"texture1 bind");
            for(DWORD sampler=0;sampler<2;++sampler){
                check(device->SetSamplerState(sampler,D3DSAMP_MINFILTER,D3DTEXF_POINT),"min filter");
                check(device->SetSamplerState(sampler,D3DSAMP_MAGFILTER,D3DTEXF_POINT),"mag filter");
                check(device->SetSamplerState(sampler,D3DSAMP_MIPFILTER,D3DTEXF_NONE),"mip filter");
                check(device->SetSamplerState(sampler,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP),"address U");
                check(device->SetSamplerState(sampler,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP),"address V");
            }
            check(device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE),"culling");
            check(device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE),"alpha test");
            check(device->SetRenderState(D3DRS_FOGENABLE,FALSE),"fog");
            check(device->SetRenderState(D3DRS_ZENABLE,TRUE),"depth test");
            check(device->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESS),"explicit diagnostic depth comparison");
            check(device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE),"separate alpha");
            check(device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD),"blend operation");
            auto draw=[&](float z){
                const Vertex triangle[]{{-1,-1,z,1,.25f,.25f,0,1},{-1,3,z,1,.25f,.25f,0,1},{3,-1,z,1,.25f,.25f,0,1}};
                check(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,triangle,sizeof(Vertex)),"draw installed shader");
            };
            struct Case{const char* name;bool srgb,additive,alphaWrite,depthWrite;float shift0,shift1,clearDepth;int second;std::array<int,4> expected;bool unormOutput=false;};
            const Case cases[]{
                {"linear-product",false,false,true,false,0,0,1,0,{64,32,16,255}},
                {"second-uv-only",false,false,true,false,0,.5f,1,0,{32,64,32,255}},
                {"first-uv-only",false,false,true,false,.5f,0,1,0,{32,32,32,255}},
                {"additive",false,true,true,false,0,0,1,0,{80,64,64,255}},
                {"alpha-write-off",false,true,false,false,0,0,1,0,{80,64,64,64}},
                {"occluded",false,true,true,false,0,0,.3f,0,{16,32,48,64}},
                {"depth-write-off",false,false,true,false,0,0,1,1,{32,64,32,255}},
                {"depth-write-on-control",false,false,true,true,0,0,1,1,{64,32,16,255}},
                {"srgb-product",true,false,true,false,0,0,1,0,{-1,-1,-1,255}},
                {"srgb-clear-occluded",true,false,true,false,0,0,.3f,0,{-1,-1,-1,64}},
                {"srgb-additive",true,true,true,false,0,0,1,0,{-1,-1,-1,255}},
                {"srgb-layered",true,true,true,false,0,0,1,1,{-1,-1,-1,255}},
                {"srgb-layered-unorm-control",true,true,true,false,0,0,1,1,{-1,-1,-1,255},true},
                {"drawn-occluder",false,false,true,true,0,0,1,2,{64,32,16,255}},
            };
            std::ofstream receipt(output/"pixels.csv");receipt<<"case,r,g,b,a\n";
            for(const auto& test:cases){
                float uv0[]{1,0,0,test.shift0,0,1,0,0},uv1[]{1,0,0,test.shift1,0,1,0,0};
                check(device->SetVertexShaderConstantF(48,uv0,2),"first UV transform");
                check(device->SetVertexShaderConstantF(50,uv1,2),"second UV transform");
                for(DWORD sampler=0;sampler<2;++sampler)check(device->SetSamplerState(sampler,D3DSAMP_SRGBTEXTURE,test.srgb),"sRGB read");
                check(device->SetRenderState(D3DRS_SRGBWRITEENABLE,test.srgb&&!test.unormOutput),"sRGB write");
                check(device->SetRenderState(D3DRS_ALPHABLENDENABLE,test.additive),"blend enable");
                check(device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE),"source factor");
                check(device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE),"destination factor");
                check(device->SetRenderState(D3DRS_ZWRITEENABLE,test.depthWrite),"depth write");
                check(device->SetRenderState(D3DRS_COLORWRITEENABLE,test.alphaWrite?15:7),"color mask");
                check(device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0x40102030,test.clearDepth,0),"clear");
                check(device->BeginScene(),"begin scene");draw(test.second==2?.3f:.5f);
                if(test.second){uv1[3]=.5f;check(device->SetVertexShaderConstantF(50,uv1,2),"second draw UV");draw(test.second==2?.5f:.75f);}
                check(device->EndScene(),"end scene");
                check(device->GetRenderTargetData(target.p,readback.p),"GPU readback");
                D3DLOCKED_RECT lock{};check(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY),"readback lock");
                std::vector<uint8_t> rgba(64*64*4);
                for(size_t y=0;y<64;++y)for(size_t x=0;x<64;++x){
                    auto src=static_cast<const uint8_t*>(lock.pBits)+y*lock.Pitch+x*4;
                    auto dst=rgba.data()+(y*64+x)*4;dst[0]=src[2];dst[1]=src[1];dst[2]=src[0];dst[3]=src[3];
                }
                check(readback->UnlockRect(),"readback unlock");
                const auto* pixel=rgba.data()+(32*64+32)*4;
                receipt<<test.name;
                for(size_t component=0;component<4;++component){
                    receipt<<','<<unsigned(pixel[component]);
                    if(test.expected[component]>=0&&std::abs(int(pixel[component])-test.expected[component])>1)
                        throw std::runtime_error(std::string(test.name)+" pixel mismatch component "+std::to_string(component)+" actual="+std::to_string(pixel[component]));
                }
                receipt<<'\n';
                std::ofstream capture(output/(std::string(test.name)+".rgba"),std::ios::binary);
                capture.write(reinterpret_cast<const char*>(rgba.data()),std::streamsize(rgba.size()));
                if(!capture)throw std::runtime_error("capture write failed");
                std::cout<<test.name<<" RGBA="<<unsigned(pixel[0])<<','<<unsigned(pixel[1])<<','<<unsigned(pixel[2])<<','<<unsigned(pixel[3])<<" passed\n";
            }
            receipt.flush();if(!receipt)throw std::runtime_error("receipt write failed");
        }
        DestroyWindow(window);window=nullptr;
        std::cout<<"PASS: 14 installed-shader GPU cases; sRGB RGB captured for cross-backend comparison\n";return 0;
    }catch(const std::exception& error){if(window)DestroyWindow(window);std::cerr<<error.what()<<'\n';return 1;}
}
