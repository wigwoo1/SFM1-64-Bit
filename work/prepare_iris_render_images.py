from pathlib import Path
p=Path('work/model_render_inputs_candidate.h');s=p.read_text().replace('bool translucent=false;};','bool translucent=false;VtfRgbaImageCandidate irisImage;};')
old='result.materials.push_back({std::move(source),std::move(image),translucent});'
new='''VtfRgbaImageCandidate irisImage;if(!source.irisTexture.empty()){std::ifstream irisFile(source.irisTexture,std::ios::binary);if(!irisFile)throw std::runtime_error("iris texture read failed");std::vector<uint8_t>irisBytes((std::istreambuf_iterator<char>(irisFile)),{});auto irisLayout=readVtfTextureLayoutCandidate(irisBytes);auto irisMip=mip<uint32_t(irisLayout.mips.size())?mip:uint32_t(irisLayout.mips.size()-1);irisImage=decodeVtfMipCandidate(irisBytes,irisMip);}result.materials.push_back({std::move(source),std::move(image),translucent,std::move(irisImage)});'''
assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
