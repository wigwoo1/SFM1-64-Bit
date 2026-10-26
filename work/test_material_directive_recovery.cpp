#define main acceptedFallbackProbeMain
#include "probe_material_fallback_owned.cpp"
#undef main

int main(){try{
    unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Directive recovery check "+std::to_string(checks));++checks;};
    using Node=SourceMaterialPatchNodeCandidate;
    std::map<std::string,std::string>files{{"materials/root.vmt","#include absent.vmt VertexLitGeneric{$alpha .375}"},{"materials/base.vmt","Template{$color \"[.2 .4 .6]\"}"}};
    unsigned reads=0;auto provider=[&](const std::string&p)->std::optional<std::string>{++reads;auto i=files.find(p);return i==files.end()?std::nullopt:std::optional<std::string>(i->second);};
    Node source;source.key="Wireframe";source.children.push_back(Node{"$fallbackmaterial",1,"root"});Node error;error.key="Wireframe_DX9";error.children.push_back(Node{"errorMarker",1,"yes"});
    auto initial=files;MaterialFallbackParametersCandidate value;
    check(materialFallbackParametersCandidate(value,source,error,"owner",false,provider));
    check(value.material.developerDiagnosticCalls==1&&value.material.parameters.warnings==0&&value.material.shader=="VertexLitGeneric");
    check(files==initial&&value.material.parameters.values[5].value.numeric.vector[0]==0x3ec00000);
    bool refused=false;try{(void)sourceMaterialKeyValuesFilesCandidate("materials/root.vmt",provider);}catch(const std::runtime_error&){refused=true;}check(refused);
    files["materials/root.vmt"]="#base base.vmt";
    check(materialFallbackParametersCandidate(value,source,error,"owner",false,provider)&&value.root.key=="Wireframe"&&value.material.shader=="Wireframe_DX9"&&value.material.developerDiagnosticCalls==0);
    check(sourceMaterialPatchFindConstCandidate(value.root,"$color")->text=="[.2 .4 .6]");
    files["materials/root.vmt"]="#include absent.vmt";
    check(materialFallbackParametersCandidate(value,source,error,"owner",false,provider)&&value.root.key=="Wireframe"&&value.root.children.empty()&&value.material.developerDiagnosticCalls==1&&value.material.parameters.warnings==0);
    const auto before=snapshot(value);const auto dev=value.material.developerDiagnosticCalls;
    for(const auto&s:std::vector<std::string>{"#include ../../escape S{}","#include root.vmt S{}","#base malformed.vmt VertexLitGeneric{}"}){
        files["materials/root.vmt"]=s;files["materials/malformed.vmt"]="S{v \"unterminated}";auto input=files;
        check(!materialFallbackParametersCandidate(value,source,error,"owner",false,provider)&&snapshot(value)==before&&value.material.developerDiagnosticCalls==dev&&files==input);
    }
    files["materials/root.vmt"].clear();for(unsigned i=0;i<257;++i)files["materials/root.vmt"]+="#base absent"+std::to_string(i)+".vmt ";files["materials/root.vmt"]+="VertexLitGeneric{}";reads=0;
    check(!materialFallbackParametersCandidate(value,source,error,"owner",false,provider)&&snapshot(value)==before&&value.material.developerDiagnosticCalls==dev&&reads<=257);
    std::cout<<"PASS directive_recovery_checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
