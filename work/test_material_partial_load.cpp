#include "material_fallback_parameters_candidate.h"
#include <map>
#include <sstream>
#include <iostream>

int main(){try{
    static_assert(sizeof(void*)==8,"Owned partial load is x64");
    unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Partial load check "+std::to_string(checks));++checks;};
    std::map<std::string,std::string>files{{"materials/root.vmt","patch{include materials/absent.vmt insert{marker 7}}"}};
    auto provider=[&](const std::string&p)->std::optional<std::string>{auto i=files.find(p);return i==files.end()?std::nullopt:std::optional<std::string>(i->second);};
    const auto before=files;
    bool strictRefused=false;try{(void)sourceMaterialPatchFilesCandidate("root",provider);}catch(const std::runtime_error&){strictRefused=true;}
    check(strictRefused);
    const auto partial=sourceMaterialPatchFilesCandidate("root",provider,false,true);
    check(partial.status==SourceMaterialPatchFileLoadStatusCandidate::Loaded&&partial.diagnosticCalls==1&&partial.document.roots.front().key=="patch");
    check(sourceMaterialPatchFindConstCandidate(partial.aggregate,"insert/marker")->bits==7&&files==before);
    for(const auto&s:std::vector<std::string>{"S{a","S{a 1","S{g{a 1"}){
        strictRefused=false;try{(void)sourceMaterialKeyValuesParseCandidate(s);}catch(const std::runtime_error&){strictRefused=true;}
        check(strictRefused);const auto recovered=sourceMaterialKeyValuesParseCandidate(s,false,true);check(recovered.diagnosticCalls==(s=="S{g{a 1"?8u:3u));
    }
    files["materials/empty.vmt"]="";
    check(sourceMaterialPatchFilesCandidate("empty",provider,false,true).status==SourceMaterialPatchFileLoadStatusCandidate::RootUnavailable);
    check(sourceMaterialPatchFilesCandidate("absent",provider,false,true).status==SourceMaterialPatchFileLoadStatusCandidate::RootUnavailable);
    // Unsupported grammar/unsafe paths stay distinct from recoverable IO/syntax.
    for(const auto&s:std::vector<std::string>{"S{a = 1}","S{a \"unterminated}","patch{include ../../escape}","#include ../../escape S{}"}){
        files["materials/root.vmt"]=s;strictRefused=false;try{(void)sourceMaterialPatchFilesCandidate("root",provider,false,true);}catch(const std::runtime_error&){strictRefused=true;}check(strictRefused);
    }
    std::cout<<"PASS partial_load_checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
