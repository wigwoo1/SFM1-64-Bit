#include "source_material_patch_policy_candidate.h"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef NATIVE_MATERIAL_KEYVALUES_PARSER
#define NOMINMAX
#include <windows.h>
#include "native_material_keyvalues_fixture.h"
#else
#include "source_material_keyvalues_parser_candidate.h"
#endif
template<class T>static T read(std::istream&in){T v;if(!in.read(reinterpret_cast<char*>(&v),sizeof(v)))throw std::runtime_error("parser fixture truncated");return v;}
static std::string text(std::istream&in){auto n=read<uint32_t>(in);if(n>(1u<<24))throw std::runtime_error("source extent");std::string s(n,'\0');if(!in.read(s.data(),n))throw std::runtime_error("source truncated");return s;}
template<class T>static void put(std::ostream&out,T v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static void writeNode(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);if(n.kind==1)putText(out,n.text);else if(n.kind==2||n.kind==3)put(out,n.bits);else if(n.kind==4||n.kind==7)put(out,n.pointer);put(out,uint32_t(n.children.size()));for(const auto&child:n.children)writeNode(out,child);}
#ifdef NATIVE_MATERIAL_KEYVALUES_PARSER
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"native parser fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("copied library directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("fixture streams");
#ifdef NATIVE_MATERIAL_KEYVALUES_PARSER
 static_assert(sizeof(void*)==4,"original x86 parser");AddVectoredExceptionHandler(1,fault);NativeMaterialKeyValuesFixture native(argv[1]);
#else
 static_assert(sizeof(void*)==8,"owned x64 parser");
#endif
auto count=read<uint32_t>(in);if(!count||count>100000)throw std::runtime_error("fixture count");put(out,count);
#ifdef NATIVE_MATERIAL_KEYVALUES_PARSER
 using BufferConstruct=void*(__thiscall*)(void*,int,int,uint8_t);using External=void(__thiscall*)(void*,void*,int,int,uint8_t);using Load=bool(__thiscall*)(void*,const char*,void*,void*,const char*,void*);using Errno=int*(__cdecl*)();
#endif
 for(uint32_t i=0;i<count;++i){auto name=text(in),source=text(in);
#ifdef NATIVE_MATERIAL_KEYVALUES_PARSER
if(source.find('\0')!=std::string::npos)throw std::runtime_error("NUL source outside fixture scope");std::vector<uint8_t>data(source.size()+34,0xb6);std::memcpy(data.data()+16,source.data(),source.size());data[source.size()+16]=0;data[source.size()+17]=0;const auto before=data;alignas(16)std::array<uint8_t,80>record;record.fill(0xcd);std::fill(record.begin()+16,record.end()-16,uint8_t(0));const auto guard=record;auto buffer=record.data()+16;reinterpret_cast<BufferConstruct>(native.at(0x6f6d0))(buffer,0,0,0);reinterpret_cast<External>(native.at(0x6f7e0))(buffer,data.data()+16,int(source.size()+2),int(source.size()),9);SourceMaterialPatchNodeCandidate initial;initial.key="placeholder";auto root=native.build(initial);*reinterpret_cast<Errno>(native.at(0x82f6d))()=0;const auto ok=reinterpret_cast<Load>(native.at(0x720a0))(root,name.c_str(),buffer,nullptr,nullptr,nullptr);put(out,uint32_t(ok));std::vector<SourceMaterialPatchNodeCandidate>roots;void*current=root;while(current){roots.push_back(native.extract(current));std::memcpy(&current,static_cast<uint8_t*>(current)+20,4);}put(out,uint32_t(roots.size()));for(const auto&n:roots)writeNode(out,n);native.destroy(root);if(data!=before)throw std::runtime_error("native source buffer changed");for(size_t j=0;j<16;++j)if(record[j]!=guard[j]||record[record.size()-1-j]!=guard[guard.size()-1-j])throw std::runtime_error("native CUtlBuffer guard");
#else
  (void)name;const auto before=source;const auto parsed=sourceMaterialKeyValuesParseCandidate(source);put(out,uint32_t(1));put(out,uint32_t(parsed.roots.size()));for(const auto&n:parsed.roots)writeNode(out,n);if(source!=before)throw std::runtime_error("owned source changed");
#endif
 }
 if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("fixture trailing bytes");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_MATERIAL_KEYVALUES_PARSER
 native.verify();
#endif
 std::cout<<"PASS raw KeyValues parser cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
