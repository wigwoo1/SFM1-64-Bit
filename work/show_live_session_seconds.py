from pathlib import Path
p=Path('work/live_session_frame_provider_candidate.h');s=p.read_text().replace('<<" native_scheduler=unfinished\\n";', '<<" native_scheduler=unfinished session_seconds="<<recoveredTickSecondsCandidate(ticks.at(frame))<<" nominal_ticks_per_second=10000\\n";');p.write_text(s)
