#pragma once
#include "model_render_inputs_candidate.h"
#include <memory>
#include <cmath>
#include <algorithm>
inline std::shared_ptr<const ModelRenderInputsCandidate>createFurSolidMaterialCandidate(const std::array<uint8_t,3>&color){
 auto inputs=std::make_shared<ModelRenderInputsCandidate>();ModelRenderMaterialCandidate material;material.source.syntax=parseVmtCandidate("\"UnlitGeneric\" { \"$nocull\" \"1\" }");material.image={1,1,{color[0],color[1],color[2],255}};material.translucent=false;inputs->materials.push_back(std::move(material));return inputs;
}
// Cutout coverage stays in the depth-writing opaque pass; lighting remains unlit.
inline std::shared_ptr<const ModelRenderInputsCandidate>createFurCoverageMaterialCandidate(const std::array<uint8_t,3>&color){
 auto inputs=std::make_shared<ModelRenderInputsCandidate>();ModelRenderMaterialCandidate material;material.source.syntax=parseVmtCandidate("\"UnlitGeneric\" { \"$nocull\" \"1\" }");material.source.syntax.children.push_back({"$alphatest","1",{},false});material.source.syntax.children.push_back({"$alphatestreference","0.5",{},false});material.image.width=64;material.image.height=128;material.image.rgba.resize(64*128*4);material.alphaCutoff=materialAlphaCutoffCandidate(material.source.syntax);
 for(unsigned y=0;y<128;++y)for(unsigned x=0;x<64;++x){float u=(float(x)+.5f)/64.f,v=(float(y)+.5f)/128.f;float halfWidth=.34f*(1.f-.85f*v);float edge=(halfWidth-std::fabs(u-.5f))*64.f;float coverage=std::max(0.f,std::min(1.f,edge+.5f))*std::min(1.f,(1.f-v)*16.f);size_t at=(y*64+x)*4;for(unsigned c=0;c<3;++c)material.image.rgba[at+c]=color[c];material.image.rgba[at+3]=static_cast<uint8_t>(std::lround(coverage*255.f));}
 inputs->materials.push_back(std::move(material));return inputs;
}
