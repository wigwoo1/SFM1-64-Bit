"""Controller terminal protocol check; launches no Blender process."""
import hashlib
import io
import json
import tempfile
from pathlib import Path
from PIL import Image
from cycles_job_controller_candidate import CyclesJobControllerCandidate

class ExitedProcess:
    def poll(self):
        return 0

with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary)
    controller=CyclesJobControllerCandidate(root/'unused-blender.exe',root)
    for case in ('valid','missing','replacement'):
        folder=root/case
        folder.mkdir()
        output=folder/'render.png'
        Image.new('RGB',(64,64)).save(output)
        report=dict(version=1,job_id=case,status='rendered',output_png=str(output),
                    output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())
        if case=='missing':
            del report['output_sha256']
        if case=='replacement':
            Image.new('RGB',(64,64),'red').save(output)
        (folder/'result.json').write_text(json.dumps(report))
        controller.jobs[case]=dict(process=ExitedProcess(),log=io.BytesIO(),
            folder=folder,payload=dict(mode='render',resolution=[64,64]),state='running')
        result=controller.poll(case)
        assert result['state']==('complete' if case=='valid' else 'failed'),result
        if case!='valid':
            assert 'changed after worker completion' in result['error'],result
    print('PASS terminal controller: valid digest accepted, missing digest and valid replacement PNG rejected; no Blender launched')
