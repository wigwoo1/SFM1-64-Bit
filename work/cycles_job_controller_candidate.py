"""Nonblocking controller for private Blender jobs; results stay private until validated."""
import hashlib,json,os,struct,subprocess,uuid,zlib
from pathlib import Path
def verify_png_structure_candidate(path):
 size=path.stat().st_size
 with path.open('rb') as stream:
  if stream.read(8)!=b'\x89PNG\r\n\x1a\n':raise ValueError('Invalid PNG signature')
  while True:
   header=stream.read(8)
   if len(header)!=8:raise ValueError('Truncated PNG chunk header')
   length,kind=struct.unpack('>I4s',header)
   if length>size-stream.tell()-4:raise ValueError('Truncated PNG chunk')
   crc=zlib.crc32(kind);remaining=length
   while remaining:
    data=stream.read(min(remaining,65536))
    if not data:raise ValueError('Truncated PNG data')
    crc=zlib.crc32(data,crc);remaining-=len(data)
   checksum=stream.read(4)
   if len(checksum)!=4 or struct.unpack('>I',checksum)[0]!=(crc&0xffffffff):raise ValueError('PNG checksum mismatch')
   if kind==b'IEND':
    if length or stream.read(1):raise ValueError('Invalid PNG end/trailing data')
    return
class CyclesJobControllerCandidate:
 def __init__(self,blender,root):
  self.blender=Path(blender).resolve();self.root=Path(root).resolve();self.root.mkdir(parents=True,exist_ok=True);self.jobs={}
 def start(self,job):
  identifier=str(uuid.uuid4());folder=self.root/identifier;folder.mkdir();payload=json.loads(json.dumps(job,allow_nan=False));payload.update(job_id=identifier,result=str(folder/'result.json'),output_png=str(folder/'render.png'))
  path=folder/'job.json';path.write_text(json.dumps(payload,indent=2,allow_nan=False),encoding='utf-8');log=(folder/'worker.log').open('wb');env=os.environ.copy();env['TEMP']=env['TMP']=str(folder)
  args=[str(self.blender),'--background','--factory-startup','--disable-autoexec','--threads','2','--python-exit-code','1','--python',str(Path(__file__).with_name('cycles_render_worker_candidate.py').resolve()),'--',str(path)]
  try:process=subprocess.Popen(args,stdout=log,stderr=subprocess.STDOUT,env=env,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
  except BaseException:log.close();raise
  self.jobs[identifier]=dict(process=process,log=log,folder=folder,payload=payload,state='running');return identifier
 def poll(self,identifier):
  record=self.jobs[identifier]
  if record['state'] not in ('running','cancelling'):return dict(record['outcome'])
  code=record['process'].poll()
  if code is None:return dict(state=record['state'],job_id=identifier,pid=record['process'].pid)
  if record['state']=='cancelling':
   record['log'].close();record['state']='cancelled';record['outcome']=dict(state='cancelled',job_id=identifier,exit_code=code,log=str(record['folder']/'worker.log'));return dict(record['outcome'])
  record['log'].close();record['state']='failed';outcome=dict(state='failed',job_id=identifier,exit_code=code,log=str(record['folder']/'worker.log'))
  try:
   result=json.loads((record['folder']/'result.json').read_text(encoding='utf-8'))
   if result.get('version')!=1 or result.get('job_id')!=identifier:raise ValueError('Result job identity mismatch')
   if code!=0 or result.get('status')=='failed':raise ValueError(result.get('error','Blender job failed'))
   expected='rendered' if record['payload']['mode']=='render' else 'inspected'
   if result.get('status')!=expected:raise ValueError('Unexpected result status')
   if expected=='rendered':
    output=record['folder']/'render.png'
    if result.get('output_png')!=str(output):raise ValueError('Unexpected output path')
    digest=hashlib.sha256()
    with output.open('rb') as stream:
     for block in iter(lambda:stream.read(65536),b''):digest.update(block)
    if result.get('output_sha256')!=digest.hexdigest():raise ValueError('Render image changed after worker completion')
    with output.open('rb') as image:header=image.read(24)
    if len(header)!=24 or header[:8]!=b'\x89PNG\r\n\x1a\n' or header[12:16]!=b'IHDR':raise ValueError('Missing/invalid render PNG')
    if list(struct.unpack('>II',header[16:24]))!=record['payload']['resolution']:raise ValueError('Render dimensions mismatch')
    verify_png_structure_candidate(output)
    from PIL import Image
    with Image.open(output) as image:image.verify()
    with Image.open(output) as image:image.load()
   record['state']='complete';outcome.update(state='complete',result=result)
  except Exception as error:outcome['error']=str(error)
  record['outcome']=outcome;return dict(outcome)
 def cancel(self,identifier):
  record=self.jobs[identifier]
  if self.poll(identifier)['state']!='running':return False
  record['process'].terminate();record['state']='cancelling';return True
 def close(self):
  for identifier,record in self.jobs.items():
   if record['state']=='running':self.cancel(identifier)
  for identifier,record in self.jobs.items():
   if record['state']=='cancelling':
    try:record['process'].wait(timeout=5)
    except subprocess.TimeoutExpired:record['process'].kill();record['process'].wait(timeout=5)
    self.poll(identifier)
