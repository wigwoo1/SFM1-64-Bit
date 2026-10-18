from pathlib import Path
p=Path('work/mounted_model_material_candidate.h');s=p.read_text()
s=s.replace('throw std::runtime_error("exactly one base texture required");','throw std::runtime_error("exactly one base texture required: "+result.vmt.string()+" shader="+result.syntax.key+" found="+std::to_string(textures.size()));')
s=s.replace('throw std::runtime_error("base texture missing");','throw std::runtime_error("base texture missing: "+textures[0]+" vmt="+result.vmt.string());')
s=s.replace('throw std::runtime_error("iris texture missing");','throw std::runtime_error("iris texture missing: "+iris.front()+" vmt="+result.vmt.string());')
p.write_text(s)
