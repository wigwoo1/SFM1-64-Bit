#include "model_prepared_texture_candidate.h"
#include "vtf_block_decode.h"
#include <iostream>

static unsigned checks=0;
static void check(bool ok){if(!ok)throw std::runtime_error("LOD preparation acceptance");++checks;}
static void word(std::vector<uint8_t>&b,size_t at,uint32_t v){std::memcpy(b.data()+at,&v,4);}
static void half(std::vector<uint8_t>&b,size_t at,uint16_t v){std::memcpy(b.data()+at,&v,2);}
static std::vector<uint8_t>fixture(bool lod=true,uint32_t flags=0,uint8_t levels=5){
    const uint32_t header=lod?96u:88u;std::vector<uint8_t>b(header);
    for(auto kv:{std::pair<size_t,uint32_t>{0,0x00465456},{4,7},{8,5},{12,header},{20,flags},
        {52,0},{57,0xffffffff},{68,lod?2u:1u},{80,0x30},{84,header}})word(b,kv.first,kv.second);
    half(b,16,16);half(b,18,8);half(b,24,2);half(b,63,1);b[56]=levels;
    if(lod){word(b,88,0x02444f4c);word(b,92,0xfeff0202);}
    for(uint32_t n=levels;n;--n)for(uint32_t frame=0;frame<2;++frame){
        const auto mip=n-1,w=std::max(1u,16u>>mip),h=std::max(1u,8u>>mip);
        for(uint32_t i=0;i<w*h*4;++i)b.push_back(uint8_t(mip*43+frame*71+i*17+3));
    }
    return b;
}
static MaterialTextureCpuImageSettingsCandidate settings(){
    MaterialTextureCpuImageSettingsCandidate s;s.maxWidth=s.maxHeight=64;
    s.forceFullMipChain=s.hardwareKnown=s.overridesKnownAbsent=true;return s;
}
// Named authored RGBA colors, with distinct red/blue and alpha values. The
// serializer writes BGRA; the independent expected byte table below is literal.
static const std::array<std::array<uint8_t,4>,10>namedRgba{{
    {241,17,43,199},{29,211,61,173},{37,71,229,157},{223,181,19,149},
    {197,23,163,137},{13,193,217,127},{151,89,47,113},{73,31,191,101},
    {181,137,53,83},{59,167,239,67}}};
static const std::array<std::array<uint8_t,4>,10>expectedBgra{{
    {43,17,241,199},{61,211,29,173},{229,71,37,157},{19,181,223,149},
    {163,23,197,137},{217,193,13,127},{47,89,151,113},{191,31,73,101},
    {53,137,181,83},{239,167,59,67}}};
static std::vector<uint8_t>bgraFixture(uint32_t flags=0,uint8_t levels=5){
    auto b=fixture(true,flags,levels);b.resize(96);word(b,52,12);
    // Discard the other fixture's payload completely. This is authored BGRA,
    // not an RGBA image relabeled as format12.
    for(uint32_t n=levels;n;--n)for(uint32_t frame=0;frame<2;++frame){
        const auto level=n-1,w=std::max(1u,16u>>level),h=std::max(1u,8u>>level);
        for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){
            const auto&color=namedRgba[(2*level+frame+2*(y%2)+x%2)%10];
            b.insert(b.end(),{color[2],color[1],color[0],color[3]});
        }
    }
    return b;
}
static void bgraSafety(){
    const auto preceding=checks;auto b=bgraFixture();const auto before=b;
    const auto layout=readVtfTextureLayoutCandidate(b);
    check(layout.format==12&&layout.frameCount==2&&layout.mips.size()==5);
    check(layout.mips[0].width==16&&layout.mips[0].height==8);
    uint32_t decodedPixels=0,retainedPixels=0,selectedBlocks=0;
    // Canonical RGBA expectations independently verify the authored BGRA bytes.
    for(uint32_t frame=0;frame<2;++frame)for(uint32_t source=0;source<5;++source){
        const auto decoded=decodeVtfMipCandidate(b,source,frame);
        for(uint32_t y=0;y<decoded.height;++y)for(uint32_t x=0;x<decoded.width;++x){
            const auto&expected=namedRgba[(2*source+frame+2*(y%2)+x%2)%10];
            const auto offset=(size_t(y)*decoded.width+x)*4;
            check(std::equal(expected.begin(),expected.end(),decoded.rgba.begin()+offset));++decodedPixels;
        }
    }
    const std::array<uint32_t,6>requests{0,1,2,3,4,100},selected{0,0,0,1,2,2};
    const std::array<uint32_t,3>sources{2,3,4},widths{4,2,1},heights{2,1,1};
    for(uint32_t frame=0;frame<2;++frame)for(size_t request=0;request<requests.size();++request){
        const auto snapshot=prepareModelTextureSnapshotCandidate("owned/bgra",b,requests[request],frame);
        check(bool(snapshot.cpu));const auto&image=*snapshot.cpu->image();const auto&p=image.plan();
        check(image.rawFlags()==0&&image.format()==12&&p[2]==12&&p[3]==16&&p[4]==8);
        check(p[6]==4&&p[7]==2&&p[9]==3&&p[10]==2&&p[11]==2);
        check(snapshot.sourceMip==selected[request]&&snapshot.frame==frame);
        for(uint32_t level=0;level<3;++level){const auto lease=image.lease(frame,level);
            check(lease&&lease->sourceLevel()==sources[level]&&lease->frame()==frame);
            check(lease->width()==widths[level]&&lease->height()==heights[level]&&lease->size()==size_t(widths[level])*heights[level]*4);
            for(uint32_t y=0;y<heights[level];++y)for(uint32_t x=0;x<widths[level];++x){
                const auto&expected=expectedBgra[(2*sources[level]+frame+2*(y%2)+x%2)%10];
                const auto offset=(size_t(y)*widths[level]+x)*4;
                check(std::equal(expected.begin(),expected.end(),lease->data()+offset));++retainedPixels;
            }
            ++selectedBlocks;
        }
        check(!image.lease(2,0)&&!image.lease(0,3));
    }
    check(b==before);
    auto resource=MaterialTextureResourceCandidate::fromVtf("owned/bgra-lease",b);
    std::weak_ptr<const MaterialTextureResourceCandidate>weak=resource;
    auto ref=MaterialTextureReferenceCandidate::ordinary(resource);std::shared_ptr<const MaterialTextureCpuImageCandidate>image;
    check(MaterialTextureCpuImageCandidate::prepare(ref,settings(),image));auto committed=image;
    auto unavailable=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("owned/bgra-partial",bgraFixture(0x100,1)));
    check(!MaterialTextureCpuImageCandidate::prepare(unavailable,settings(),image)&&image==committed);
    auto fallback=prepareModelTextureSnapshotCandidate("owned/bgra-fallback",bgraFixture(0x100,1),0,1);
    check(!fallback.cpu&&fallback.sourceMip==0&&fallback.frame==1);
    bool invalidFrame=false;try{(void)prepareModelTextureSnapshotCandidate("owned/bgra",b,0,2);}catch(const std::exception&){invalidFrame=true;}check(invalidFrame);
    auto lease=image->lease(1,2);check(lease&&lease->sourceLevel()==4);
    b.assign(b.size(),0xee);resource.reset();ref={};image.reset();committed.reset();
    check(!weak.expired()&&std::equal(expectedBgra[9].begin(),expectedBgra[9].end(),lease->data()));
    lease.reset();check(weak.expired());
    std::cout<<"PASS bgra_lod_checks="<<checks-preceding<<" selected_blocks="<<selectedBlocks
        <<" retained_pixels="<<retainedPixels<<" decoded_pixels="<<decodedPixels<<" frames=2 selected_levels=3 original_runtime=ABSENT\n";
}
int main(){try{
    static_assert(sizeof(void*)==8,"independent native x64 test");
    auto bytes=fixture();const auto before=bytes;auto layout=readVtfTextureLayoutCandidate(bytes);
    check(layout.lodControl&&(*layout.lodControl)[2]==255&&(*layout.lodControl)[3]==254);
    check(layout.mips[0].width==16&&layout.mips[0].height==8&&layout.mips.size()==5);
    for(uint32_t frame=0;frame<2;++frame)for(uint32_t requested:{0u,1u,2u,3u,4u,100u}){
        auto snapshot=prepareModelTextureSnapshotCandidate("owned/lod",bytes,requested,frame);
        check(bool(snapshot.cpu));const auto&image=*snapshot.cpu->image();const auto&p=image.plan();
        const auto selected=requested<=2?0u:requested==3?1u:2u;
        check(p[3]==16&&p[4]==8&&p[6]==4&&p[7]==2&&p[9]==3&&p[11]==2);
        check(snapshot.sourceMip==selected&&snapshot.frame==frame);
        auto lease=image.lease(frame,selected);check(lease&&lease->sourceLevel()==selected+2&&lease->frame()==frame);
        check(lease->data()[0]==uint8_t((selected+2)*43+frame*71+3));
        check(!image.lease(2,0)&&!image.lease(0,3));
    }
    check(bytes==before);
    // Preserve the prior absent-resource contract, including its historical
    // oversized request field (the Vulkan consumer clamps it independently).
    auto absent=prepareModelTextureSnapshotCandidate("owned/absent",fixture(false),100,0);
    check(absent.cpu&&absent.sourceMip==100&&absent.cpu->image()->plan()[11]==0);
    check(!prepareModelTextureSnapshotCandidate("owned/no-mip",fixture(false,0x100),1,0).cpu);
    auto noMip=prepareModelTextureSnapshotCandidate("owned/no-mip-lod",fixture(true,0x100),0,1);
    check(noMip.cpu&&noMip.cpu->image()->plan()[11]==2&&noMip.cpu->image()->plan()[9]==1&&noMip.sourceMip==0);
    for(uint32_t flags:{0u,0x100u}){
        auto fallback=prepareModelTextureSnapshotCandidate("owned/unavailable-lod",fixture(true,flags,1),0,1);
        check(!fallback.cpu&&fallback.sourceMip==0&&fallback.frame==1);
    }
    auto resource=MaterialTextureResourceCandidate::fromVtf("owned/lease",bytes);
    std::weak_ptr<const MaterialTextureResourceCandidate>weak=resource;
    auto ref=MaterialTextureReferenceCandidate::ordinary(resource);std::shared_ptr<const MaterialTextureCpuImageCandidate>image;
    check(MaterialTextureCpuImageCandidate::prepare(ref,settings(),image));auto committed=image;
    for(uint32_t exponent:{31u,32u,127u,128u,255u}){
        auto invalid=bytes;invalid[92]=uint8_t(exponent);auto input=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("owned/refuse",invalid));
        check(!MaterialTextureCpuImageCandidate::prepare(input,settings(),image)&&image==committed);
        check(!prepareModelTextureSnapshotCandidate("owned/refuse",invalid,0,0).cpu);
    }
    for(auto exponents:{std::array<uint8_t,2>{0,2},{2,0},{0,0},{30,30}}){
        auto special=bytes;special[92]=exponents[0];special[93]=exponents[1];auto s=prepareModelTextureSnapshotCandidate("owned/sentinel",special,0,0);
        check(s.cpu&&s.cpu->image()->plan()[11]==0&&s.cpu->image()->plan()[6]==16);
    }
    auto partial=MaterialTextureReferenceCandidate::ordinary(MaterialTextureResourceCandidate::fromVtf("owned/partial",fixture(true,0,1)));
    check(!MaterialTextureCpuImageCandidate::prepare(partial,settings(),image)&&image==committed);
    auto malformed=[&](std::vector<uint8_t>b){bool refused=false;try{(void)readVtfTextureLayoutCandidate(b);}catch(const std::exception&){refused=true;}check(refused);};
    auto duplicate=bytes;duplicate.insert(duplicate.begin()+96,8,0);word(duplicate,12,104);word(duplicate,68,3);word(duplicate,84,104);word(duplicate,96,0x02444f4c);word(duplicate,100,0x0202);malformed(duplicate);
    for(uint32_t type:{0x00444f4cu,0x03444f4cu}){auto b=bytes;word(b,88,type);malformed(b);}
    auto shortTable=bytes;word(shortTable,12,88);malformed(shortTable);
    auto truncated=bytes;truncated.pop_back();malformed(truncated);
    TextureImagePlanInputCandidate planInput;planInput.width=16;planInput.height=8;planInput.frames=2;planInput.format=0;
    planInput.maxWidth=planInput.maxHeight=64;planInput.hardwareKnown=planInput.overridesKnownAbsent=true;
    TextureImagePlanCandidate plan;plan.fill(0xaabbccdd);const auto sentinel=plan;
    check(!textureImagePlanCandidate(planInput,plan)&&plan==sentinel);
    planInput.minor=5;planInput.lodResourceKnownAbsent=true;planInput.lodControl=std::array<uint8_t,4>{2,2,0,0};
    check(!textureImagePlanCandidate(planInput,plan)&&plan==sentinel);
    auto lease=image->lease(1,1);check(lease&&lease->sourceLevel()==3);
    bytes.assign(bytes.size(),0xee);resource.reset();ref={};image.reset();
    // Drop the final image owner too, leaving only the pixel lease.
    committed.reset();check(!weak.expired()&&lease->data()[0]==uint8_t(3*43+71+3));
    lease.reset();check(weak.expired());
    std::cout<<"PASS lod_preparation_checks="<<checks<<" authored_selected_mapping=1 original_runtime=ABSENT\n";
    if(checks!=106)throw std::runtime_error("preceding LOD unit coverage changed");
    bgraSafety();
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
