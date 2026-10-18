#include "vcs6_shader_bytecode_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <d3dcompiler.h>
#include <array>
#include <thread>
#include <sstream>
#include "d3d9_shader_constants_candidate.h"
static std::vector<uint8_t> read(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("file open");return {(std::istreambuf_iterator<char>(file)),{}};}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("manifest required");
 std::ifstream manifest(argv[1]);std::string cachePath,oraclePath;unsigned checked=0,rejected=0;
 unsigned constantChecks=0,ctabRejections=0;
 std::map<std::string,std::unique_ptr<RetainedVcs6ShaderCacheCandidate>> caches;
 std::shared_ptr<const std::vector<uint8_t>> survivingProgram;std::vector<uint8_t> survivingReference;
 while(std::getline(manifest,cachePath)){
  uint32_t argument,dynamic;manifest>>argument>>dynamic;manifest.ignore();std::getline(manifest,oraclePath);
  auto data=read(cachePath);
  if(oraclePath=="REJECT") {bool failed=false;try{vcs6ShaderBytecodeCandidate(data,argument,dynamic);}catch(const std::runtime_error&){failed=true;}
   if(!failed)throw std::runtime_error("invalid cache accepted");++rejected;
  }else{auto shader=vcs6ShaderBytecodeCandidate(data,argument,dynamic);if(shader!=read(oraclePath))throw std::runtime_error("decoded bytecode oracle mismatch");
   auto& cache=caches[cachePath];if(!cache){cache=std::make_unique<RetainedVcs6ShaderCacheCandidate>(data);
    std::array<std::thread,8> threads;std::array<std::shared_ptr<const std::vector<uint8_t>>,8> programs;
    std::array<std::exception_ptr,8> errors{};
    for(size_t i=0;i<threads.size();++i)threads[i]=std::thread([&,i]{try{programs[i]=cache->shader(argument,dynamic);}catch(...){errors[i]=std::current_exception();}});
    for(auto& thread:threads)thread.join();
    for(size_t i=0;i<programs.size();++i){if(errors[i])std::rethrow_exception(errors[i]);if(programs[i].get()!=programs[0].get()||*programs[i]!=shader)throw std::runtime_error("concurrent cold lookup mismatch");}
    if(cache->decodedBlocks()!=1)throw std::runtime_error("concurrent duplicate decode");
    RetainedVcs6ShaderCacheCandidate limited(data,0);bool budgetRejected=false;
    try{limited.shader(argument,dynamic);}catch(const std::runtime_error&){budgetRejected=true;}
    if(!budgetRejected||limited.decodedBlocks()!=0||limited.retainedBytes()!=0)throw std::runtime_error("budget failure published cache state");
   }
   auto retained=cache->shader(argument,dynamic);auto repeat=cache->shader(argument,dynamic);
   if(*retained!=shader||retained.get()!=repeat.get())throw std::runtime_error("retained cache mismatch");
   survivingProgram=retained;survivingReference=shader;
   ID3DBlob* assembly=nullptr;auto result=D3DDisassemble(shader.data(),shader.size(),0,nullptr,&assembly);
   if(FAILED(result)||!assembly){if(assembly)assembly->Release();throw std::runtime_error("D3DDisassemble rejected decoded program");}
   std::string assemblyText(static_cast<const char*>(assembly->GetBufferPointer()),assembly->GetBufferSize());assembly->Release();
   auto constants=d3d9ShaderConstantsCandidate(shader);
   if(!constants.empty()&&!ctabRejections){uint32_t table=uint32_t(shader[28])|(uint32_t(shader[29])<<8)|(uint32_t(shader[30])<<16)|(uint32_t(shader[31])<<24);
    for(size_t offset:{size_t(12),size_t(20),size_t(24),size_t(28),size_t(12)+table,size_t(24)+table}){
     auto bad=shader;for(size_t byte=0;byte<4;++byte)bad[offset+byte]=0xff;
     bool failed=false;try{d3d9ShaderConstantsCandidate(bad);}catch(const std::runtime_error&){failed=true;}
     if(!failed)throw std::runtime_error("malformed CTAB accepted");++ctabRejections;}}
   for(const auto& constant:constants){std::istringstream rows(assemblyText);std::string row;bool found=false;
    while(std::getline(rows,row)){std::istringstream fields(row);std::string comment,name,reg;unsigned count;
     if(fields>>comment>>name>>reg>>count){const char prefixes[]={'b','i','c','s'};
      if(comment=="//"&&name==constant.name&&reg==std::string(1,prefixes[constant.registerSet])+std::to_string(constant.index)&&count==constant.count)found=true;}}
    if(!found)throw std::runtime_error("CTAB register mismatch against D3DDisassemble");++constantChecks;}
   ++checked;}
 }
 if(!manifest.eof()||!checked)throw std::runtime_error("manifest parse");
 size_t blocks=0,bytes=0;for(auto& pair:caches){blocks+=pair.second->decodedBlocks();bytes+=pair.second->retainedBytes();}
 caches.clear();if(!survivingProgram||*survivingProgram!=survivingReference)throw std::runtime_error("program lifetime after cache destruction");
 std::cout<<"x64_bytecode_exact="<<checked<<" D3DDisassemble_accepted="<<checked<<" CTAB_register_checks="<<constantChecks<<" CTAB_invalid_rejected="<<ctabRejections<<" rejected="<<rejected<<" retained_blocks="<<blocks<<" retained_bytes="<<bytes<<" program_lifetime=passed concurrent_cold_lookup=passed budget_atomicity=passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
