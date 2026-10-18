"""Private nonblocking conversion controller sharing render-job cleanup policy."""
import json
import os
import subprocess
from pathlib import Path
from cycles_job_controller_candidate import CyclesJobControllerCandidate
from sfm_cycles_conversion_candidate import accept_conversion
from conversion_progress_candidate import ConversionProgressCandidate

class ConversionJobControllerCandidate(CyclesJobControllerCandidate):
    def start(self, prepared, expected_imported_obj=None, current_session=None):
        prepared=json.loads(json.dumps(prepared,allow_nan=False))
        expected=json.loads(json.dumps(expected_imported_obj,allow_nan=False))
        identifier=prepared['job']['run_id']
        if identifier in self.jobs:raise ValueError('Conversion job already started')
        folder=Path(prepared['folder']).resolve()
        if not folder.is_relative_to(self.root):raise ValueError('Conversion folder outside controller root')
        if Path(prepared['job']['session']).resolve()!=folder/'session.dmx':
            raise ValueError('Conversion input is not the private captured session')
        job_path=folder/'job.json'
        if json.loads(job_path.read_text(encoding='utf-8'))!=prepared['job']:
            raise ValueError('Prepared conversion parameters changed')
        if (folder/'session.report.json').exists() or (folder/'session.blend').exists():
            raise ValueError('Conversion job already has an output')
        log=(folder/'controller-worker.log').open('xb')
        env=os.environ.copy();env['TEMP']=env['TMP']=str(folder)
        args=[str(self.blender),'--background','--factory-startup','--disable-autoexec',
              '--threads','2','--python-exit-code','1','--python',
              str(Path(__file__).with_name('sfm_cycles_conversion_worker_candidate.py').resolve()),
              '--',str(job_path)]
        try:
            process=subprocess.Popen(args,stdout=log,stderr=subprocess.STDOUT,env=env,
                creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        except BaseException:
            log.close();raise
        self.jobs[identifier]=dict(process=process,log=log,folder=folder,prepared=prepared,
            expected=expected,current_session=str(Path(current_session).resolve()) if current_session else None,
            state='running',progress=ConversionProgressCandidate(folder/'controller-worker.log'))
        return identifier

    def poll(self, identifier):
        record=self.jobs[identifier]
        if record['state'] not in ('running','cancelling'):return dict(record['outcome'])
        code=record['process'].poll()
        if code is None:return dict(state=record['state'],job_id=identifier,pid=record['process'].pid,
                                   progress=record['progress'].poll())
        record['log'].close()
        outcome=dict(state='failed',job_id=identifier,exit_code=code,
                     log=str(record['folder']/'controller-worker.log'))
        if record['state']=='cancelling':outcome['state']='cancelled'
        else:
            try:
                result=accept_conversion(record['prepared'],code,record['current_session'],record['expected'])
                outcome.update(state='complete',result=result)
            except Exception as error:outcome['error']=str(error)
        record['state']=outcome['state'];record['outcome']=outcome
        return dict(outcome)
