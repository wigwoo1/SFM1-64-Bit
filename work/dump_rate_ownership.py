from pathlib import Path
p=Path('work/inspect_session_rates.cpp');s=p.read_text();s=s.replace('auto g=loadGraph(argv[1]);','auto g=loadGraph(argv[1]);RateOwnershipDump::dump(g);');p.write_text(s)
