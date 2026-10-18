#include "source_material_source_provider_candidate.h"
#include <iostream>
template<class T>static T read(std::istream&in){T v;if(!in.read(reinterpret_cast<char*>(&v),sizeof(v)))throw std::runtime_error("parser fixture truncated");return v;}
static std::string text(std::istream&in){auto n=read<uint32_t>(in);if(n>(1u<<24))throw std::runtime_error("source extent");std::string s(n,'\0');if(!in.read(s.data(),n))throw std::runtime_error("source truncated");return s;}
template<class T>static void put(std::ostream&out,T v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));}
#ifdef MATERIAL_SOURCE_RESOLVED
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
#endif
#ifdef MATERIAL_SOURCE_RESOLVED
static void writeNode(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);if(n.kind==1)putText(out,n.text);else if(n.kind==2||n.kind==3)put(out,n.bits);else if(n.kind==4||n.kind==7)put(out,n.pointer);put(out,uint32_t(n.children.size()));for(const auto&child:n.children)writeNode(out,child);}
#endif


int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"owned source provider x64");if(argc!=3)throw std::runtime_error("mount input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("fixture streams");auto count=read<uint32_t>(in);if(!count||count>128)throw std::runtime_error("mount count");std::vector<SourceMaterialSourceMountCandidate>mounts;for(uint32_t i=0;i<count;++i){auto path=text(in),id=text(in);auto position=read<uint32_t>(in);if(position>1)throw std::runtime_error("mount position");SourceMaterialSourceMountCandidate mount{std::filesystem::path(path),id};if(position)mounts.push_back(std::move(mount));else mounts.insert(mounts.begin(),std::move(mount));}SourceMaterialSourceProviderCandidate source(mounts);auto cases=read<uint32_t>(in);put(out,cases);for(uint32_t i=0;i<cases;++i){auto path=text(in),id=text(in);
#ifdef MATERIAL_SOURCE_RESOLVED
 auto parsed=sourceMaterialKeyValuesFilesCandidate(path,source.materialSource(id));put(out,uint32_t(1));put(out,uint32_t(parsed.document.roots.size()));for(const auto&root:parsed.document.roots)writeNode(out,root);
#else
 auto file=source.read(path,id);put(out,uint32_t(bool(file)));put(out,uint32_t(file?file->bytes.size():0));if(file&&!file->bytes.empty())out.write(reinterpret_cast<const char*>(file->bytes.data()),std::streamsize(file->bytes.size()));
#endif
 }if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("fixture trailing bytes");out.flush();if(!out)throw std::runtime_error("output write");std::cout<<"PASS owned source requests="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
