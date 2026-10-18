from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
old='if(modelVisible&&changed<1000)throw std::runtime_error("draw produced no substantial geometry");'
new='if(!frozenScene&&modelVisible&&changed<1000)throw std::runtime_error("draw produced no substantial geometry");'
assert old in s;s=s.replace(old,new,1);p.write_text(s)
