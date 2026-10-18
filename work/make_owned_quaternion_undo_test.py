from pathlib import Path
s=Path('work/test_owned_vector_undo_x64.cpp').read_text().replace('captureVectorUndo','captureQuaternionUndo').replace('replayVectorUndo','replayQuaternionUndo').replace('array<float,3>next{float(seed+1),2,3}','array<float,4>next{float(seed+1),2,3,4}').replace('v.wire.resize(12)','v.wire.resize(16)').replace('value.data(),12','value.data(),16').replace('"position"','"orientation"').replace('{2,10,{v}}','{2,13,{v}}').replace('owned_vector_undo_redo_cycles','owned_quaternion_undo_redo_cycles')
Path('work/test_owned_quaternion_undo_x64.cpp').write_text(s)
