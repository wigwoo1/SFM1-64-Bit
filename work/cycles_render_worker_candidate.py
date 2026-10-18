"""Private Blender render worker; no writes to input .blend or SFM assets."""
import sys,json,math,hashlib,traceback
from pathlib import Path
import bpy
sys.path.insert(0,str(Path(__file__).resolve().parent))
from cycles_settings_candidate import validate_settings
if hasattr(sys.stdout,'reconfigure'):sys.stdout.reconfigure(line_buffering=True)
def file_hash(path):
 digest=hashlib.sha256()
 with path.open("rb") as stream:
  for block in iter(lambda:stream.read(8*1024*1024),b""):digest.update(block)
 return digest.hexdigest()
def value_json(value):
 if isinstance(value,(str,bool,int,float)):return value
 return list(value)
def schema(settings):
 result={};unsupported={}
 for prop in settings.bl_rna.properties:
  name=prop.identifier
  if name=='rna_type' or prop.is_readonly:continue
  if prop.type not in ('BOOLEAN','INT','FLOAT','ENUM','STRING'):unsupported[name]=prop.type;continue
  item=dict(type=prop.type,label=prop.name,description=prop.description,array_length=getattr(prop,"array_length",0),default=value_json(getattr(settings,name)))
  if prop.type in ('INT','FLOAT'):item.update(minimum=prop.hard_min,maximum=prop.hard_max)
  if prop.type=='ENUM':
   item['choices']=[entry.identifier for entry in prop.enum_items]
   if not item['choices']:
    from cycles import properties as cycles_properties
    callback=getattr(cycles_properties,'enum_'+name,None)
    if callback:item['choices']=[entry[0] for entry in callback(settings,bpy.context)]
  result[name]=item
 return dict(properties=result,unsupported=unsupported)
def equal_value(actual,requested):
 if type(requested) is float:return type(actual) in (float,int) and math.isclose(actual,requested,rel_tol=1e-6,abs_tol=1e-9)
 if isinstance(requested,list):return isinstance(actual,list) and len(actual)==len(requested) and all(equal_value(a,b) for a,b in zip(actual,requested))
 return actual==requested

def main(job):
 if job.get('version')!=1:raise ValueError('Unsupported Cycles job version')
 if job.get('mode') not in ('inspect','render'):raise ValueError('Unsupported Cycles job mode')
 source=Path(job['input_blend']) if job.get('input_blend') else None
 digest=None
 if source:
  digest=file_hash(source)
  bpy.ops.wm.open_mainfile(filepath=str(source.resolve()),load_ui=False,use_scripts=False)
 report=dict(version=1,job_id=job.get('job_id'),status='inspected',blender_version=bpy.app.version_string,input_sha256=digest,cycles_schema=schema(bpy.context.scene.cycles),scenes=[dict(name=s.name,camera=s.camera.name if s.camera else None,objects=len(s.objects),frame_start=s.frame_start,frame_end=s.frame_end,current_frame=s.frame_current) for s in bpy.data.scenes])
 conversion=bpy.data.texts.get('SFM Conversion Report.json')
 if conversion:
  try:report['conversion_report']=json.loads(conversion.as_string())
  except ValueError:report['conversion_report_error']='Invalid embedded report'
 if job['mode']=='inspect':return report
 if not source:raise ValueError('Render requires input_blend')
 scene=bpy.data.scenes.get(job['scene'])
 if scene is None or scene.camera is None:raise ValueError('Render scene/camera unavailable')
 settings=job.get('cycles',{})
 validate_settings(report['cycles_schema'],settings)
 width,height=job['resolution'];frame=job['frame']
 if type(width) is not int or type(height) is not int or not 64<=width<=8192 or not 64<=height<=8192:raise ValueError('Render resolution invalid')
 if type(frame) is not int:raise ValueError('Render frame invalid')
 effective={}
 for target in bpy.data.scenes:
  target.render.engine='CYCLES'
  for name,value in settings.items():setattr(target.cycles,name,value)
  target.render.resolution_x=width;target.render.resolution_y=height;target.render.resolution_percentage=100
  effective[target.name]={name:value_json(getattr(target.cycles,name)) for name in settings}
  for name,value in settings.items():
   if not equal_value(effective[target.name][name],value):raise ValueError('Cycles clamped requested setting: '+name)
 for target in bpy.data.scenes:
  tree=getattr(target,'node_tree',None) or getattr(target,'compositing_node_group',None)
  if tree and any(node.type=='OUTPUT_FILE' for node in tree.nodes):raise ValueError('Compositor file outputs require explicit output routing')
 scene.frame_set(frame);output=Path(job['output_png']);output.parent.mkdir(parents=True,exist_ok=True)
 scene.render.image_settings.file_format='PNG';scene.render.filepath=str(output.resolve());scene.render.use_file_extension=True
 print(json.dumps(dict(event='render_started',scene=scene.name,frame=frame,resolution=[width,height])))
 bpy.ops.render.render(write_still=True,scene=scene.name)
 if not output.is_file():raise RuntimeError('Render did not produce expected PNG')
 if file_hash(source)!=digest:raise RuntimeError('Input changed during render')
 report.update(status='rendered',scene=scene.name,frame=frame,resolution=[width,height],effective_cycles=effective,output_png=str(output.resolve()),output_sha256=file_hash(output))
 return report
job=json.loads(Path(sys.argv[sys.argv.index('--')+1]).read_text(encoding='utf-8'));result_path=Path(job['result']);result_path.parent.mkdir(parents=True,exist_ok=True)
try:result=main(job)
except Exception as error:
 result=dict(version=1,job_id=job.get('job_id'),status='failed',error=str(error));traceback.print_exc()
 temporary=result_path.with_suffix('.building.json');temporary.write_text(json.dumps(result,indent=2,allow_nan=False),encoding='utf-8');temporary.replace(result_path)
 raise
else:
 temporary=result_path.with_suffix('.building.json');temporary.write_text(json.dumps(result,indent=2,allow_nan=False),encoding='utf-8');temporary.replace(result_path)
 print(json.dumps(dict(event='complete',status=result['status'],result=str(result_path))))
