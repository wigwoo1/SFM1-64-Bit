"""Backend conversion-to-render state machine; native editor integration pending."""
import json
from pathlib import Path
from sfm_cycles_conversion_candidate import file_digest
from sfm_conversion_job_controller_candidate import ConversionJobControllerCandidate
from cycles_job_controller_candidate import CyclesJobControllerCandidate
from cycles_settings_candidate import validate_settings

class SfmCyclesBridgeCandidate:
    def __init__(self, blender, conversion_root, render_root):
        self.conversions=ConversionJobControllerCandidate(blender,conversion_root)
        self.renders=CyclesJobControllerCandidate(blender,render_root)
        self.jobs={}

    def start(self, prepared, render_settings, expected_obj=None, current_session=None, cycles_schema=None):
        settings=json.loads(json.dumps(render_settings,allow_nan=False))
        if type(settings) is not dict:raise ValueError('Render settings must be an object')
        if settings.get('renderer','cycles')!='cycles':
            raise ValueError('This bridge requires a Cycles render request')
        resolution=settings.get('resolution')
        if (type(resolution) is not list or len(resolution)!=2 or
            any(type(value) is not int or not 64<=value<=8192 for value in resolution)):
            raise ValueError('Render resolution invalid')
        if 'frame' in settings and type(settings['frame']) is not int:
            raise ValueError('Render frame invalid')
        if 'scene' in settings and (type(settings['scene']) is not str or not settings['scene']):
            raise ValueError('Render scene invalid')
        if cycles_schema is not None:validate_settings(cycles_schema,settings.get('cycles',{}))
        elif type(settings.get('cycles',{})) is not dict:
            raise ValueError('Cycles settings must be an object')
        identifier=self.conversions.start(prepared,expected_obj,current_session)
        self.jobs[identifier]=dict(state='converting',settings=settings,
            current_session=str(Path(current_session).resolve()) if current_session else None,
            session_sha256=prepared['job']['session_sha256'])
        return identifier

    def _render_outcome(self, identifier, record, rendered):
        result=dict(state=rendered['state'],job_id=identifier,
                    conversion=record['conversion'],render=rendered)
        if rendered['state']=='complete' and record.get('current_session'):
            try:
                if file_digest(record['current_session'])!=record['session_sha256']:
                    raise ValueError('Rendered result is stale for the current session')
            except Exception as error:
                result.update(state='failed',error=str(error),rejected_render=rendered,
                              render=dict(state='failed',error=str(error)))
        record.update(state=result['state'],outcome=result)
        return dict(result)

    def poll(self, identifier):
        record=self.jobs[identifier]
        if record['state'] in ('complete','failed','cancelled'):return dict(record['outcome'])
        if record['state']=='converting':
            outcome=self.conversions.poll(identifier)
            if outcome['state']=='running':return dict(state='converting',job_id=identifier,pid=outcome['pid'],progress=outcome.get('progress'))
            if outcome['state']!='complete':
                record.update(state=outcome['state'],outcome=outcome);return dict(outcome)
            record['conversion']=outcome
            try:
                accepted=outcome['result'];settings=record['settings']
                name=settings.get('scene',accepted['shots'][0]['name'])
                shot=next((shot for shot in accepted['shots'] if shot['name']==name),None)
                if shot is None:raise ValueError('Render selection is not a converted shot')
                frame=settings.get('frame',shot['frame_start'])
                if type(frame) is not int or not shot['frame_start']<=frame<=shot['frame_end']:
                    raise ValueError('Render frame outside converted shot')
                payload=dict(settings,version=1,mode='render',scene=name,frame=frame,
                             input_blend=accepted['input_blend'])
                record['render_id']=self.renders.start(payload)
                record['state']='rendering'
            except Exception as error:
                result=dict(state='failed',job_id=identifier,error=str(error),conversion=outcome)
                record.update(state='failed',outcome=result);return result
        if record['state']=='cancelling':
            child=record['cancel_controller'].poll(record['cancel_id'])
            if child['state']=='cancelling':return dict(state='cancelling',job_id=identifier)
            result=dict(state='cancelled',job_id=identifier)
            record.update(state='cancelled',outcome=result);return result
        rendered=self.renders.poll(record['render_id'])
        if rendered['state']=='running':return dict(state='rendering',job_id=identifier,pid=rendered['pid'])
        return self._render_outcome(identifier,record,rendered)

    def cancel(self, identifier):
        record=self.jobs[identifier]
        if record['state'] not in ('converting','rendering'):return False
        controller=self.conversions if record['state']=='converting' else self.renders
        child_id=identifier if record['state']=='converting' else record['render_id']
        if controller.cancel(child_id):
            record.update(state='cancelling',cancel_controller=controller,cancel_id=child_id)
        else:
            if record['state']=='rendering':
                rendered=controller.poll(child_id)
                self._render_outcome(identifier,record,rendered)
                return False
            # A terminal conversion must not start a render after cancellation.
            record.update(state='cancelled',outcome=dict(state='cancelled',job_id=identifier))
        return True

    def close(self):
        for identifier in self.jobs:self.cancel(identifier)
        self.conversions.close();self.renders.close()
        for identifier,record in self.jobs.items():
            if record['state']=='cancelling':self.poll(identifier)
