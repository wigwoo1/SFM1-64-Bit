from pathlib import Path
p=Path('work/test_film_scene_render_binding_x64.cpp')
p.write_text(p.read_text().replace('first.models.end(),2552)', 'first.models.end(),uint32_t(2552))'))
