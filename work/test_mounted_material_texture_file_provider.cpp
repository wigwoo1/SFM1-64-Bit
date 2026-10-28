#include <cstdlib>
#include <new>
#include <cstdint>
static int64_t failAfter=-1;static size_t heapNew=0,heapDelete=0;
void*operator new(size_t n){if(failAfter>=0&&failAfter--==0)throw std::bad_alloc();auto*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();++heapNew;return p;}
void operator delete(void*p)noexcept{if(p)++heapDelete;std::free(p);}
void operator delete(void*p,size_t)noexcept{::operator delete(p);}
void*operator new[](size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete[](void*p,size_t)noexcept{::operator delete(p);}
#include "mounted_material_texture_file_provider_candidate.h"
#include <fstream>
#include <iostream>
using Mounted=MountedMaterialTextureFileProviderCandidate;
using Files=MaterialTextureFileResolverCandidate;
using Handle=mounted_material_texture_detail::Handle;
static void need(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
static DWORD handles(){DWORD count=0;need(GetProcessHandleCount(GetCurrentProcess(),&count)!=0,"own test-process handle count");return count;}
static std::vector<uint8_t>vtf(unsigned seed){std::vector<uint8_t>b(112,0);auto w=[&](size_t o,uint32_t v){std::memcpy(b.data()+o,&v,4);};auto h=[&](size_t o,uint16_t v){std::memcpy(b.data()+o,&v,2);};w(0,0x00465456);w(4,7);w(8,2);w(12,80);h(16,2);h(18,2);h(24,2);w(52,0);b[56]=1;h(63,1);for(size_t i=80;i<b.size();++i)b[i]=uint8_t(seed+i);return b;}
static void write(const std::filesystem::path&p,const std::vector<uint8_t>&bytes){std::filesystem::create_directories(p.parent_path());std::ofstream out(p,std::ios::binary|std::ios::trunc);need(bool(out),"disposable fixture write");out.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));out.close();need(bool(out),"disposable fixture complete write");}
static MaterialTextureFileRequestCandidate request(const char*name){MaterialTextureFileRequestCandidate r;need(materialTextureFileRequestCandidate(name,r),"fixture logical request");return r;}
static std::filesystem::path fixtureRoot(const char*input){
 std::wstring buffer(32768,L'\0');auto n=GetModuleFileNameW(nullptr,buffer.data(),DWORD(buffer.size()));need(n&&n<buffer.size(),"test executable path");buffer.resize(n);
 auto executable=std::filesystem::canonical(std::filesystem::path(buffer).parent_path());auto path=std::filesystem::absolute(input).lexically_normal();auto parent=std::filesystem::canonical(path.parent_path());auto rel=parent.lexically_relative(executable);
 need(!rel.empty()&&!rel.is_absolute()&&rel.native().find(L'\0')==std::wstring::npos,"fixture parent below isolated executable directory");for(const auto&part:rel)need(part!="..","fixture may not escape isolated build output");
 need(!path.filename().empty()&&!std::filesystem::exists(path),"fixture root must be fresh");need(std::filesystem::create_directory(path),"create dedicated fresh fixture root");return path;
}
static int run(const std::filesystem::path&root){
 static_assert(sizeof(void*)==8,"native owned x64 provider");unsigned checks=0;auto check=[&](bool ok,const char*message){need(ok,message);++checks;};
 const auto upper=root/"upper",lower=root/"lower",outside=root/"upper-evil";auto first=vtf(13),second=vtf(71);const auto relative=std::filesystem::path("materials/fixture/a.vtf");write(upper/relative,first);write(lower/relative,second);write(outside/relative,second);
 auto r=request("fixture/a");std::vector<std::filesystem::path>mounts{upper,lower};auto provider=Mounted(mounts).provider();mounts.clear();
 check(provider(r)==first,"copied mount list and destroyed facade retain priority");
 {auto paths=std::vector<std::filesystem::path>{lower,upper};auto reverse=Mounted(paths).provider();check(reverse(r)==second,"reversed explicit mount priority");}
 auto onlyUpper=request("fixture/only_lower");write(lower/"materials/fixture/only_lower.vtf",second);check(provider(onlyUpper)==second,"absent preferred file falls through");
 for(unsigned mode=0;mode<6;++mode){auto forged=r;if(mode==0)forged.file="../outside.vtf";if(mode==1)forged.pathID="MOD";if(mode==2)forged.mode="wb";if(mode==3)forged.key="FIXTURE/A";if(mode==4)forged.key="../a";if(mode==5)forged.file="C:/outside.vtf";check(!provider(forged),"forged public request refused");}
 check(mountedMaterialTexturePathInsideCandidate(L"\\\\?\\C:\\mount\\file.vtf",L"\\\\?\\C:\\mount")&&!mountedMaterialTexturePathInsideCandidate(L"\\\\?\\C:\\mount-evil\\file.vtf",L"\\\\?\\C:\\mount")&&!mountedMaterialTexturePathInsideCandidate(L"\\\\?\\C:\\mount",L"\\\\?\\C:\\mount")&&mountedMaterialTexturePathInsideCandidate(L"\\\\?\\C:\\file.vtf",L"\\\\?\\C:\\"),"final-path full directory boundary");
 check(mountedMaterialTexturePathInsideCandidate(L"\\Device\\HarddiskVolume8\\mount\\file.vtf",L"\\Device\\HarddiskVolume8\\mount")&&!mountedMaterialTexturePathInsideCandidate(L"\\Device\\HarddiskVolume8\\mount-evil\\file.vtf",L"\\Device\\HarddiskVolume8\\mount")&&!mountedMaterialTexturePathInsideCandidate(L"\\Device\\HarddiskVolume9\\mount\\file.vtf",L"\\Device\\HarddiskVolume8\\mount"),"normalized NT volume and complete directory boundary");
 for(const auto&path:{std::filesystem::path("relative"),std::filesystem::path(L"\\\\server\\share"),std::filesystem::path(L"\\\\?\\GLOBALROOT\\Device\\HarddiskVolume8"),upper/".."/"lower"}){bool refused=false;try{Mounted invalid({path});}catch(const std::invalid_argument&){refused=true;}check(refused,"relative UNC device and non-normalized mounts refused");}
 {Handle locked(CreateFileW((upper/relative).c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));need(bool(locked),"disposable preferred-file lock");check(!provider(r),"locked preferred file does not substitute lower file");}check(provider(r)==first,"selected file read succeeds after fixture lock released");
 auto bad=request("fixture/bad");write(lower/"materials/fixture/bad.vtf",second);
 for(auto bytes:{std::vector<uint8_t>{1,2,3},std::vector<uint8_t>(first.begin(),first.begin()+96),std::vector<uint8_t>{}}){write(upper/"materials/fixture/bad.vtf",bytes);check(provider(bad)==bytes,"provider returns selected malformed/incomplete bytes without substitution");Files files(provider);check(!files.resolve(std::string("fixture/bad"))&&files.cachedCount()==0,"existing resolver rejects malformed/incomplete preferred VTF");}
 auto large=request("fixture/large");auto tooLarge=first;tooLarge.push_back(0);write(upper/"materials/fixture/large.vtf",tooLarge);write(lower/"materials/fixture/large.vtf",second);check(!Mounted({upper,lower},112).provider()(large),"oversized upper file cannot fall through to fitting lower file");
 std::filesystem::create_directories(upper/"materials/fixture/directory.vtf");write(lower/"materials/fixture/directory.vtf",second);check(!provider(request("fixture/directory")),"directory cannot substitute lower regular file");
 {const auto future=root/"future";auto old=Mounted({future,lower}).provider();write(future/relative,first);check(old(r)==second&&Mounted({future,lower}).provider()(r)==first,"absent mount omitted until explicit new profile");}
 {auto moving=root/"moving",renamed=root/"moving-renamed";write(moving/relative,first);auto retained=Mounted({moving,lower}).provider();std::filesystem::rename(moving,renamed);write(moving/relative,second);check(!retained(r),"moved mount identity is not replaced by same-path directory");}
 for(uint64_t cap:{uint64_t(0),(uint64_t(1)<<29)+1}){bool refused=false;try{Mounted invalid({upper},cap);}catch(const std::invalid_argument&){refused=true;}check(refused,"invalid size limit rejected");}
 {bool refused=false;try{Mounted invalid({upper/relative});}catch(const std::runtime_error&){refused=true;}check(refused,"non-directory mount rejected");}
 check(!Mounted({root/"absent"}).provider()(r),"all missing mounts stay unavailable");
 bool symlink=false;DWORD symlinkError=0;auto link=upper/"materials/fixture/escape.vtf";auto target=outside/relative;write(lower/"materials/fixture/escape.vtf",second);
 if(CreateSymbolicLinkW(link.c_str(),target.c_str(),0x2))symlink=true;else{symlinkError=GetLastError();if(symlinkError==ERROR_INVALID_PARAMETER&&CreateSymbolicLinkW(link.c_str(),target.c_str(),0))symlink=true;else symlinkError=GetLastError();}
 if(symlink){check(!provider(request("fixture/escape")),"same opened handle final path refuses escaping file symlink");check(DeleteFileW(link.c_str())!=0,"remove only disposable link itself");}
 {auto cached=request("fixture/cached");write(upper/"materials/fixture/cached.vtf",first);write(lower/"materials/fixture/cached.vtf",second);Files files(provider);auto a=files.resolve(std::string("FIXTURE\\CACHED.VTF"));check(a&&a->resource()->bytes()==first,"real file becomes immutable cached owner");need(std::filesystem::remove(upper/"materials/fixture/cached.vtf"),"remove disposable selected file");auto again=files.resolve(std::string("fixture/cached"));check(again&&*again==*a&&again->resource()->bytes()==first,"cached alias retains copied bytes after source removal");Files fresh(provider);auto b=fresh.resolve(std::string("fixture/cached"));check(b&&b->resource()->bytes()==second,"fresh profile sees absent upper file fallback");}
 unsigned constructionFailures=0,readFailures=0;bool constructed=false,readComplete=false;const std::vector<std::filesystem::path>stableMounts{upper,lower};
 for(int64_t n=0;n<512;++n){const auto balance=heapNew-heapDelete;const auto handlesBefore=handles();bool ok=false;{Files::Provider held;failAfter=n;try{held=Mounted(stableMounts).provider();ok=true;}catch(const std::exception&){}failAfter=-1;if(ok)check(held(r)==first,"construction allocation sweep reaches valid owned provider");}check(heapNew-heapDelete==balance&&handles()==handlesBefore,"construction allocation failure releases handles and memory");if(ok){constructed=true;break;}++constructionFailures;}
 check(constructed&&constructionFailures>0,"actual provider construction allocation failures covered");
 for(int64_t n=0;n<512;++n){const auto balance=heapNew-heapDelete;const auto handlesBefore=handles();bool ok=false;{failAfter=n;auto bytes=provider(r);failAfter=-1;ok=bytes.has_value();check(!ok||*bytes==first,"read allocation outcome preserves complete selected bytes");}check(heapNew-heapDelete==balance&&handles()==handlesBefore,"read allocation failure releases selected file handle and memory");if(ok){readComplete=true;break;}++readFailures;}
 check(readComplete&&readFailures>0&&provider(r)==first,"actual read allocation failures leave provider usable");
 const auto beforeRelease=handles();provider={};check(handles()+2==beforeRelease,"provider releases both retained directory handles");
 std::cout<<"PASS mounted_texture_checks="<<checks<<" construction_allocation_failures="<<constructionFailures<<" read_allocation_failures="<<readFailures<<" symlink_escape="<<(symlink?"PASS":"UNVERIFIED")<<" symlink_error="<<symlinkError<<" priority=PASS handles=PASS read_only=PASS GPU=NOT_RUN\n";return 0;
}
int main(int argc,char**argv){try{need(argc==3,"--fixtures NEW_DIRECTORY or --junction-fixture EXISTING_DIRECTORY required");if(std::string(argv[1])=="--junction-fixture"){const auto root=std::filesystem::absolute(argv[2]);need(std::filesystem::is_directory(root/"upper/materials/junction"),"prepared disposable junction required");auto provider=Mounted({root/"upper",root/"lower"}).provider();need(!provider(request("junction/fixture/a")),"escaping junction must not read target or lower fallback");std::cout<<"PASS mounted_junction_escape=PASS lower_fallback=REFUSED\n";return 0;}need(std::string(argv[1])=="--fixtures","unknown test mode");return run(fixtureRoot(argv[2]));}catch(const std::exception&e){failAfter=-1;std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
