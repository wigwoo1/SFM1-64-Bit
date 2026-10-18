from pathlib import Path
p=Path('work/test_dmx_editor_timing.cpp');s=p.read_text().replace('        std::cout<<"session="','        validateGraph(g);\n        std::cout<<"session="').replace('missing_settings_fallback=417 passed','numeric_accessor=int30/time60/bool1/unsupported0 missing_rate_fallback=417 restored_graph_valid passed');p.write_text(s)
