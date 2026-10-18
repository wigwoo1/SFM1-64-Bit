from pathlib import Path
p=Path('work/mounted_model_material_candidate.h');s=p.read_text().replace('vmt,baseTexture;', 'vmt,baseTexture,irisTexture;')
old='if(result.baseTexture.empty())throw std::runtime_error("base texture missing");return result;'
new='''if(result.baseTexture.empty())throw std::runtime_error("base texture missing");auto iris=vmtValuesCandidate(result.syntax,"$iris");if(iris.size()>1)throw std::runtime_error("duplicate iris texture");if(!iris.empty()){result.irisTexture=findMountedMaterialCandidate(mounts,materialRelativePathCandidate(iris.front(),".vtf"));if(result.irisTexture.empty())throw std::runtime_error("iris texture missing");}return result;'''
assert s.count(old)==1;s=s.replace(old,new);p.write_text(s)
p=Path('work/inspect_alternate_render_inputs.cpp');s=p.read_text().replace('++cases;}for(size_t group', 'for(auto&material:inputs.materials)if(!material.source.irisTexture.empty())std::cout<<"iris_resource="<<material.source.irisTexture.string()<<" shader="<<material.source.syntax.key<<" native_eye_shader=unfinished\\n";++cases;}for(size_t group');p.write_text(s)
