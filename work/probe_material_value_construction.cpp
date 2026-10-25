#include "material_value_construction_candidate.h"
#include "material_decimal_digits_candidate.h"
#include "material_decimal_powers_candidate.h"
#include "material_decimal_scaling_candidate.h"
#include "material_decimal_double_candidate.h"
static_assert(materialDecimalPositivePowersCandidate.size()==29&&materialDecimalNegativePowersCandidate.size()==29,"Scaling table dimensions");
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <xmmintrin.h>
#include "material_construction_native_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void analytical(){
    using Kind=MaterialValueConstructionKindCandidate;unsigned checks=0,refusals=0;
    auto check=[&](bool b){need(b,"Constructor analytical failure");++checks;};MaterialStringValueCandidate v;
    check(materialValueConstructCandidate(v,Kind::String,{},0,"1e3")&&v.numeric.integer==1000&&v.numeric.vector[0]==0x447a0000);
    check(materialStringSetCandidate(v,7,{},0,"1e3")&&v.numeric.integer==1);
    check(materialValueConstructCandidate(v,Kind::String,{},0,"16777217")&&v.numeric.integer==16777217&&v.numeric.vector[0]==0x4b800000);
    check(materialValueConstructCandidate(v,Kind::String,{},0,"4294967296",true)&&v.numeric.integer==0x80000000u);
    check(materialValueConstructCandidate(v,Kind::String,{},0,"4294967296",false)&&v.numeric.integer==0);
    check(materialValueConstructCandidate(v,Kind::Undefined)&&v.numeric.typeAndMetadata==0xffffff46&&v.numeric.integer==0&&v.text.empty());
    check(materialValueConstructCandidate(v,Kind::Integer,{7,0,0,0})&&v.numeric.typeAndMetadata==0xffffff44&&v.numeric.vector[3]==0x40e00000);
    check(materialConstructorDoubleCandidate("9007199254740995")==9007199254740994.0);
    check(materialValueConstructCandidate(v,Kind::String,{},0,"\t6459691196903481.5",false)&&v.numeric.integer==254177337);
    for(auto bad:std::array<std::array<uint32_t,2>,5>{{{5,0},{2,0},{2,5},{0,1},{3,1}}}){
        const auto old=v;check(!materialValueConstructCandidate(v,Kind(bad[0]),{},bad[1])&&v.text==old.text&&!std::memcmp(&v.numeric,&old.numeric,24));++refusals;
    }
    const auto old=v;check(!materialValueConstructCandidate(v,Kind::String,{},0,std::string(4096,'a'))&&!std::memcmp(&v.numeric,&old.numeric,24)&&v.text==old.text);++refusals;
    std::cout<<"PASS construction_analytical_checks="<<checks<<" refusal_cases="<<refusals<<'\n';
}
int main(int argc,char**argv){try{
    if(argc==5&&std::string(argv[1])=="--parse-batch"){
        need(!std::ifstream(argv[4],std::ios::binary).good(),"Fresh parser output required");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        MaterialConstructionNativeFixture fixture(argv[2]);
#endif
        std::ifstream input(argv[3],std::ios::binary);std::ofstream output(argv[4],std::ios::binary);need(bool(input)&&bool(output),"Parser IO");
        uint32_t size=0;size_t count=0;
        while(input.read(reinterpret_cast<char*>(&size),4)){
            need(size<=4095,"Parser input length");std::string s(size,'\0');need(bool(input.read(s.data(),size)),"Parser string framing");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
            using Parse=double(__cdecl*)(const char*);const double d=reinterpret_cast<Parse>(0x1007e637)(s.c_str());
#else
            const double d=materialConstructorDoubleCandidate(s);
#endif
            output.write(reinterpret_cast<const char*>(&d),8);++count;
        }
        output.flush();need(input.eof()&&input.gcount()==0&&bool(output)&&count,"Parser framing/write");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        fixture.image.verify();
#endif
        std::cout<<"PASS parser_cases="<<count<<'\n';return 0;
    }
    if(argc==5&&std::string(argv[1])=="--convert-batch"){
        need(!std::ifstream(argv[4],std::ios::binary).good(),"Fresh conversion output required");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        MaterialConstructionNativeFixture fixture(argv[2]);
#endif
        std::ifstream input(argv[3],std::ios::binary);std::ofstream output(argv[4],std::ios::binary);need(bool(input)&&bool(output),"Conversion IO");
        std::array<uint8_t,12> wide{};size_t count=0;
        while(input.read(reinterpret_cast<char*>(wide.data()),12)){
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
            uint64_t bits=0;using Convert=int(__cdecl*)(const void*,void*);
            const int status=reinterpret_cast<Convert>(0x1008e861)(wide.data(),&bits);
#else
            const auto value=materialDecimalDoubleCandidate(wide);const auto bits=value.bits;const int status=value.status;
#endif
            output.write(reinterpret_cast<const char*>(&bits),8);output.write(reinterpret_cast<const char*>(&status),4);++count;
        }
        output.flush();need(input.eof()&&input.gcount()==0&&bool(output)&&count,"Conversion framing/write");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        fixture.image.verify();
#endif
        std::cout<<"PASS conversion_cases="<<count<<'\n';return 0;
    }
    if(argc==5&&std::string(argv[1])=="--wide-batch"){
        need(!std::ifstream(argv[4],std::ios::binary).good(),"Fresh wide output required");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        MaterialConstructionNativeFixture fixture(argv[2]);
#endif
        std::ifstream input(argv[3],std::ios::binary);std::ofstream output(argv[4],std::ios::binary);
        need(bool(input)&&bool(output),"Wide IO");
        std::string text;size_t count=0;
        while(std::getline(input,text)){
            need(text.size()<=4095,"Wide input length");std::array<uint8_t,12> wide{};
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
            const char*end=nullptr;const uintptr_t locale[]{0x10104b00,0x10104470};
            using Parse=int(__cdecl*)(void*,const char**,const char*,int,int,int,int,const void*);
            reinterpret_cast<Parse>(0x10087b86)(wide.data(),&end,text.c_str(),0,0,0,0,locale);
            need(end==text.c_str()+text.size(),"Wide complete consumption");
#else
            const auto split=text.find('e');need(split!=std::string::npos,"Canonical coefficient/exponent");
            need(materialDecimalScaleCandidate(text.substr(0,split),std::stoi(text.substr(split+1)),wide),"Owned scaling input");
#endif
            output.write(reinterpret_cast<const char*>(wide.data()),12);++count;
        }
        output.flush();need(input.eof()&&bool(output)&&count,"Wide framing/write");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        fixture.image.verify();
#endif
        std::cout<<"PASS wide_cases="<<count<<'\n';return 0;
    }
    if(argc==5&&std::string(argv[1])=="--digits-batch"){
        need(!std::ifstream(argv[4],std::ios::binary).good(),"Fresh digits output required");
        std::ifstream input(argv[3],std::ios::binary);std::ofstream output(argv[4],std::ios::binary);
        need(bool(input)&&bool(output),"Digits IO");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        MaterialConstructionNativeFixture fixture(argv[2]);
#endif
        size_t cases=0;std::string digits;
        while(std::getline(input,digits)){
            std::array<uint8_t,12> result{};
            need(materialDecimalDigitsCandidate(digits,result),"Nonzero 1..24 decimal digits required");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
            std::array<uint8_t,24> numeric{};
            for(size_t i=0;i<digits.size();++i)numeric[i]=uint8_t(digits[i]-'0');
            using Digits=void(__cdecl*)(const void*,unsigned,void*);
            reinterpret_cast<Digits>(0x1008f65e)(numeric.data(),unsigned(digits.size()),result.data());
#endif
            output.write(reinterpret_cast<const char*>(result.data()),result.size());++cases;
        }
        output.flush();need(input.eof()&&bool(output)&&cases,"Digits framing/write");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        fixture.image.verify();
#endif
        std::cout<<"PASS digits_cases="<<cases<<'\n';return 0;
    }
    if(argc==2&&std::string(argv[1])=="--test"){analytical();return 0;}
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
    if(argc==4&&std::string(argv[1])=="--parse-wide"){
        MaterialConstructionNativeFixture f(argv[2]);
        // Match the eight cdecl arguments supplied by 0x10086901..10086913.
        // The parser receives a locale pair, not a live CRT/engine context.
        const uintptr_t locale[]{0x10104b00,0x10104470};
        std::array<uint8_t,12> wide{};const char* end=nullptr;
        using Parse=int(__cdecl*)(void*,const char**,const char*,int,int,int,int,const void*);
        const int status=reinterpret_cast<Parse>(0x10087b86)(wide.data(),&end,argv[3],0,0,0,0,locale);
        uint64_t bits=0;using Convert=int(__cdecl*)(const void*,void*);
        const int converted=reinterpret_cast<Convert>(0x1008e861)(wide.data(),&bits);
        std::cout<<"status="<<status<<" consumed="<<(end-argv[3])<<" converted="<<converted<<" wide=";
        static const char hex[]="0123456789abcdef";
        for(auto b:wide)std::cout<<hex[b>>4]<<hex[b&15];
        std::cout<<" double="<<std::hex<<bits<<'\n';f.image.verify();return 0;
    }
    if(argc==4&&std::string(argv[1])=="--parse-double"){
        MaterialConstructionNativeFixture f(argv[2]);unsigned control=0;_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);_mm_setcsr(0x1f80);
        using Parse=double(__cdecl*)(const char*);const double d=reinterpret_cast<Parse>(0x1007e637)(argv[3]);uint64_t bits=0;std::memcpy(&bits,&d,8);
        std::cout<<"native_double_bits="<<std::hex<<bits<<'\n';f.image.verify();return 0;
    }
#endif
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"library queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Construction IO");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
    static_assert(sizeof(void*)==4,"Private reference only");MaterialConstructionNativeFixture f(argv[1]);
    const unsigned oldMxcsr=_mm_getcsr();unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"Owned x64");
#endif
    std::array<uint32_t,9>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[0]<=4&&q[2]<=1&&q[8]<=4095&&(q[0]==2?(q[1]>=1&&q[1]<=4):q[1]==0)&&(q[0]==3||q[8]==0),"Constructor input domain");
        std::string text(q[8],'\0');if(q[8])need(bool(in.read(&text[0],q[8])),"Text framing");const std::array<uint32_t,4>a{q[4],q[5],q[6],q[7]};MaterialStringValueCandidate value;
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        f.begin(q[2]!=0);_mm_setcsr(0x1f80);std::array<uint8_t,40>object;object.fill(uint8_t(q[3]));
        constexpr uintptr_t methods[]{0x100179b0,0x10017910,0x10017850,0x10017780,0x10017a50};
        if(q[0]<2){using Fn=void*(__thiscall*)(void*,void*,const char*,uint32_t);reinterpret_cast<Fn>(methods[q[0]])(object.data(),nullptr,nullptr,a[0]);}
        else if(q[0]==2){using Fn=void*(__thiscall*)(void*,void*,const char*,const void*,int);reinterpret_cast<Fn>(methods[2])(object.data(),nullptr,nullptr,a.data(),int(q[1]));}
        else if(q[0]==3){using Fn=void*(__thiscall*)(void*,void*,const char*,const char*);reinterpret_cast<Fn>(methods[3])(object.data(),nullptr,nullptr,text.c_str());}
        else{using Fn=void*(__thiscall*)(void*,void*,const char*);reinterpret_cast<Fn>(methods[4])(object.data(),nullptr,nullptr);}
        std::memcpy(&value.numeric.integer,object.data()+8,4);std::memcpy(value.numeric.vector.data(),object.data()+12,16);std::memcpy(&value.numeric.typeAndMetadata,object.data()+28,4);
        uintptr_t pointer=0;std::memcpy(&pointer,object.data(),4);need(pointer==0x10098f34,"Constructor vtable");std::memcpy(&pointer,object.data()+32,4);need(!pointer,"Constructor owner");
        if(q[0]==3){std::memcpy(&pointer,object.data()+4,4);value.text=reinterpret_cast<const char*>(pointer);}else{std::memcpy(&pointer,object.data()+4,4);need(!pointer,"Numeric constructor cached string");}
        need(f.queues==0&&f.allocations==(q[0]==3?1u:0u),"Constructor service calls");
#else
        need(materialValueConstructCandidate(value,MaterialValueConstructionKindCandidate(q[0]),a,q[1],text,q[2]!=0),"Owned constructor rejected");
#endif
        const uint32_t length=uint32_t(value.text.size());out.write(reinterpret_cast<const char*>(&value.numeric),24);out.write(reinterpret_cast<const char*>(&length),4);out.write(value.text.data(),length);
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
        using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x1001a0a0)(object.data());need(f.allocations==f.releases,"Constructor release imbalance");
#endif
        ++cases;
    }
    need(in.eof()&&in.gcount()==0&&cases,"Constructor framing");out.flush();need(bool(out),"Construction output");
#ifdef SFM64_NATIVE_MATERIAL_CONSTRUCTION
    f.image.verify();_mm_setcsr(oldMxcsr);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);
#endif
    std::cout<<"PASS material_construction_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
