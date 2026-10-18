from pathlib import Path
p=Path('work/live_session_frame_provider_candidate.h')
s=p.read_text()
start=s.index('std::cout<<"live_session_frame=')
end=s.index(';RenderFrameStateCandidate',start)
statement=s[start:end].replace('std::cout','frameLog',1)
s=s[:start]+'std::ostringstream frameLog;'+statement+';std::cout<<frameLog.str()'+s[end:]
p.write_text(s)
