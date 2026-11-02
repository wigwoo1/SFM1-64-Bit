// Owned offline raster discriminator. Never loads the original SFM runtime.
#define SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "probe_d3d9_prepared_scene_x64.cpp"
#undef SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY

struct GridVertex {
    std::array<float,4> clip, attribute;
};
static_assert(sizeof(GridVertex)==32,"owned reference layout");

class InterpolationGrid {
    IDirect3DDevice9* device_;
    D3D9SceneComCandidate<IDirect3DVertexShader9> vertex_;
    D3D9SceneComCandidate<IDirect3DPixelShader9> pixel_;
    D3D9SceneComCandidate<IDirect3DVertexDeclaration9> declaration_;
    D3D9SceneComCandidate<IDirect3DSurface9> target_, readback_;
public:
    explicit InterpolationGrid(IDirect3DDevice9* d):device_(d){
        // No matrix, half-pixel correction, sampler, color conversion, or
        // production shading stream participates in this independent path.
        const char* source=R"(
struct V { float4 position:POSITION; float4 value:TEXCOORD0; };
V vertexMain(V v) { return v; }
float4 pixelMain(V v):COLOR0 { return v.value; }
)";
        D3D9SceneComCandidate<ID3DBlob> vs,ps;
        auto compile=[&](const char* entry,const char* profile,auto& blob){
            blob.create([&](auto** p){return D3DCompile(source,std::strlen(source),
                "owned-interpolation-grid",nullptr,nullptr,entry,profile,
                D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,0,p,nullptr);});
        };
        compile("vertexMain","vs_3_0",vs);compile("pixelMain","ps_3_0",ps);
        vertex_.create([&](auto** p){return d->CreateVertexShader(static_cast<const DWORD*>(vs->GetBufferPointer()),p);});
        pixel_.create([&](auto** p){return d->CreatePixelShader(static_cast<const DWORD*>(ps->GetBufferPointer()),p);});
        const D3DVERTEXELEMENT9 elements[]{
            {0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,16,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        declaration_.create([&](auto** p){return d->CreateVertexDeclaration(elements,p);});
        target_.create([&](auto** p){return d->CreateRenderTarget(32,32,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,p,nullptr);});
        readback_.create([&](auto** p){return d->CreateOffscreenPlainSurface(32,32,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,p,nullptr);});
    }
    std::vector<float> render(const std::array<GridVertex,3>& vertices){
        auto*d=device_;
        D3D9SceneComCandidate<IDirect3DSurface9> oldTarget,oldDepth;
        oldTarget.create([&](auto** p){return d->GetRenderTarget(0,p);});
        IDirect3DSurface9* depth=nullptr;auto hr=d->GetDepthStencilSurface(&depth);
        if(hr!=D3DERR_NOTFOUND)d3d9SceneCheckCandidate(hr);
        oldDepth.reset(depth);bool begun=false,locked=false;
        auto cleanup=[&]{
            if(locked)readback_->UnlockRect();
            if(begun)d->EndScene();
            d->SetStreamSource(0,nullptr,0,0);d->SetStreamSource(1,nullptr,0,0);
            d->SetVertexDeclaration(nullptr);d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);
            d->SetRenderTarget(0,oldTarget.get());d->SetDepthStencilSurface(oldDepth.get());
        };
        try{
            d3d9SceneCheckCandidate(d->SetDepthStencilSurface(nullptr));
            d3d9SceneCheckCandidate(d->SetRenderTarget(0,target_.get()));
            D3DVIEWPORT9 viewport{0,0,32,32,0,1};d3d9SceneCheckCandidate(d->SetViewport(&viewport));
            const std::pair<D3DRENDERSTATETYPE,DWORD> states[]{
                {D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},
                {D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_ALPHATESTENABLE,FALSE},
                {D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},
                {D3DRS_COLORWRITEENABLE,15},{D3DRS_SCISSORTESTENABLE,FALSE},
                {D3DRS_MULTISAMPLEANTIALIAS,FALSE}};
            for(const auto& s:states)d3d9SceneCheckCandidate(d->SetRenderState(s.first,s.second));
            d3d9SceneCheckCandidate(d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0));
            d3d9SceneCheckCandidate(d->SetVertexDeclaration(declaration_.get()));
            d3d9SceneCheckCandidate(d->SetVertexShader(vertex_.get()));
            d3d9SceneCheckCandidate(d->SetPixelShader(pixel_.get()));
            d3d9SceneCheckCandidate(d->BeginScene());begun=true;
            d3d9SceneCheckCandidate(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,vertices.data(),sizeof(GridVertex)));
            d3d9SceneCheckCandidate(d->EndScene());begun=false;
            d3d9SceneCheckCandidate(d->GetRenderTargetData(target_.get(),readback_.get()));
            D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(readback_->LockRect(&lock,nullptr,D3DLOCK_READONLY));locked=true;
            std::vector<float> result(4096);
            for(size_t y=0;y<32;++y)std::memcpy(result.data()+y*128,static_cast<const uint8_t*>(lock.pBits)+y*size_t(lock.Pitch),512);
            d3d9SceneCheckCandidate(readback_->UnlockRect());locked=false;
            cleanup();return result;
        }catch(...){cleanup();throw;}
    }
};

int main(int argc,char** argv){try{
    need(argc==3,"owned-query fresh-output-directory");
    const auto file=bytes(argv[1]);need(file.size()>4,"query framing");
    std::ifstream input(argv[1],std::ios::binary);need(bool(input),"query open");
    const auto count=word(input);need(count&&count<=8192,"bounded grid cases");
    const std::filesystem::path output=argv[2];need(std::filesystem::create_directory(output),"fresh output only");
    std::ofstream capture(output/"pixels.bin",std::ios::binary);need(bool(capture),"capture open");
    size_t sampleCount=0;
    {SceneDevice context(output);const auto contextBaseline=d3d9SceneLiveResourcesCandidate.load();
        {InterpolationGrid grid(context.device.get());const auto baseline=d3d9SceneLiveResourcesCandidate.load();
            for(uint32_t serial=0;serial<count;++serial){
                std::array<GridVertex,3> vertices{};
                for(auto& v:vertices){
                    for(auto& x:v.clip)x=number(word(input));
                    for(auto& x:v.attribute)x=number(word(input));
                    need(v.clip[3]>0&&std::abs(v.clip[0])<v.clip[3]&&std::abs(v.clip[1])<v.clip[3]&&v.clip[2]>0&&v.clip[2]<v.clip[3],"unclipped discriminator only");
                }
                const auto n=word(input);need(n==3||n==1024,"bounded samples");
                std::vector<std::pair<uint32_t,uint32_t>> samples;
                for(uint32_t j=0;j<n;++j){const auto x=word(input),y=word(input);need(x<32&&y<32,"sample in target");samples.emplace_back(x,y);}
                const auto pixels=grid.render(vertices);word(capture,serial);word(capture,n);
                for(const auto& s:samples){const auto offset=4*(size_t(s.second)*32+s.first);for(size_t k=0;k<4;++k)need(std::isfinite(pixels[offset+k]),"finite grid value");capture.write(reinterpret_cast<const char*>(pixels.data()+offset),16);}
                sampleCount+=n;need(d3d9SceneLiveResourcesCandidate==baseline,"grid per-draw resources retired");
            }
        }
        need(d3d9SceneLiveResourcesCandidate==contextBaseline,"grid owner resources retired");
    }
    need(input.peek()==EOF&&input.eof()&&d3d9SceneLiveResourcesCandidate==0,"complete query/resources retired");
    capture.flush();need(bool(capture),"complete capture");
    std::cout<<"PASS interpolation_grid_cases="<<count<<" samples="<<sampleCount<<" native_x64=1 original_runtime=ABSENT live_resources=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
