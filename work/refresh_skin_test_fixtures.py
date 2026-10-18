from pathlib import Path
for name in ['test_model_visibility.cpp','verify_hidden_model_render.py','verify_live_skin_selection.py']:
 p=Path('work',name);s=p.read_text().replace('hidden-model-session-v1.dmx','hidden-model-session-v2.dmx').replace('invalid-skin-session-v1.dmx','invalid-skin-session-v2.dmx');p.write_text(s)
