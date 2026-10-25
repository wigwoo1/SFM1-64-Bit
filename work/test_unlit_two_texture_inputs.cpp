#include "model_render_inputs_candidate.h"
#include <iostream>

void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");
 require(argc==3,"usage: test fresh-workspace-test-directory installed-tf-mount");
 const auto directory=std::filesystem::absolute(argv[1]).lexically_normal();
 const auto allowed=(std::filesystem::current_path()/"outputs").lexically_normal();
 auto relative=directory.lexically_relative(allowed);
 require(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!="..","test directory must be under workspace outputs");
 require(std::filesystem::create_directory(directory),"fresh test directory required");
 const std::filesystem::path installed=argv[2];
 SfmModelGeometryCandidate model;model.bodyModelCounts={1};model.materials.names={"pilotlight"};
 model.materials.directories={"models/effects/pyro/"};model.materials.skins={{0}};
 model.meshes.push_back({0,0,0,0,0,0,{}});
 auto native=prepareModelRenderInputsCandidate(model,0,{installed},0);
 const auto&material=native.materials.at(0);
 require(material.unlitTwoTexture&&material.additive&&!material.translucent&&!material.vertexTransition,"native family/blend classification");
 require(material.image.width==64&&material.image.height==64&&material.secondBaseImage.width==128&&material.secondBaseImage.height==128,"native two-texture decode dimensions");
 require(material.source.baseTexture2.filename()=="pilotlight_motion.vtf","native $texture2 resolution");
 require(material.baseTextureFlags==0x40u&&material.secondTextureFlags==0x40u,"installed VTF flags must survive decoding");
 require(material.baseTextureTransform==identityMaterialUvTransformCandidate()&&material.secondTextureTransform==identityMaterialUvTransformCandidate(),"authored proxy evaluation must not be invented");
 auto overrideValues=[](const std::filesystem::path&,VmtNodeCandidate&syntax){
  syntax.children.push_back({"$basetexturetransform","[1 0 0 0.003242 0 1 0 -0.015513 0 0 1 0 0 0 0 1]",{},false});
  syntax.children.push_back({"$texture2transform","[1 0 0 0.719841 0 1 0 0.177722 0 0 1 0 0 0 0 1]",{},false});
 };
 auto saved=prepareModelRenderInputsCandidate(model,0,{installed},2,{},overrideValues);
 const auto&value=saved.materials.at(0);
 require(value.image.width==16&&value.secondBaseImage.width==32,"independent second mip decode");
 require(value.baseTextureFlags==material.baseTextureFlags&&value.secondTextureFlags==material.secondTextureFlags,"VTF flags must not depend on selected mip");
 require(std::abs(value.baseTextureTransform[2]-.003242f)<1e-7f&&std::abs(value.secondTextureTransform[2]-.719841f)<1e-7f&&std::abs(value.secondTextureTransform[6]-.177722f)<1e-7f,"saved transform independence");
 auto vmtPath=directory/"materials/models/effects/pyro/pilotlight.vmt";
 std::filesystem::create_directories(vmtPath.parent_path());
 auto write=[&](const std::string&text){std::ofstream out(vmtPath);out<<text;out.close();require(bool(out),"fixture write failed");};
 write("WorldVertexTransition {$basetexture models/effects/pyro/pilotlight $basetexture2 models/effects/pyro/pilotlight_motion $texture2 missing/decoy}");
 auto world=prepareModelRenderInputsCandidate(model,0,{directory,installed},2);
 require(world.materials[0].vertexTransition&&!world.materials[0].unlitTwoTexture&&!world.materials[0].additive&&world.materials[0].secondBaseImage.width==32,"WorldVertexTransition must retain $basetexture2 behavior");
 write("UnlitTwoTexture {$basetexture models/effects/pyro/pilotlight $texture2 missing/owned-test}");
 auto missing=resolveMountedModelMaterialCandidate(model.materials,0,0,{directory,installed});
 require(missing.baseTexture2Missing&&missing.missingTextureReferences==std::vector<std::string>{"$texture2 missing/owned-test"},"missing second texture diagnostic");
 size_t rejected=0;
 for(auto text:{"UnlitTwoTexture {$basetexture models/effects/pyro/pilotlight $texture2 a $texture2 b}","UnlitTwoTexture {$basetexture models/effects/pyro/pilotlight $texture2 ../outside}"}){
  write(text);try{(void)resolveMountedModelMaterialCandidate(model.materials,0,0,{directory,installed});}catch(const std::runtime_error&){++rejected;}
 }
 for(auto text:{"UnlitTwoTexture {$texture2transform bad}","UnlitTwoTexture {$texture2transform identity $texture2transform identity}"}){
  try{(void)materialUvTransformCandidate(parseVmtCandidate(text),"$texture2transform");}catch(const std::exception&){++rejected;}
 }
 for(auto text:{"UnlitTwoTexture {$additive 2}","UnlitTwoTexture {$additive 0 $additive 1}"}){
  try{(void)materialAdditiveCandidate(parseVmtCandidate(text));}catch(const std::runtime_error&){++rejected;}
 }
 require(rejected==6,"invalid second texture/transform/additive checks");
 // Owned 1x1 VTFs prove the full resolver retains both independent flag words,
 // including alpha flags even when decoded alpha bytes happen to be opaque.
 auto texture=[&](const char*name,uint32_t flags){
  std::vector<uint8_t>bytes(92);auto word=[&](size_t n,uint32_t v){std::memcpy(bytes.data()+n,&v,4);};auto half=[&](size_t n,uint16_t v){std::memcpy(bytes.data()+n,&v,2);};
  word(0,0x00465456);word(4,7);word(8,3);word(12,88);half(16,1);half(18,1);word(20,flags);half(24,1);half(63,1);bytes[56]=1;
  word(68,1);word(80,0x30);word(84,88);
  bytes[88]=64;bytes[89]=128;bytes[90]=192;bytes[91]=255;
  std::ofstream stream(directory/(std::string("materials/")+name+".vtf"),std::ios::binary);stream.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());stream.close();require(bool(stream),"owned VTF write failed");
 };
 write("UnlitTwoTexture {$basetexture owned-base $texture2 owned-second $additive 1}");
 texture("owned-base",0x1040);texture("owned-second",0x2040);
 auto flagged=prepareModelRenderInputsCandidate(model,0,{directory},0);
 require(flagged.materials[0].baseTextureFlags==0x1040u&&flagged.materials[0].secondTextureFlags==0x2040u&&flagged.materials[0].image.rgba[3]==255,"independent raw alpha flags lost during preparation");
 texture("owned-base",0);texture("owned-second",0x40);
 auto opaque=prepareModelRenderInputsCandidate(model,0,{directory},0);
 require(opaque.materials[0].baseTextureFlags==0u&&opaque.materials[0].secondTextureFlags==0x40u,"known zero flags must differ from unknown flags");
 write("UnlitTwoTexture {$basetexture missing/owned-test $texture2 owned-second $additive 1}");
 auto fallback=prepareModelRenderInputsCandidate(model,0,{directory},0);
 require(!fallback.materials[0].baseTextureFlags&&fallback.materials[0].secondTextureFlags==0x40u,"fallback must not invent original texture flags");
 std::cout<<"PASS: native two-texture data, saved transforms/mips, WorldVertexTransition isolation, missing diagnostic, six invalid inputs rejected, independent VTF flags and unknown fallback preserved\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
