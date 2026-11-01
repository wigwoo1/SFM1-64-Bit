#include "shader_texture_cpu_preparation_candidate.h"
#include <fstream>
#include <iostream>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word");return n;}
static std::string text(std::istream&i){const auto n=read(i);need(n<=4095,"text size");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text");return s;}
static void write(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
static void text(std::ostream&o,const std::string&s){write(o,uint32_t(s.size()));o.write(s.data(),std::streamsize(s.size()));}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64 pending CPU lookup");need(argc==3&&!std::ifstream(argv[2]).good(),"queries fresh-output");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);need(bool(in)&&bool(out),"IO");const auto count=read(in);need(count&&count<=8192,"cases");uint32_t available=0,checks=0;
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,12>q{};for(auto&n:q)n=read(in);const auto logical=text(in),name=text(in),source=text(in);(void)logical;
        std::ifstream f(source,std::ios::binary);need(bool(f),"snapshot");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});const auto before=b;
        MaterialTextureReferenceCandidate ref;try{ref=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf(name,b));}catch(const std::exception&){}
        MaterialTextureCpuImageSettingsCandidate settings;settings.picmip=q[1];settings.maxWidth=q[2];settings.maxHeight=q[3];settings.forceFullMipChain=q[4]!=0;settings.dimensionClamp=q[5];settings.hardwareKnown=settings.overridesKnownAbsent=true;
        TextureFormatPolicyServicesCandidate conversion;conversion.enabled=[&]()->std::optional<uint32_t>{return q[8];};conversion.nearestFormat=[&](uint32_t,uint32_t)->std::optional<uint32_t>{return q[7];};conversion.hdrType=[&]()->std::optional<uint32_t>{return q[9];};
        MaterialShaderTextureStateCandidate input;input.value.created=materialValueConstructCandidate(input.value.value,MaterialValueConstructionKindCandidate::String,{},0,name);need(input.value.created,"string value");const auto inputBefore=input;auto output=input;
        std::shared_ptr<const ShaderTextureCpuPreparationCandidate>pending;MaterialShaderTextureServicesCandidate calls;calls.configMask=[]()->std::optional<uint32_t>{return 0;};
        calls.find=[&](const MaterialShaderTextureRequestCandidate&r)->std::optional<MaterialShaderTextureLookupCandidate>{if((q[11]&255u)&&(q[8]&255u))ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(r,ref,settings,q[10],conversion,pending);return {};};
        // A pending CPU stage deliberately cannot complete the typed lookup.
        need(!materialShaderTextureInitCandidate(input,output,MaterialShaderTextureKindCandidate::Normal,q[0],std::string("Prefix group"),calls),"no premature typed publication");
        need(output.value.value.text==inputBefore.value.value.text&&output.value.value.numeric.typeAndMetadata==inputBefore.value.value.numeric.typeAndMetadata&&output.flags2==inputBefore.flags2&&input.value.value.text==name&&b==before,"preserve original typed state/input");++checks;
        write(out,serial);write(out,uint32_t(bool(pending)));
        if(pending){auto image=pending->image();text(out,pending->name());text(out,*pending->textureGroup());write(out,pending->requestedFlags());write(out,uint32_t(pending->hasAlpha()));const auto plan=image->plan();out.write(reinterpret_cast<const char*>(plan.data()),sizeof(plan));write(out,image->format());write(out,image->effectiveVtfFlags());
            auto stable=pending;const auto check=[&](bool v){need(v,"pending lookup safety");++checks;};MaterialShaderTextureRequestCandidate request;request.name=name;request.group="Prefix group";request.flags=q[0];
            auto bad=request;bad.kind=MaterialShaderTextureKindCandidate::Bump;check(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(bad,ref,settings,q[10],conversion,pending)&&pending==stable);
            bad=request;bad.group.reset();check(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(bad,ref,settings,q[10],conversion,pending)&&pending==stable);
            bad=request;bad.name="../bad";check(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(bad,ref,settings,q[10],conversion,pending)&&pending==stable);
            auto unavailable=conversion;unavailable.enabled={};check(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(request,ref,settings,q[10],unavailable,pending)&&pending==stable);
            check(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(request,MaterialTextureReferenceCandidate::environment(),settings,q[10],conversion,pending)&&pending==stable);
            auto wrong=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/wrong",before));check(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(request,wrong,settings,q[10],conversion,pending)&&pending==stable);
            std::vector<MaterialConvertedTextureMipLeaseCandidate>leases;for(uint32_t mip=0;mip<plan[9];++mip)for(uint32_t frame=0;frame<plan[10];++frame){auto lease=image->lease(frame,mip);check(bool(lease));leases.push_back(*lease);}
            const bool converted=image->ownsConvertedPixels();auto weak=std::weak_ptr<const MaterialTextureResourceCandidate>(ref.resource());ref={};pending.reset();stable.reset();image.reset();check(converted?weak.expired():!weak.expired());std::fill(b.begin(),b.end(),uint8_t(0xee));std::string pixels;for(const auto&lease:leases){const auto n=uint32_t(lease.size());pixels.append(reinterpret_cast<const char*>(&n),4);pixels.append(reinterpret_cast<const char*>(lease.data()),lease.size());}text(out,pixels);++available;
        }
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"output");std::cout<<"PASS owned_shader_CPU_pending_cases="<<count<<" available="<<available<<" safety_checks="<<checks<<" typed_publication=NOT_COMPLETED gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
