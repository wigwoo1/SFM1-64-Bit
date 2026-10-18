from pathlib import Path
p=Path('work/mounted_model_material_candidate.h');s=p.read_text()
a='auto textures=vmtValuesCandidate(result.syntax,"$basetexture");if(textures.size()!=1)'
b='auto textures=vmtValuesCandidate(result.syntax,"$basetexture");if(textures.empty()&&vmtAsciiLowerCandidate(result.syntax.key)=="vertexlitgeneric"){if(!vmtValuesCandidate(result.syntax,"$envmap").empty())throw std::runtime_error("untextured envmap shader unfinished: "+result.vmt.string());return result;}if(textures.size()!=1)'
assert s.count(a)==1;s=s.replace(a,b);p.write_text(s)
p=Path('work/model_render_inputs_candidate.h');s=p.read_text();s=s.replace('#include "material_blend_candidate.h"','#include "material_blend_candidate.h"\n#include "material_constant_color_candidate.h"')
a='std::ifstream file(source.baseTexture,std::ios::binary);';b='VtfRgbaImageCandidate image;if(source.baseTexture.empty()){auto color=materialConstantColorCandidate(source.syntax);image={1,1,{color.begin(),color.end()}};}else{std::ifstream file(source.baseTexture,std::ios::binary);';assert s.count(a)==1;s=s.replace(a,b)
a='auto image=decodeVtfMipCandidate(bytes,selectedMip);if(result.materials.size()';b='image=decodeVtfMipCandidate(bytes,selectedMip);}if(result.materials.size()';assert s.count(a)==1;s=s.replace(a,b);p.write_text(s)
