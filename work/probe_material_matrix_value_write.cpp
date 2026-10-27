#include "material_matrix_value_write_candidate.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#ifdef SFM64_NATIVE_MATRIX_WRITE
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_construction_native_fixture.h"
#endif
static void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"library-or-absent query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Matrix value IO");
#ifdef SFM64_NATIVE_MATRIX_WRITE
    MaterialConstructionNativeFixture f(argv[1]);
    unsigned allocations=0,releases=0;
#else
    static_assert(sizeof(void*)==8,"Owned matrix values x64");
#endif
    const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);unsigned cases=0;std::array<uint32_t,45>q{};
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        need(q[44]<4096,"Matrix input text extent");std::string text(q[44],'\0');need(bool(in.read(text.data(),text.size())),"Matrix text input");const auto originalQuery=q;const auto originalText=text;MaterialTypedDispatchCandidate value;value.created=true;value.name="$matrix-var";std::memcpy(&value.value.numeric,q.data(),24);const auto type=q[0]&15u;need(materialStringDomainCandidate(type)||type==7,"Checked initial type");
        need((q[22]<=7||q[22]==9)&&((q[22]==5&&q[23]>=1&&q[23]<=4)||(q[22]!=5&&q[23]==0))&&(q[22]==7||text.empty()),"Checked operation domain");
        MaterialMatrixMathCandidate initial{},matrix{};std::copy_n(q.begin()+6,16,initial.begin());std::copy_n(q.begin()+28,16,matrix.begin());const auto matrixBefore=matrix;std::array<uint32_t,4>args{};std::copy_n(q.begin()+24,4,args.begin());if(type==7){value.matrix=initial;value.matrixIdentity=materialMatrixIdentityValueCandidate(initial);}if(type==1)value.value.text="old";
        MaterialMatrixMathCandidate getter{};bool getterIdentity=false;
#ifdef SFM64_NATIVE_MATRIX_WRITE
        f.begin(false);std::array<uint8_t,40>raw{};const uintptr_t table=0x10098f34;std::memcpy(raw.data(),&table,4);std::memcpy(raw.data()+8,&value.value.numeric.integer,4);std::memcpy(raw.data()+12,value.value.numeric.vector.data(),16);std::memcpy(raw.data()+28,&q[0],4);
        if(type==7){auto*p=static_cast<uint8_t*>(MaterialConstructionNativeFixture::alloc(nullptr,nullptr,65));std::memcpy(p,initial.data(),64);p[64]=uint8_t(value.matrixIdentity);auto address=reinterpret_cast<uintptr_t>(p);std::memcpy(raw.data()+36,&address,4);}
        if(type==1){auto*p=static_cast<char*>(MaterialConstructionNativeFixture::alloc(nullptr,nullptr,4));std::memcpy(p,"old",4);auto address=reinterpret_cast<uintptr_t>(p);std::memcpy(raw.data()+4,&address,4);}
        if(q[22]==9){using Call=void(__thiscall*)(void*,const void*);reinterpret_cast<Call>(0x10019f70)(raw.data(),matrix.data());need(f.allocations==unsigned(type==1||type==7)+1&&f.releases==unsigned(type==1||type==7),"Original matrix setter replaces backing even for equal input");}
        else if(q[22]==0||q[22]==1){using Call=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Call>(q[22]?0x10019350:0x10019460)(raw.data(),args[0]);}
        else if(q[22]==2){using Call=void(__thiscall*)(void*,uint32_t,uint32_t);reinterpret_cast<Call>(0x10019cd0)(raw.data(),args[0],args[1]);}
        else if(q[22]==3){using Call=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);reinterpret_cast<Call>(0x10019d10)(raw.data(),args[0],args[1],args[2]);}
        else if(q[22]==4){using Call=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t);reinterpret_cast<Call>(0x10019d50)(raw.data(),args[0],args[1],args[2],args[3]);}
        else if(q[22]==5){using Call=void(__thiscall*)(void*,const void*,uint32_t);reinterpret_cast<Call>(0x10019b00)(raw.data(),args.data(),q[23]);}
        else if(q[22]==6){using Call=void(__thiscall*)(void*);reinterpret_cast<Call>(0x1001a0a0)(raw.data());}
        else if(q[22]==7){using Call=void(__thiscall*)(void*,const char*);reinterpret_cast<Call>(0x1001a580)(raw.data(),text.c_str());}
        else throw std::runtime_error("Unchecked native operation");
        std::memcpy(&value.value.numeric.integer,raw.data()+8,4);std::memcpy(value.value.numeric.vector.data(),raw.data()+12,16);std::memcpy(&value.value.numeric.typeAndMetadata,raw.data()+28,4);value.value.text.clear();value.matrix.reset();value.matrixIdentity=false;
        if((value.value.numeric.typeAndMetadata&15u)==7){uintptr_t address=0;std::memcpy(&address,raw.data()+36,4);need(address!=0,"Native matrix backing");value.matrix.emplace();std::memcpy(value.matrix->data(),reinterpret_cast<void*>(address),64);value.matrixIdentity=*reinterpret_cast<uint8_t*>(address+64)!=0;}
        if((value.value.numeric.typeAndMetadata&15u)==1){uintptr_t address=0;std::memcpy(&address,raw.data()+4,4);value.value.text=reinterpret_cast<const char*>(address);}
        const auto beforeGet=raw;using Get=const void*(__thiscall*)(void*);auto*read=reinterpret_cast<Get>(0x10018d60)(raw.data());need(read!=nullptr,"Original matrix getter");std::memcpy(getter.data(),read,64);using Identity=bool(__thiscall*)(void*);getterIdentity=reinterpret_cast<Identity>(0x10018430)(raw.data());need(raw==beforeGet,"Original getters preserve variable storage");using Free=void(__thiscall*)(void*);reinterpret_cast<Free>(0x10017ad0)(raw.data());need(f.allocations==f.releases,"Original backing balance");allocations+=f.allocations;releases+=f.releases;f.image.verify();
#else
        if(q[22]==9)need(materialTypedMatrixSetCandidate(value,matrix),"Owned matrix set");else need(materialTypedNumericWriteCandidate(value,q[22],args,q[23],text),"Owned numeric after matrix");auto read=materialTypedMatrixReadCandidate(value);need(bool(read),"Owned matrix read");getter=*read;getterIdentity=materialTypedMatrixIsIdentityCandidate(value);
#endif
        need(q==originalQuery&&text==originalText&&matrix==matrixBefore,"Borrowed input unchanged");out.write(reinterpret_cast<const char*>(&value.value.numeric),24);uint32_t has=value.matrix.has_value(),identity=value.matrixIdentity,readIdentity=getterIdentity;out.write(reinterpret_cast<const char*>(&has),4);out.write(reinterpret_cast<const char*>(&identity),4);out.write(reinterpret_cast<const char*>(&readIdentity),4);MaterialMatrixMathCandidate backing=value.matrix?*value.matrix:MaterialMatrixMathCandidate{};out.write(reinterpret_cast<const char*>(backing.data()),64);out.write(reinterpret_cast<const char*>(getter.data()),64);uint32_t length=uint32_t(value.value.text.size());out.write(reinterpret_cast<const char*>(&length),4);out.write(value.value.text.data(),length);++cases;
    }
    need(in.eof()&&in.gcount()==0&&cases,"Matrix query framing");out.flush();need(bool(out),"Complete matrix output");_mm_setcsr(mx);
#ifdef SFM64_NATIVE_MATRIX_WRITE
    std::cout<<"native_allocations="<<allocations<<" releases="<<releases<<'\n';
#endif
    std::cout<<"PASS matrix_value_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
