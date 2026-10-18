from pathlib import Path
p=Path('work/test_owned_selected_ik_x64.cpp');s=p.read_text();s=s.replace('#include <iostream>','#include <iostream>\n#include "graph_exact_equal.h"')
old='return selected;},phase);';assert old in s;s=s.replace(old,'return selected;},phase,true);')
old='if(phase==0&&mask==7)finalGraph=exported;';assert old in s
new='''if(actual.undoRecords.size()!=actual.changes.attributes.size())throw std::runtime_error("selected undo record count");if(replayOwnedIkUndoRecordsCandidate(p,actual.undoRecords,true)!=actual.undoRecords.size()||!graphExactEqualCandidate(p.graph,base))throw std::runtime_error("selected full graph undo");if(replayOwnedIkUndoRecordsCandidate(p,actual.undoRecords,false)!=actual.undoRecords.size()||!graphExactEqualCandidate(p.graph,expected))throw std::runtime_error("selected full graph redo");if(phase==0&&mask==7)finalGraph=exported;'''
s=s.replace(old,new).replace('payload_phase_dirty_reload_teardown=passed','payload_phase_dirty_full_graph_undo_redo_reload_teardown=passed');p.write_text(s)
p=Path('build-session-core.cmd');s=p.read_text();old='test_session_ik_selected_controls)';assert old in s;p.write_text(s.replace(old,'test_session_ik_selected_controls test_owned_vector_undo_x64 test_owned_quaternion_undo_x64 test_owned_ik_undo_pass_x64)'))
p=Path('test-session-core.ps1');s=p.read_text();old="'test_owned_typed_write_x64'))";assert old in s;s=s.replace(old,"'test_owned_typed_write_x64', 'test_owned_vector_undo_x64', 'test_owned_quaternion_undo_x64'))")
old="    Invoke-CoreCheck 'test_owned_session_ik_pass_x64'";assert old in s;s=s.replace(old,"    Invoke-CoreCheck 'test_owned_ik_undo_pass_x64' @($sessionInput, (Join-Path $sessionOutputDirectory \"ik-undo-$sessionFixture.dmx\"))\n"+old);p.write_text(s)
