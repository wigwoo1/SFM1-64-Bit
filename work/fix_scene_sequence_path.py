from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text();s=s.replace('loadGraph(sceneSessionFiles.at(frame))','loadGraph(sceneSessionFiles.at(frame).c_str())');p.write_text(s)
