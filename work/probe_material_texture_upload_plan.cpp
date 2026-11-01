#include "material_texture_upload_plan_candidate.h"
#include <fstream>
#include <iostream>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word");return n;}
static std::string text(std::istream&i){const auto n=read(i);need(n&&n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text");return s;}
static void write(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
static void append(std::string&s,uint32_t n){s.append(reinterpret_cast<const char*>(&n),4);}
static void text(std::ostream&o,const std::string&s){write(o,uint32_t(s.size()));o.write(s.data(),std::streamsize(s.size()));}
static std::vector<uint8_t>fixture(){
    std::vector<uint8_t>b(88,0);const auto word=[&](size_t at,uint32_t n){std::memcpy(b.data()+at,&n,4);};const auto half=[&](size_t at,uint16_t n){std::memcpy(b.data()+at,&n,2);};
    word(0,0x00465456);word(4,7);word(8,5);word(12,88);half(16,4);half(18,4);half(24,2);word(52,0);b[56]=3;word(57,0xffffffff);half(63,1);word(68,1);word(80,0x30);word(84,88);
    for(uint32_t mip=3;mip--;){for(uint32_t frame=0;frame<2;++frame)for(uint32_t p=0;p<(4u>>mip)*(4u>>mip);++p){b.push_back(uint8_t(17+mip+frame));b.push_back(37);b.push_back(97);b.push_back(255);}}
    return b;
}
static int safety(){
    unsigned checks=0;const auto check=[&](bool b){need(b,"upload plan safety");++checks;};
    auto bytes=fixture();auto resource=MaterialTextureResourceCandidate::fromVtf("fixture/upload",bytes);auto ref=MaterialTextureReferenceCandidate::ordinary(resource);auto weak=std::weak_ptr<const MaterialTextureResourceCandidate>(resource);
    MaterialTextureCpuImageSettingsCandidate settings;settings.maxWidth=settings.maxHeight=64;settings.forceFullMipChain=settings.hardwareKnown=settings.overridesKnownAbsent=true;
    std::shared_ptr<const MaterialTextureCpuImageCandidate>image;check(MaterialTextureCpuImageCandidate::prepare(ref,settings,image));TextureFormatPolicyServicesCandidate services;services.enabled=[]()->std::optional<uint32_t>{return 0;};
    std::shared_ptr<const MaterialTextureConvertedImageCandidate>converted;check(MaterialTextureConvertedImageCandidate::prepare(image,0,services,converted));
    using Plan=MaterialTextureUploadPlanCandidate;std::shared_ptr<const Plan>output;check(Plan::prepare(converted,1,3,12,output)&&output->commands().size()==8&&output->frameCount()==2&&output->sourceFormat()==0&&output->destinationFormat()==12);
    auto original=output;
    for(unsigned mode=0;mode<6;++mode){auto source=converted;auto faces=1u,mips=3u,target=12u;size_t cap=100;if(mode==0)source.reset();if(mode==1)faces=2;if(mode==2)mips=4;if(mode==3)target=64;if(mode==4)cap=0;if(mode==5)cap=7;check(!Plan::prepare(source,faces,mips,target,output,cap)&&output==original);}
    std::vector<MaterialConvertedTextureMipLeaseCandidate>leases;for(const auto&c:output->commands())if(c.pixels){check(c.kind==MaterialTextureUploadCommandKindCandidate::Upload2D&&c.arguments[0]<3&&c.arguments[1]==0&&c.arguments[2]==12&&c.arguments[3]==0&&c.arguments[6]==0&&c.arguments[7]==0);leases.push_back(*c.pixels);}
    check(Plan::prepare(converted,0,3,0,output)&&output->commands().size()==2);for(const auto&c:output->commands())check(c.kind==MaterialTextureUploadCommandKindCandidate::SelectFrame&&!c.pixels);
    check(Plan::prepare(converted,1,0,0,output)&&output->commands().size()==2);
    output.reset();original.reset();converted.reset();image.reset();resource.reset();ref={};bytes.clear();check(!weak.expired());for(const auto&lease:leases)check(lease.data()[0]==uint8_t(17+lease.sourceLevel()+lease.frame()));leases.clear();check(weak.expired());
    ref=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/converted",fixture()));weak=ref.resource();check(MaterialTextureCpuImageCandidate::prepare(ref,settings,image));services.enabled=[]()->std::optional<uint32_t>{return 1;};services.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 3;};check(MaterialTextureConvertedImageCandidate::prepare(image,0,services,converted));check(Plan::prepare(converted,1,2,3,output)&&output->commands().size()==6);
    auto lease=output->commands().back().pixels;check(lease&&lease->size()==12&&lease->data()[0]==97&&lease->data()[2]==19);converted.reset();image.reset();ref={};output.reset();check(weak.expired()&&lease->data()[0]==97);
    std::cout<<"PASS owned_upload_plan_safety_checks="<<checks<<" GPU_submissions=0\n";return 0;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64 upload plan");if(argc==2&&std::string(argv[1])=="--test")return safety();need(argc==3&&!std::ifstream(argv[2]).good(),"queries fresh-output");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);need(bool(in)&&bool(out),"IO");const auto count=read(in);need(count&&count<=8192,"cases");uint32_t available=0;
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,17>q{};for(auto&n:q)n=read(in);const auto logical=text(in),name=text(in),source=text(in);(void)logical;std::ifstream file(source,std::ios::binary);need(bool(file),"snapshot");std::vector<uint8_t>b((std::istreambuf_iterator<char>(file)),{});const auto before=b;MaterialTextureReferenceCandidate ref;try{ref=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf(name,b));}catch(const std::exception&){}
        MaterialTextureCpuImageSettingsCandidate settings;settings.requestedFlags=q[0];settings.picmip=q[1];settings.maxWidth=q[2];settings.maxHeight=q[3];settings.forceFullMipChain=q[4]!=0;settings.dimensionClamp=q[5];settings.hardwareKnown=settings.overridesKnownAbsent=true;
        TextureFormatPolicyServicesCandidate services;services.enabled=[&]()->std::optional<uint32_t>{return q[8];};services.nearestFormat=[&](uint32_t,uint32_t)->std::optional<uint32_t>{return q[7];};services.hdrType=[&]()->std::optional<uint32_t>{return q[9];};
        std::shared_ptr<const MaterialTextureCpuImageCandidate>image;std::shared_ptr<const MaterialTextureConvertedImageCandidate>converted;std::shared_ptr<const MaterialTextureUploadPlanCandidate>plan;bool ok=MaterialTextureCpuImageCandidate::prepare(ref,settings,image)&&MaterialTextureConvertedImageCandidate::prepare(image,q[10],services,converted);
        if(ok){const auto mips=q[13]==0xffffffffu?converted->plan()[9]:q[13],target=q[14]==0xffffffffu?converted->plan()[2]:q[14];ok=MaterialTextureUploadPlanCandidate::prepare(converted,q[12],mips,target,plan);}
        write(out,serial);write(out,uint32_t(ok));need(b==before,"input unchanged");
        if(plan){const auto frames=plan->frameCount(),mips=plan->mipCount(),format=plan->sourceFormat(),target=plan->destinationFormat();auto commands=plan->commands();plan.reset();converted.reset();image.reset();ref={};std::fill(b.begin(),b.end(),uint8_t(0xee));std::string events;uint32_t uploads=0;
            for(const auto&c:commands){if(c.kind==MaterialTextureUploadCommandKindCandidate::SelectFrame){append(events,1);append(events,c.frame);append(events,q[16]==0||(q[16]==2&&c.frame==0)?0x4000u+c.frame:0);}
                else{need(c.pixels&&c.pixels->data(),"retained upload lease");append(events,2);append(events,c.frame);for(auto n:c.arguments)append(events,n);append(events,uint32_t(c.pixels->size()));events.append(reinterpret_cast<const char*>(c.pixels->data()),c.pixels->size());++uploads;}}
            const std::array<uint32_t,7>record{serial,frames,mips,format,target,frames,uploads};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));text(out,events);++available;
        }
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace write");std::cout<<"PASS owned_upload_plan_cases="<<count<<" available="<<available<<" retained_pixel_leases=1 GPU_submissions=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
