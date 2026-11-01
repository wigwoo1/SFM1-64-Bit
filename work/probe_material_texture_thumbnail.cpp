#include "material_texture_thumbnail_candidate.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using Thumb=MaterialTextureThumbnailCandidate;using Ref=MaterialTextureReferenceCandidate;
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t word(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"input word");return n;}
static std::string text(std::istream&i){auto n=word(i);need(n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"input text");return s;}
static void write(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned native x64 thumbnails");
    need(argc==3&&!std::ifstream(argv[2]).good(),"queries fresh-output");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
    const auto count=word(in);need(count&&count<=4096,"query extent");uint32_t checks=0,available=0;
    for(uint32_t serial=0;serial<count;++serial){for(unsigned i=0;i<7;++i)(void)word(in);const auto logical=text(in),key=text(in),source=text(in);(void)logical;
        std::ifstream f(source,std::ios::binary);need(bool(f),"snapshot input");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});const auto before=b;Ref ref;
        try{ref=Ref::ordinary(MaterialTextureResourceCandidate::fromVtf(key,b));}catch(const std::exception&){}
        std::shared_ptr<const Thumb>image;const bool ok=Thumb::prepare(ref,image);need(b==before,"input preserved");write(out,serial);write(out,uint32_t(ok));
        if(ok){const auto snapshot=image;const auto expected=image->rgb();std::weak_ptr<const MaterialTextureResourceCandidate>weak=ref.resource();ref={};std::fill(b.begin(),b.end(),uint8_t(0xee));need(weak.expired()&&image->rgb()==expected,"converted pixels own storage");++checks;
            auto reject=[&](Ref bad){need(!Thumb::prepare(bad,image)&&image==snapshot,"failure preserves output");++checks;};reject({});reject(Ref::environment());reject(Ref::ordinary(nullptr));
            if(serial==1){for(uint32_t mode=0;mode<3;++mode){const uint32_t format=mode?13:3;std::vector<uint8_t>bad(format==3?108u:112u,0);
                const auto put=[&](size_t at,uint32_t n){std::memcpy(bad.data()+at,&n,4);};const auto half=[&](size_t at,uint16_t n){std::memcpy(bad.data()+at,&n,2);};
                put(0,0x00465456);put(4,7);put(8,2);put(12,80);half(16,2);half(18,2);half(24,1);put(52,0);bad[56]=1;put(57,format);bad[61]=mode==0?2:mode==1?6:3;bad[62]=mode==0?2:mode==1?4:5;half(63,1);
                reject(Ref::ordinary(MaterialTextureResourceCandidate::fromVtf("fixture/unsupported",bad)));}}
            write(out,image->width());write(out,image->height());write(out,uint32_t(image->rgb().size()));out.write(reinterpret_cast<const char*>(image->rgb().data()),std::streamsize(image->rgb().size()));image.reset();need(snapshot->rgb()==expected,"snapshot retains thumbnail");++checks;++available;
        }
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"output write");std::cout<<"PASS owned_thumbnail_cases="<<count<<" available="<<available<<" safety_checks="<<checks<<" original_runtime=ABSENT gpu_calls=0 editor=NOT_CONNECTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
