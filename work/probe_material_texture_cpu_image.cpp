#include "material_texture_cpu_image_candidate.h"
#include "material_typed_dispatch_candidate.h"
#include "material_texture_file_resolver_candidate.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>
using Image=MaterialTextureCpuImageCandidate;
using Ref=MaterialTextureReferenceCandidate;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n=0;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"input word");return n;}
static std::string text(std::istream&i){auto n=read(i);need(n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text bytes");return s;}
static void write(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
static void text(std::ostream&o,const std::string&s){write(o,uint32_t(s.size()));o.write(s.data(),std::streamsize(s.size()));}
static std::vector<uint8_t>fixture(uint32_t minor=4,uint32_t mips=3,uint32_t depth=1,bool lod=false){
    const uint32_t header=minor<3?80u:lod?96u:88u;std::vector<uint8_t>b(header,0);
    const auto word=[&](size_t o,uint32_t n){std::memcpy(b.data()+o,&n,4);};
    const auto half=[&](size_t o,uint16_t n){std::memcpy(b.data()+o,&n,2);};
    word(0,0x00465456);word(4,7);word(8,minor);word(12,header);half(16,4);half(18,4);half(24,2);
    word(32,0x3e800000);word(36,0x3f000000);word(40,0x3f400000);word(52,0);word(57,0xffffffff);b[56]=uint8_t(mips);half(63,uint16_t(depth));
    if(minor>=3){word(68,lod?2u:1u);word(80,0x30);word(84,header);if(lod){word(88,0x02444f4c);word(92,0x0202);}}
    for(uint32_t mip=mips;mip>0;--mip)for(uint32_t frame=0;frame<2;++frame){const auto level=mip-1;const auto dim=std::max(1u,4u>>level);for(uint32_t i=0;i<dim*dim*4;++i)b.push_back(uint8_t(level*43+frame*71+i*17+3));}
    return b;
}
static MaterialTextureCpuImageSettingsCandidate settings(){MaterialTextureCpuImageSettingsCandidate s;s.maxWidth=64;s.maxHeight=64;s.forceFullMipChain=true;s.hardwareKnown=true;s.overridesKnownAbsent=true;return s;}
static int safety(){
    uint32_t checks=0;const auto check=[&](bool b){need(b,"CPU image safety");++checks;};
    auto bytes=fixture();auto resource=MaterialTextureResourceCandidate::fromVtf("fixture/animated",bytes);
    std::weak_ptr<const MaterialTextureResourceCandidate>weak=resource;auto ref=Ref::ordinary(resource);auto s=settings();
    std::shared_ptr<const Image>output;check(Image::prepare(ref,s,output));auto original=output;
    check(output->plan()[9]==3&&output->plan()[10]==2&&output->rawFlags()==0&&output->effectiveVtfFlags()==0);
    for(uint32_t mip=0;mip<3;++mip)for(uint32_t frame=0;frame<2;++frame){auto lease=output->lease(frame,mip);check(lease&&lease->sourceLevel()==mip&&lease->frame()==frame&&lease->width()==(4u>>mip)&&lease->size()==size_t(4u>>mip)*(4u>>mip)*4&&lease->data()[0]==uint8_t(mip*43+frame*71+3));}
    check(!output->lease(2,0)&&!output->lease(0,3));
    for(uint32_t mode=0;mode<10;++mode){auto bad=s;auto input=ref;
        if(mode==0)bad.hardwareKnown=false;if(mode==1)bad.overridesKnownAbsent=false;
        if(mode==2)bad.picmip=4;if(mode==3)bad.dimensionClamp=3;if(mode==4)bad.requestedFlags=0x4000;
        if(mode==5)input={};if(mode==6)input=Ref::environment();if(mode==7)input=Ref::ordinary(nullptr);
        if(mode==8){auto unsupported=fixture(4,3,1,true);unsupported[92]=31;input=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/lod",unsupported));}
        if(mode==9)input=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/depth",fixture(4,3,2)));
        check(!Image::prepare(input,bad,output)&&output==original);
    }
    auto partial=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/partial",fixture(4,1)));
    check(!Image::prepare(partial,s,output)&&output==original);
    auto single=s;single.forceFullMipChain=false;
    std::shared_ptr<const Image>one;check(Image::prepare(partial,single,one)&&one->plan()[9]==1);
    single.dimensionClamp=2;check(!Image::prepare(partial,single,output)&&output==original);
    auto lease=output->lease(1,2);auto leaseCopy=lease;
    MaterialTypedDispatchCandidate typed;typed.created=materialValueConstructCandidate(typed.value,MaterialValueConstructionKindCandidate::Undefined,{},0,"");
    check(materialTypedTextureSetCandidate(typed,ref));auto snapshot=typed;typed={};check(materialTypedTextureValidCandidate(snapshot));snapshot={};
    bytes.assign(bytes.size(),0xff);check(lease&&lease->data()[0]==uint8_t(2*43+71+3));
    resource.reset();ref={};original.reset();output.reset();check(!weak.expired()&&leaseCopy->data()[0]==uint8_t(2*43+71+3));
    lease.reset();check(!weak.expired());leaseCopy.reset();check(weak.expired());
    std::optional<MaterialTextureMipLeaseCandidate>resolverLease;
    std::weak_ptr<const MaterialTextureResourceCandidate>resolverWeak;
    {MaterialTextureFileResolverCandidate resolver([](const auto&)->std::optional<std::vector<uint8_t>>{return fixture();});
        auto resolved=resolver.resolve(std::string("fixture/resolved"));check(resolved.has_value());
        std::shared_ptr<const Image>resolvedImage;check(Image::prepare(*resolved,s,resolvedImage));
        resolverWeak=resolved->resource();resolverLease=resolvedImage->lease(1,1);}
    check(!resolverWeak.expired()&&resolverLease&&resolverLease->data()[0]==uint8_t(43+71+3));
    auto movedLease=std::move(*resolverLease);check(resolverLease->data()==nullptr&&resolverLease->size()==0&&movedLease.data()[0]==uint8_t(43+71+3));
    resolverLease.reset();
    // Retire the last lease in its own scope through an optional owner.
    std::optional<MaterialTextureMipLeaseCandidate>lastLease=std::move(movedLease);
    lastLease.reset();check(resolverWeak.expired());
    const auto refused=[&](std::vector<uint8_t>b){bool caught=false;try{(void)MaterialTextureResourceCandidate::fromVtf("fixture/bad",b);}catch(...){caught=true;}check(caught);};
    bytes=fixture();bytes.pop_back();refused(bytes);bytes=fixture();bytes.resize(88);refused(bytes);bytes=fixture();bytes.push_back(0);refused(bytes);bytes=fixture();bytes[0]=0;refused(bytes);
    std::cout<<"PASS owned_cpu_image_safety_checks="<<checks<<" retained_pixels=1 original_runtime=ABSENT gpu_calls=0\n";return 0;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned native x64 CPU image");
    if(argc==2&&std::string(argv[1])=="--test")return safety();
    need(argc==3&&!std::ifstream(argv[2]).good(),"queries fresh-output");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
    const auto count=read(in);need(count&&count<=4096,"case extent");uint32_t successes=0;
    for(uint32_t serial=0;serial<count;++serial){
        MaterialTextureCpuImageSettingsCandidate s;s.requestedFlags=read(in);s.picmip=read(in);s.maxWidth=read(in);s.maxHeight=read(in);s.forceFullMipChain=read(in)!=0;s.dimensionClamp=read(in);s.overridesKnownAbsent=true;s.hardwareKnown=true;
        const auto mode=read(in);const auto logical=text(in),key=text(in),source=text(in);s.graffitiPolicy=logical.compare(0,19,"materials/graffiti/")==0;
        std::ifstream f(source,std::ios::binary);need(bool(f),"source snapshot");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(f)),{});const auto before=bytes;
        Ref ref;try{if(mode)ref=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf(key,bytes));}catch(const std::exception&){}
        std::shared_ptr<const Image>image;const bool ok=Image::prepare(ref,s,image);need(bytes==before,"input unchanged");
        write(out,serial);write(out,uint32_t(ok));
        if(ok){write(out,image->rawFlags());write(out,image->effectiveVtfFlags());write(out,image->requestedFlags());const auto plan=image->plan();out.write(reinterpret_cast<const char*>(plan.data()),sizeof(plan));
            std::vector<MaterialTextureMipLeaseCandidate>leases;for(uint32_t mip=0;mip<plan[9];++mip)for(uint32_t frame=0;frame<plan[10];++frame){auto lease=image->lease(frame,mip);need(bool(lease),"valid selected lease");leases.push_back(*lease);}
            const auto retained=leases.front().data();ref={};image.reset();std::fill(bytes.begin(),bytes.end(),uint8_t(0xee));need(leases.front().data()==retained,"stable retained storage");
            std::string pixels;for(const auto&lease:leases){const auto n=uint32_t(lease.size());pixels.append(reinterpret_cast<const char*>(&n),4);pixels.append(reinterpret_cast<const char*>(lease.data()),lease.size());}text(out,pixels);++successes;
        }
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace write");std::cout<<"PASS owned_cpu_image_cases="<<count<<" available="<<successes<<" original_runtime=ABSENT gpu_calls=0 editor=NOT_CONNECTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
