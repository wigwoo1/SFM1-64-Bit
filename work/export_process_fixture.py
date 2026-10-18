import json
import subprocess
import sys
import time
from pathlib import Path
child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(120)'])
Path(sys.argv[1]).write_text(json.dumps(dict(child=child.pid,args=sys.argv[2:])))
time.sleep(120)
