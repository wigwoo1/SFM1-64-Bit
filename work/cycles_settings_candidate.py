'Validation shared by editor-side settings and the private Blender worker.'
import math

def validate(item,value,name):
 if item['array_length']:
  if not isinstance(value,list) or len(value)!=item['array_length']:raise ValueError('Invalid array: '+name)
  scalar=dict(item,array_length=0)
  for component in value:validate(scalar,component,name)
  return
 kind=item['type']
 if kind=='BOOLEAN' and type(value) is not bool:raise ValueError('Expected boolean: '+name)
 if kind=='INT' and type(value) is not int:raise ValueError('Expected integer: '+name)
 if kind=='FLOAT' and (type(value) not in (int,float) or not math.isfinite(value)):raise ValueError('Expected finite number: '+name)
 if kind in ('INT','FLOAT') and not item['minimum']<=value<=item['maximum']:raise ValueError('Setting outside supported range: '+name)
 if kind=='ENUM' and value not in item['choices']:raise ValueError('Unsupported enum: '+name)
 if kind=='STRING' and type(value) is not str:raise ValueError('Expected string: '+name)

def validate_settings(schema, settings):
 if type(settings) is not dict:raise ValueError('Cycles settings must be an object')
 definitions=schema['properties']
 for name,value in settings.items():
  if name not in definitions:raise ValueError('Unknown or unsupported Cycles setting: '+name)
  validate(definitions[name],value,name)
