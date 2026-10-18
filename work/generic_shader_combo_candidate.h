#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
enum class GenericPs20bStaticFieldCandidate : size_t {
    DetailTexture=0,Cubemap=1,DiffuseLighting=2,EnvmapMask=3,
    BaseAlphaEnvmapMask=4,SelfIllum=5,VertexColor=6,Flashlight=7,
    SelfIllumEnvmapMaskAlpha=8,DetailBlendMode=9,SeamlessBase=10,
    SeamlessDetail=11,DistanceAlpha=12,DistanceAlphaFromDetail=13,
    SoftMask=14,Outline=15,OuterGlow=16,FlashlightDepthFilterMode=17,
    ShaderSrgbRead=18,DesaturateWithBaseAlpha=19,LightingPreview=20,
    EnvmapFresnel=21,SrgbInputAdapter=22,ImplicitMaterialFlag=23
};
// Inputs are the two separately observed hardware query results. Names of the
// implicit fields are not yet recovered; materialFlags is params[0] byte8.
inline std::array<uint32_t,13> genericVs20StaticInitialCandidate(
    int32_t firstLevel,uint32_t materialFlags,int32_t secondLevel) {
    std::array<uint32_t,13> fields{};
    fields[11]=firstLevel>=92 && (materialFlags&0x80000000u)==0;
    fields[12]=secondLevel<92;
    return fields;
}
inline std::array<uint32_t,6> genericVs20DynamicInitialCandidate(int32_t query) {
    std::array<uint32_t,6> fields{};fields[5]=query==2;return fields;
}
// Scoped to the observed non-bump VS20 branch. Caller supplies recovered model
// and hardware state; this does not choose VS20 versus VS30 or other branches.
struct GenericVs20DynamicInputsCandidate {
    uint32_t vertexCompression;
    int32_t lightCount;
    uint8_t lightStateFlag;
    int32_t boneCount;
    uint8_t staticControlFlow;
    int32_t constructorQuery;
    // Public SDK comparison suggests scene fog / DOWATERFOG; installed semantic
    // mapping remains an inference, so preserve the raw query interface here.
};
inline bool genericHasDynamicLightCandidate(int32_t count,uint8_t flag) {
    return flag!=0 || count>0;
}
inline std::array<uint32_t,6> genericVs20DynamicFieldsCandidate(
    const GenericVs20DynamicInputsCandidate& input) {
    auto fields=genericVs20DynamicInitialCandidate(input.constructorQuery);
    fields[0]=input.vertexCompression;
    fields[1]=genericHasDynamicLightCandidate(input.lightCount,input.lightStateFlag);
    fields[2]=input.boneCount>0;
    fields[3]=0;
    fields[4]=input.staticControlFlow?0:static_cast<uint32_t>(input.lightCount);
    return fields;
}
// Raw arithmetic parity only: caller still owns legal domains and field meanings.
inline uint32_t genericVs20StaticComboCandidate(const std::array<uint32_t,13>& f) {
    uint32_t x=f[11]+2u*f[12];x=f[10]+2u*x;x=3u*x+f[9];
    x=f[8]+4u*x;x+=f[7];
    for(int i=6;i>=0;--i)x=f[std::size_t(i)]+2u*x;
    return x*48u;
}
inline uint32_t genericVs20DynamicComboCandidate(const std::array<uint32_t,6>& f) {
    uint32_t x=3u*f[5]+f[4]+f[3];
    for(int i=2;i>=0;--i)x=f[std::size_t(i)]+2u*x;
    return x;
}
// Scoped owned admission policy for the observed non-bump branch. The raw
// native packer accepts arbitrary words; this guard prevents radix collisions.
// It does not assert how the original renderer ensures these input domains.
inline uint32_t genericVs20DynamicComboCheckedCandidate(const std::array<uint32_t,6>& f) {
    if(f[0]>1||f[1]>1||f[2]>1||f[3]!=0||f[4]>2||f[5]>1)
        throw std::invalid_argument("generic VS20 dynamic field domain");
    return genericVs20DynamicComboCandidate(f);
}
// Recovered inline non-bump pixel selection, scoped to explicit branch inputs.
inline uint32_t genericPs20DynamicComboCandidate(bool writeWater,int32_t query) {
    return uint32_t(writeWater)+2u*uint32_t(query==2);
}
inline uint32_t genericPs20bDynamicComboCandidate(bool shadows,bool writeWater,int32_t query) {
    return uint32_t(shadows)+2u*(uint32_t(writeWater)+2u*uint32_t(query==2));
}
inline uint32_t genericPs20bStaticComboCandidate(const std::array<uint32_t,24>& f) {
    uint32_t x=f[22]+2u*f[23];x=f[21]+2u*x;x=f[20]+4u*x;
    x=f[19]+2u*x;x+=f[18];x=3u*x+f[17];x=f[16]+2u*x;
    for(int i=15;i>=10;--i)x=f[size_t(i)]+2u*x;
    x=f[9]+10u*x;
    for(int i=8;i>=0;--i)x=f[size_t(i)]+2u*x;
    return x*8u;
}
inline std::array<uint32_t,24> genericPs20bStaticInitialCandidate(uint32_t materialFlags) {
    std::array<uint32_t,24> fields{};
    fields[23]=(materialFlags&0x80000000u)==0;
    return fields;
}
