#pragma once
// Requires the owned DMX Graph definitions. These are saved values, not evaluated
// timeline values, native object offsets, or flashlight renderer constants.
inline constexpr std::array<const char*,31> projectedLightSavedFloatNamesCandidate{{"minDistance","horizontalFOV","verticalFOV","ambientIntensity","radius","shadowDepthBias","shadowSlopeScaleDepthBias","shadowFilterSize","shadowAtten","jitterSeed","frameRate","farZAtten","ambientOcclusion","nearEdge","farEdge","cutOn","cutOff","width","edgeWidth","height","edgeHeight","roundness","volumetricIntensity","noiseStrength","flashlightTime","planeOffset","intensity","constantAttenuation","linearAttenuation","quadraticAttenuation","maxDistance"}};
struct ProjectedLightSavedFieldsCandidate {
    std::array<uint32_t,31> floatWords{}; // Order follows projectedLightSavedFloatNamesCandidate.
    uint32_t colorWord{}, animationTimeWord{}, numPlanesWord{};
    std::array<uint32_t,2> positionJitterWords{};
    std::array<uint8_t,4> booleanBytes{}; // castsShadows, drawShadowFrustum, uberlight, volumetric.
    std::string textureName;
    uint32_t transformReference{};
    std::array<char,16> transformId{};
    std::array<uint32_t,3> localPositionWords{};
    std::array<uint32_t,4> localOrientationWords{};
    bool hasLocalScale=false;
    uint32_t localScaleWord=0x3f800000;
};
inline ProjectedLightSavedFieldsCandidate projectedLightSavedFieldsCandidate(const Graph& graph,size_t index){
    const auto& element=graph.elements.at(index);
    if(graph.strings.at(element.type)!="DmeProjectedLight")throw std::invalid_argument("projected light element type");
    const auto& names=projectedLightSavedFloatNamesCandidate;
    ProjectedLightSavedFieldsCandidate result;
    const auto require=[&](const char* name,unsigned type,size_t bytes)->const Value&{
        const Attribute* found=nullptr;
        for(const auto& attribute:element.attributes)if(graph.strings.at(attribute.name)==name){
            if(found)throw std::invalid_argument("duplicate projected light field");found=&attribute;
        }
        if(!found||found->type!=type||found->values.size()!=1||found->values[0].wire.size()!=bytes)throw std::invalid_argument("projected light field shape");
        return found->values[0];
    };
    for(unsigned field=0;field<names.size();++field){
        result.floatWords[field]=wireU32(require(names[field],3,4));
    }
    result.colorWord=wireU32(require("color",8,4));
    result.animationTimeWord=wireU32(require("animationTime",7,4));
    result.numPlanesWord=wireU32(require("numPlanes",2,4));
    const auto& jitter=require("positionJitter",9,8);
    for(unsigned component=0;component<2;++component)std::memcpy(&result.positionJitterWords[component],jitter.wire.data()+component*4,4);
    const char* booleanNames[]{"castsShadows","drawShadowFrustum","uberlight","volumetric"};
    for(unsigned field=0;field<4;++field)result.booleanBytes[field]=static_cast<uint8_t>(require(booleanNames[field],4,1).wire[0]);
    const auto& texture=require("texture",5,0);
    if(texture.stringIndex>=graph.strings.size()||graph.strings[texture.stringIndex]!=texture.text)throw std::invalid_argument("projected light texture string");
    result.textureName=texture.text;
    result.transformReference=require("transform",1,0).reference;
    if(result.transformReference>=graph.elements.size())throw std::invalid_argument("projected light transform reference");
    const auto& transform=graph.elements[result.transformReference];
    if(graph.strings.at(transform.type)!="DmeTransform")throw std::invalid_argument("projected light transform type");
    result.transformId=transform.id;
    const auto readTransform=[&](const char* name,unsigned type,void* destination,size_t bytes){
        const Attribute* found=nullptr;
        for(const auto& attribute:transform.attributes)if(graph.strings.at(attribute.name)==name){
            if(found)throw std::invalid_argument("duplicate transform field");found=&attribute;
        }
        if(!found||found->type!=type||found->values.size()!=1||found->values[0].wire.size()!=bytes)throw std::invalid_argument("transform field shape");
        std::memcpy(destination,found->values[0].wire.data(),bytes);
    };
    readTransform("position",10,result.localPositionWords.data(),12);
    readTransform("orientation",13,result.localOrientationWords.data(),16);
    for(const auto& attribute:transform.attributes)if(graph.strings.at(attribute.name)=="scale"){
        if(result.hasLocalScale)throw std::invalid_argument("duplicate transform scale");
        if(attribute.type!=3||attribute.values.size()!=1||attribute.values[0].wire.size()!=4)throw std::invalid_argument("transform scale shape");
        result.hasLocalScale=true;result.localScaleWord=wireU32(attribute.values[0]);
    }
    return result;
}
