#include "material_texture_converted_image_candidate.h"
#include <iostream>
static void need(bool b){if(!b)throw std::runtime_error("block format admission safety");}
static std::vector<uint8_t>fixture(uint32_t format){
    std::vector<uint8_t>b(88,0);const auto word=[&](size_t at,uint32_t n){std::memcpy(b.data()+at,&n,4);};const auto half=[&](size_t at,uint16_t n){std::memcpy(b.data()+at,&n,2);};
    word(0,0x00465456);word(4,7);word(8,5);word(12,88);word(20,0x1000);half(16,4);half(18,4);half(24,2);word(52,format);b[56]=3;word(57,0xffffffff);half(63,1);word(68,1);word(80,0x30);word(84,88);
    for(unsigned i=0;i<6;++i){if(format==15){b.push_back(255);b.push_back(0);for(unsigned j=0;j<6;++j)b.push_back(255);}uint16_t a=0xf800,c=0x07e0;uint32_t selectors=0xffffffff;const auto start=b.size();b.resize(start+8);std::memcpy(b.data()+start,&a,2);std::memcpy(b.data()+start+2,&c,2);std::memcpy(b.data()+start+4,&selectors,4);}
    return b;
}
int main(){try{
    static_assert(sizeof(void*)==8,"owned x64 block admission");using Image=MaterialTextureCpuImageCandidate;using Converted=MaterialTextureConvertedImageCandidate;unsigned checks=0;const auto check=[&](bool b){need(b);++checks;};
    auto resource=MaterialTextureResourceCandidate::fromVtf("fixture/dxt5",fixture(15));auto ref=MaterialTextureReferenceCandidate::ordinary(resource);auto weak=std::weak_ptr<const MaterialTextureResourceCandidate>(resource);
    MaterialTextureCpuImageSettingsCandidate settings;settings.maxWidth=settings.maxHeight=64;settings.forceFullMipChain=true;settings.hardwareKnown=settings.overridesKnownAbsent=true;
    std::shared_ptr<const Image>image;check(Image::prepare(ref,settings,image));auto old=image;
    auto onebit=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/onebit",fixture(20)));check(!Image::prepare(onebit,settings,image)&&image==old);
    TextureFormatPolicyServicesCandidate calls;calls.enabled=[]()->std::optional<uint32_t>{return 1;};calls.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 12;};calls.hdrType=[]()->std::optional<uint32_t>{return 0;};
    std::shared_ptr<const Converted>converted;check(Converted::prepare(image,1,calls,converted));check(converted->format()==12&&converted->effectiveVtfFlags()==0x2000&&converted->plan()[0]==0x1000&&converted->rawFlags()==0x1000);
    auto saved=converted;check(!Converted::prepare(image,1,calls,converted,1)&&converted==saved);
    auto lease=converted->lease(1,2);check(lease&&lease->size()==4&&lease->data()[0]==0&&lease->data()[1]==170&&lease->data()[2]==85&&lease->data()[3]==36);
    saved.reset();converted.reset();old.reset();image.reset();ref={};resource.reset();check(weak.expired()&&lease->data()[3]==36);
    ref=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/passthrough",fixture(15)));check(Image::prepare(ref,settings,image));calls.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 15;};check(Converted::prepare(image,1,calls,converted)&&!converted->ownsConvertedPixels()&&converted->effectiveVtfFlags()==0x1000);
    lease=converted->lease(1,2);check(lease&&lease->size()==16&&lease->data()[0]==255&&lease->data()[1]==0);ref={};image.reset();converted.reset();check(lease->data()[2]==255);
    for(uint32_t bad:{14u,20u,24u}){ref=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/refuse",fixture(15)));check(Image::prepare(ref,settings,image));calls.nearestFormat=[bad](uint32_t,uint32_t)->std::optional<uint32_t>{return bad;};check(!Converted::prepare(image,1,calls,converted)&&!converted);}
    std::cout<<"PASS owned_block_admission_safety_checks="<<checks<<" format20=REFUSED original_runtime=ABSENT gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
