#include "source_bsp_base_lightmap_candidate.h"
#include <iostream>
#include <sstream>
static unsigned checks=0,positives=0,refusals=0;
static void need(bool ok){if(!ok)throw std::runtime_error("base lightmap metadata contract");++checks;}
static std::string fingerprint(const VmtNodeCandidate& root){
    std::ostringstream out;auto append=[&](auto&& self,const VmtNodeCandidate& n)->void{out<<n.key.size()<<':'<<n.key<<n.value.size()<<':'<<n.value<<':'<<n.block<<':'<<n.children.size();for(const auto& c:n.children)self(self,c);};append(append,root);return out.str();
}
int main(){try{
    static_assert(sizeof(void*)==8,"owned x64 metadata test");
    for(const auto* text:{
        "LightmappedGeneric { $basetexture owned/base }",
        "LightmappedGeneric { $basetexture owned/base %keywords \"tf, owned labels\" }",
        "LightmappedGeneric { $surfaceprop metal $basetexture owned/base }",
        "LightmappedGeneric { %KEYWORDS \"tf, Mixed CASE\" $baseTexture owned/base $SURFACEPROP unresolved_owned_label }",
        "LightmappedGeneric { $basetexture owned/base $basetexturetransform \"center 0 0 scale .7 .6 translate .1 .2\" %keywords tf $surfaceprop metal }"}){
        const auto material=parseVmtCandidate(text);const auto before=fingerprint(material);validateSourceBaseLightmapMaterialCandidate(material);need(fingerprint(material)==before);++positives;
        for(const auto& n:material.children)if(sourceBaseLightmapMetadataKeyCandidate(n.key))need(sourceBaseLightmapMetadataLeafCandidate(n));
    }
    auto base=parseVmtCandidate("LightmappedGeneric { $basetexture owned/base }");
    auto reject=[&](const VmtNodeCandidate& material){const auto before=fingerprint(material);bool failed=false;try{validateSourceBaseLightmapMaterialCandidate(material);}catch(const std::runtime_error&){failed=true;}need(failed&&fingerprint(material)==before);++refusals;};
    for(const auto* key:{"%compilesky","%tooltexture","%detailtype","%unknown","$surfaceprop2","$bumpmap","$detail","$ssbump","$alpha","HDR?%keywords","360?$surfaceprop"}){auto m=base;m.children.push_back({key,"1",{},false});reject(m);}
    for(const auto* key:{"%keywords","$surfaceprop"}){
        for(unsigned bad=0;bad<8;++bad){auto m=base;VmtNodeCandidate n{key,"owned",{},false};
            if(bad==0)n.value.clear();if(bad==1)n.block=true;if(bad==2)n.children.push_back({"child","value",{},false});
            if(bad==3)n.value=std::string("a\0b",3);if(bad==4)n.value="a\nb";if(bad==5)n.value=std::string("\x80",1);if(bad==6)n.value=std::string(4096,'x');
            m.children.push_back(n);if(bad==7){n.key=key[0]=='%'?"%KEYWORDS":"$SURFACEPROP";m.children.push_back(n);}reject(m);
        }
        auto m=base;m.children.push_back({key,std::string(4095,'x'),{},false});const auto before=fingerprint(m);validateSourceBaseLightmapMaterialCandidate(m);need(fingerprint(m)==before);++positives;
    }
    for(unsigned bad=0;bad<4;++bad){auto m=base;m.children.push_back({"%keywords","tf",{},false});m.children.push_back({"$surfaceprop","metal",{},false});
        if(bad==0)m.key="VertexLitGeneric";if(bad==1)m.block=false;if(bad==2)m.children[0].key="$other";if(bad==3)m.children.push_back({"$basetexture","second",{},false});reject(m);
    }
    need(!sourceBaseLightmapMetadataKeyCandidate("%compileSky")&&!sourceBaseLightmapMetadataKeyCandidate("$surfaceprop2"));
    std::cout<<"PASS metadata_checks="<<checks<<" admitted_cases="<<positives<<" refusals="<<refusals<<" authored_nodes_preserved=1 original_runtime=ABSENT\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
