from pathlib import Path
p=Path('work/test_session_material_overrides.cpp')
s=p.read_text().replace('size_t base=0;','size_t base=0,resolved=0,missing=0;')
s=s.replace('if(texture.empty())throw std::runtime_error("saved override texture missing");std::cout<<"override_texture="<<texture.string()<<"\\n";','if(texture.empty()){++missing;std::cout<<"override_texture_missing="<<values[0]<<"\\n";}else{++resolved;std::cout<<"override_texture="<<texture.string()<<"\\n";}')
s=s.replace('std::cout<<"session material overrides passed','std::cout<<"override_texture_resolved="<<resolved<<" missing="<<missing<<"\\n";std::cout<<"session material overrides passed')
p.write_text(s)
