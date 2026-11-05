// Owned device/resources only; no SFM runtime or asset dependency.
#define SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "probe_d3d9_prepared_scene_x64.cpp"
#undef SFM64_PREPARED_SCENE_PROBE_CONTEXT_ONLY
#include "d3d9_packed_lightmap_texture_candidate.h"

using Owner=D3D9PackedLightmapTextureCandidate;
using Pixel=NativeLightmapHdr2BitsCandidate;
using Sources=std::array<std::vector<Pixel>,4>;
using Views=std::array<const std::vector<Pixel>*,4>;
static constexpr uint32_t pattern[]{0,0x80000000,0x33800000,0xb3800000,0x337fffff,0x3f801fff,
    0x3f802000,0x477fe000,0x7f800000,0xff800000,0x7fc12345,0xff800001,0x12345678,0x89abcdef,0x3f000000,0x40000000};
static Sources sources(uint32_t width,uint32_t height){Sources s;for(size_t k=0;k<4;++k){s[k].resize(size_t(width)*height);
    for(size_t i=0;i<s[k].size();++i)for(size_t c=0;c<4;++c)s[k][i][c]=pattern[(i*3+k*5+c*7)%16];}return s;}
static Views views(const Sources& s,uint32_t mask){Views v{};for(size_t k=0;k<4;++k)if(mask&(1u<<k))v[k]=&s[k];return v;}
static std::vector<uint8_t> readback(const Owner& owner){
    D3DSURFACE_DESC desc{};d3d9SceneCheckCandidate(owner.texture()->GetLevelDesc(0,&desc));
    need(desc.Format==D3DFORMAT(owner.actualD3DFormat())&&desc.Width==owner.width()&&desc.Height==owner.height(),"actual readback descriptor");
    D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(owner.texture()->LockRect(0,&lock,nullptr,D3DLOCK_READONLY));
    bool locked=true;try{const size_t row=size_t(owner.width())*8;need(lock.pBits&&lock.Pitch>0&&size_t(lock.Pitch)>=row,"actual readback pitch");
        std::vector<uint8_t> data(row*owner.height());for(uint32_t y=0;y<owner.height();++y)std::memcpy(data.data()+size_t(y)*row,static_cast<const uint8_t*>(lock.pBits)+size_t(y)*size_t(lock.Pitch),row);
        locked=false;d3d9SceneCheckCandidate(owner.texture()->UnlockRect(0));return data;
    }catch(...){if(locked)owner.texture()->UnlockRect(0);throw;}
}
static void emit(std::ostream& out,uint32_t serial,uint32_t format,uint32_t w,uint32_t h,uint32_t mask,const Owner& owner){
    auto data=readback(owner);need(data==readback(owner),"repeat real texture readback");
    for(auto v:{serial,format,w,h,mask,owner.width(),owner.height(),uint32_t(data.size())})word(out,v);
    out.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));need(bool(out),"capture write");
}
int main(int argc,char**argv){try{
    need(argc==2,"fresh output directory required");const std::filesystem::path out=argv[1];need(std::filesystem::create_directory(out),"fresh probe directory");
    // The shared SceneDevice fixture authenticates both system runtime modules.
    // This byte-only probe has no shader call that would otherwise import DXC.
    struct CompilerModule {HMODULE value=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        ~CompilerModule(){if(value)FreeLibrary(value);}} compiler;
    need(compiler.value!=nullptr,"system shader compiler fixture module");
    SceneDevice context(out);const auto baseline=d3d9SceneLiveResourcesCandidate.load();D3DCAPS9 caps{};d3d9SceneCheckCandidate(context.device->GetDeviceCaps(&caps));
    D3DDEVICE_CREATION_PARAMETERS creation{};d3d9SceneCheckCandidate(context.device->GetCreationParameters(&creation));D3DDISPLAYMODE display{};d3d9SceneCheckCandidate(context.api->GetAdapterDisplayMode(creation.AdapterOrdinal,&display));
    std::ofstream captures(out/"textures.bin",std::ios::binary),old(out/"old-prepare.bin",std::ios::binary),guards(out/"guards.bin",std::ios::binary),metrics(out/"metrics.txt");
    metrics<<"max_width="<<caps.MaxTextureWidth<<"\nmax_height="<<caps.MaxTextureHeight<<'\n';
    size_t records=0,exceptions=0,hresultCases=0,guardCases=0,invalidCases=0;uint32_t serial=0;
    for(uint32_t format:{36u,113u}){
        const HRESULT support=context.api->CheckDeviceFormat(creation.AdapterOrdinal,D3DDEVTYPE_HAL,display.Format,0,D3DRTYPE_TEXTURE,D3DFORMAT(format));
        metrics<<"format_"<<format<<"_check="<<uint32_t(support)<<'\n';if(FAILED(support)){metrics<<"format_"<<format<<"_skipped=1\n";continue;}
        for(uint32_t mask=1;mask<16;mask+=2)for(auto size:std::array<std::array<uint32_t,2>,4>{{{1,1},{3,2},{129,129},{2049,2}}}){
            const auto w=size[0],h=size[1];const uint32_t n=mask==15?4u:1u;if(uint64_t(w)*n>caps.MaxTextureWidth||h>caps.MaxTextureHeight){metrics<<"layout_skip="<<format<<','<<w<<','<<h<<','<<mask<<'\n';continue;}
            std::unique_ptr<Owner> owner;{auto s=sources(w,h);owner=Owner::prepareHdr2(context.device.get(),w,h,views(s,mask),format);}
            need(owner->actualD3DFormat()==format&&owner->width()==n*w,"HDR2 owner metadata");emit(captures,serial++,format,w,h,mask,*owner);++records;
        }
        need(d3d9SceneLiveResourcesCandidate==baseline,"completed real cases release resources");
        auto s=sources(3,2);const auto v=views(s,15);std::vector<std::string> stages;
        auto committed=Owner::prepareHdr2(context.device.get(),3,2,v,format,[&](const char* name){stages.emplace_back(name);});
        const auto identity=committed.get();const auto texture=committed->texture();const auto bytesBefore=readback(*committed);const auto live=d3d9SceneLiveResourcesCandidate.load();
        auto preserved=[&]{need(committed.get()==identity&&committed->texture()==texture&&d3d9SceneLiveResourcesCandidate==live,"failed replacement owner/resources preserved");need(readback(*committed)==bytesBefore,"failed replacement previous bytes preserved");};
        for(bool allocation:{false,true})for(size_t fail=0;fail<stages.size();++fail){size_t index=0;bool refused=false;try{
            auto next=Owner::prepareHdr2(context.device.get(),3,2,v,format,[&](const char*){if(index++==fail){if(allocation)throw std::bad_alloc();throw std::runtime_error("injected stage");}});committed=std::move(next);
        }catch(const std::exception&){refused=true;}need(refused,"stage exception must refuse publication");preserved();++exceptions;}
        // Fault seam delegates to real COM resources except the deliberately
        // altered return/descriptor/lock data. Counts distinguish HRESULTs.
        for(int fault=0;fault<14;++fault){Owner::Hdr2Calls calls;size_t unlocks=0;bool realLock=false;bool returnedFailure=false;
            calls.unlock=[&](IDirect3DTexture9*t){++unlocks;const auto hr=t->UnlockRect(0);if(fault==12||fault==13){returnedFailure=true;return E_FAIL;}return hr;};
            if(fault<=2)calls.create=[&](IDirect3DDevice9*d,UINT w,UINT h,D3DFORMAT f,IDirect3DTexture9**t)->HRESULT{
                if(fault==0){returnedFailure=true;return E_FAIL;}if(fault==2)return S_OK;
                const auto hr=d->CreateTexture(w,h,1,0,f,D3DPOOL_MANAGED,t,nullptr);need(SUCCEEDED(hr),"fault setup real create");returnedFailure=true;return E_FAIL;};
            if(fault>=3&&fault<=7)calls.describe=[&](IDirect3DTexture9*t,D3DSURFACE_DESC*d)->HRESULT{
                if(fault==3){returnedFailure=true;return E_FAIL;}const auto hr=t->GetLevelDesc(0,d);if(FAILED(hr))return hr;
                if(fault==4)d->Format=format==36?D3DFMT_A16B16G16R16F:D3DFMT_A16B16G16R16;
                if(fault==5)++d->Width;if(fault==6)d->Pool=D3DPOOL_SYSTEMMEM;if(fault==7)d->Usage=D3DUSAGE_RENDERTARGET;return hr;};
            if(fault>=8&&fault<=11)calls.lock=[&](IDirect3DTexture9*t,D3DLOCKED_RECT*l)->HRESULT{
                if(fault==8){returnedFailure=true;return E_FAIL;}const auto hr=t->LockRect(0,l,nullptr,0);realLock=SUCCEEDED(hr);
                if(realLock){if(fault==9)l->pBits=nullptr;if(fault==10)l->Pitch=-1;if(fault==11)l->Pitch=1;}return hr;};
            bool refused=false;std::string error;try{auto next=Owner::prepareHdr2(context.device.get(),3,2,v,format,[&](const char* stage){if(fault==13&&std::string(stage)=="hdr2-locked")throw std::runtime_error("primary-cleanup-proof");},calls);committed=std::move(next);
            }catch(const std::exception&e){refused=true;error=e.what();}need(refused,"fault seam refused");
            if(fault<=8)need(unlocks==0,"pre-lock failure must not unlock");else need(unlocks==1,"successful lock must unlock exactly once");
            if(fault==13)need(error=="primary-cleanup-proof","cleanup error must preserve primary exception");
            if(fault==0||fault==1||fault==3||fault==8||fault==12||fault==13)need(returnedFailure,"HRESULT failure actually returned through seam");
            if(fault>=9&&fault<=11)need(realLock,"invalid layout after real acquisition");preserved();++hresultCases;
        }
        for(int bad=0;bad<7;++bad){auto altered=s;auto pointers=views(altered,15);uint32_t request=format,w=3,h=2;IDirect3DDevice9*device=context.device.get();
            if(bad==0)request=21;if(bad==1)w=0;if(bad==2)h=0;if(bad==3)pointers[0]=nullptr;if(bad==4)altered[2].pop_back();if(bad==5)w=UINT_MAX;if(bad==6)device=nullptr;
            bool refused=false;try{auto next=Owner::prepareHdr2(device,w,h,pointers,request);committed=std::move(next);}catch(const std::exception&){refused=true;}
            need(refused,"invalid input refused");preserved();++invalidCases;
        }
        // Guarded lock rows exercise the production copy branch with padding
        // independent of the actual HAL row alignment. Not real texture data.
        for(size_t pitch:{size_t(96+19),size_t(70001)}){
            std::vector<uint8_t> memory(16+pitch+96+16,0xa5);Owner::Hdr2Calls calls;size_t unlocks=0;
            calls.lock=[&](IDirect3DTexture9*t,D3DLOCKED_RECT*l){const auto hr=t->LockRect(0,l,nullptr,0);if(SUCCEEDED(hr)){l->pBits=memory.data()+16;l->Pitch=static_cast<INT>(pitch);}return hr;};
            calls.unlock=[&](IDirect3DTexture9*t){++unlocks;return t->UnlockRect(0);};
            {auto next=Owner::prepareHdr2(context.device.get(),3,2,v,format,{},calls);need(unlocks==1,"guarded lock unlock once");}
            for(auto n:{format,3u,2u,15u,uint32_t(pitch),uint32_t(memory.size())})word(guards,n);
            guards.write(reinterpret_cast<const char*>(memory.data()),std::streamsize(memory.size()));preserved();++guardCases;
        }
        // Fresh regression of the unchanged old numerical operation.
        std::vector<std::array<float,4>> oldPixels(6,{.25f,.5f,1.f,1.f});
        {auto previous=Owner::prepare(context.device.get(),3,2,oldPixels,format==113?D3D9PackedLightmapFormatCandidate::Rgba16Float:D3D9PackedLightmapFormatCandidate::Rgba16Unorm);emit(old,0,format,3,2,1,*previous);}
        preserved();committed.reset();need(d3d9SceneLiveResourcesCandidate==baseline,"all format resources released");
    }
    metrics<<"real_records="<<records<<"\nstage_exception_cases="<<exceptions<<"\nfault_seam_cases="<<hresultCases<<"\nguarded_lock_cases="<<guardCases<<"\ninvalid_cases="<<invalidCases<<"\nresources_restored=1\n";
    need(records>0&&bool(metrics)&&bool(captures)&&bool(old)&&bool(guards),"probe result output");std::cout<<"HDR2 texture records="<<records<<" faults="<<hresultCases<<" exceptions="<<exceptions<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
