#include "material_texture_converted_image_candidate.h"
#define main precedingCpuImageMain
#include "probe_material_texture_cpu_image.cpp"
#undef main
using Converted=MaterialTextureConvertedImageCandidate;
static TextureFormatPolicyServicesCandidate services(uint32_t target){
    TextureFormatPolicyServicesCandidate s;s.enabled=[]()->std::optional<uint32_t>{return 1;};s.nearestFormat=[target](uint32_t,uint32_t)->std::optional<uint32_t>{return target;};s.hdrType=[]()->std::optional<uint32_t>{return 0;};return s;
}
static int convertedSafety(){
    safety();unsigned checks=0;const auto check=[&](bool b){need(b,"converted image safety");++checks;};
    auto bytes=fixture();const auto originalBytes=bytes;auto resource=MaterialTextureResourceCandidate::fromVtf("fixture/conversion",bytes);auto ref=Ref::ordinary(resource);
    std::shared_ptr<const Image>image;check(Image::prepare(ref,settings(),image));auto weakResource=std::weak_ptr<const MaterialTextureResourceCandidate>(resource);
    auto calls=services(3);std::shared_ptr<const Converted>output;check(Converted::prepare(image,0,calls,output));auto original=output;
    check(output->format()==3&&output->plan()[2]==3&&output->plan()[0]==0&&output->effectiveVtfFlags()==0&&output->ownsConvertedPixels());
    for(uint32_t mip=0;mip<3;++mip)for(uint32_t frame=0;frame<2;++frame){auto lease=output->lease(frame,mip);const auto n=uint8_t(mip*43+frame*71+3);check(lease&&lease->sourceLevel()==mip&&lease->frame()==frame&&lease->size()==size_t(4u>>mip)*(4u>>mip)*3&&lease->data()[0]==uint8_t(n+34)&&lease->data()[1]==uint8_t(n+17)&&lease->data()[2]==n);}
    check(!output->lease(2,0)&&!output->lease(0,3));
    for(unsigned mode=0;mode<13;++mode){auto broken=calls;auto source=image;size_t limit=size_t(1)<<29;
        if(mode==0)source.reset();if(mode==1)broken.enabled={};if(mode==2)broken.enabled=[]()->std::optional<uint32_t>{return {};};
        if(mode==3)broken.nearestFormat={};if(mode==4)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return {};};
        if(mode==5)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 13;};if(mode==6)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 24;};
        if(mode==7)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 64;};
        if(mode==8)broken.enabled=[]()->std::optional<uint32_t>{throw std::bad_alloc();};
        if(mode==9){broken=services(0);broken.hdrType={};}if(mode==10)limit=0;if(mode==11)limit=10;if(mode==12)limit=100;
        check(!Converted::prepare(source,0,broken,output,limit)&&output==original);
    }
    auto lease=output->lease(1,2);auto copy=lease;std::weak_ptr<const Converted>weakOutput=output;
    bytes.assign(bytes.size(),0xee);resource.reset();ref={};image.reset();original.reset();output.reset();
    check(weakOutput.expired()&&weakResource.expired()&&lease&&copy&&lease->data()[0]==uint8_t(2*43+71+3+34));
    auto moved=std::move(*copy);check(copy->data()==nullptr&&copy->size()==0&&moved.data()==lease->data());lease.reset();copy.reset();check(moved.size()==3&&moved.data()[2]==uint8_t(2*43+71+3));
    auto remaining=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/passthrough",originalBytes));check(Image::prepare(remaining,settings(),image));weakResource=remaining.resource();
    auto disabled=services(0);disabled.enabled=[]()->std::optional<uint32_t>{return 0x100;};disabled.nearestFormat={};disabled.hdrType={};
    check(Converted::prepare(image,0,disabled,output)&&!output->ownsConvertedPixels()&&!output->policy().changed&&output->effectiveVtfFlags()==0);
    auto untouched=output->lease(1,1);const auto*borrowed=untouched->data();remaining={};image.reset();output.reset();check(!weakResource.expired()&&untouched->data()==borrowed&&untouched->data()[0]==uint8_t(43+71+3));untouched.reset();check(weakResource.expired());
    auto finalRef=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/reentrant",originalBytes));check(Image::prepare(finalRef,settings(),image));auto reentrant=services(12);reentrant.enabled=[&]()->std::optional<uint32_t>{image.reset();finalRef={};reentrant.nearestFormat={};return 1;};
    check(Converted::prepare(image,0,reentrant,output)&&output->format()==12&&output->effectiveVtfFlags()==0x2000&&output->plan()[0]==0);
    check(output->lease(0,0)->data()[0]==37);
    std::cout<<"PASS owned_converted_image_safety_checks="<<checks<<" source_safety_checks=37 original_runtime=ABSENT gpu_calls=0\n";return 0;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned native x64 converted CPU image");if(argc==2&&std::string(argv[1])=="--test")return convertedSafety();
    need(argc==3&&!std::ifstream(argv[2]).good(),"queries fresh-output");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);need(bool(in)&&bool(out),"probe IO");const auto count=read(in);need(count&&count<=8192,"case extent");uint32_t available=0;
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,11>q{};for(auto&n:q)n=read(in);const auto logical=text(in),key=text(in),file=text(in);(void)logical;
        std::ifstream f(file,std::ios::binary);need(bool(f),"source snapshot");std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(f)),{});const auto before=bytes;
        Ref ref;try{ref=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf(key,bytes));}catch(const std::exception&){}
        MaterialTextureCpuImageSettingsCandidate config;config.requestedFlags=q[0];config.picmip=q[1];config.maxWidth=q[2];config.maxHeight=q[3];config.forceFullMipChain=q[4]!=0;config.dimensionClamp=q[5];config.hardwareKnown=true;config.overridesKnownAbsent=true;
        std::shared_ptr<const Image>image;bool ok=Image::prepare(ref,config,image);std::shared_ptr<const Converted>converted;std::string trace;
        TextureFormatPolicyServicesCandidate calls;calls.enabled=[&]()->std::optional<uint32_t>{trace+="M "+std::to_string(q[8])+"\n";return q[8];};
        calls.nearestFormat=[&](uint32_t request,uint32_t flag)->std::optional<uint32_t>{trace+="FORMAT "+std::to_string(request)+" "+std::to_string(flag)+" "+std::to_string(q[7])+"\n";return q[7];};
        calls.hdrType=[&]()->std::optional<uint32_t>{trace+="HDR "+std::to_string(q[9])+"\n";return q[9];};
        if(ok)ok=Converted::prepare(image,q[10],calls,converted);need(bytes==before,"input unchanged");write(out,serial);write(out,uint32_t(ok));
        if(ok){write(out,converted->rawFlags());write(out,converted->sourceVtfFlags());write(out,converted->requestedFlags());const auto plan=converted->plan();out.write(reinterpret_cast<const char*>(plan.data()),sizeof(plan));
            write(out,converted->format());write(out,converted->effectiveVtfFlags());write(out,uint32_t(converted->policy().changed));write(out,converted->policy().conversionCount);write(out,uint32_t(converted->ownsConvertedPixels()));
            std::vector<MaterialConvertedTextureMipLeaseCandidate>leases;for(uint32_t mip=0;mip<plan[9];++mip)for(uint32_t frame=0;frame<plan[10];++frame){auto lease=converted->lease(frame,mip);need(bool(lease),"converted lease");leases.push_back(*lease);}
            ref={};image.reset();converted.reset();std::fill(bytes.begin(),bytes.end(),uint8_t(0xee));std::string pixels;
            for(const auto&lease:leases){const auto n=uint32_t(lease.size());pixels.append(reinterpret_cast<const char*>(&n),4);pixels.append(reinterpret_cast<const char*>(lease.data()),lease.size());}
            text(out,trace);text(out,pixels);++available;
        }
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace write");std::cout<<"PASS owned_converted_image_cases="<<count<<" available="<<available<<" original_runtime=ABSENT gpu_calls=0 editor=NOT_CONNECTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
