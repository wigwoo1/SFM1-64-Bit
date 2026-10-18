"""Bounded incremental reading of converter JSON events from a private log."""
import json
import math
from pathlib import Path

class ConversionProgressCandidate:
    def __init__(self,path):
        self.path=Path(path);self.offset=0;self.pending=b'';self.discarding=False;self.latest=None
    def poll(self):
        try:
            with self.path.open('rb') as stream:
                stream.seek(0,2)
                if stream.tell()<self.offset:
                    self.offset=0;self.pending=b'';self.discarding=False;self.latest=None
                stream.seek(self.offset)
                chunk=stream.read(65536);self.offset+=len(chunk)
        except FileNotFoundError:return self.latest.copy() if self.latest else None
        lines=(self.pending+chunk).split(b'\n');self.pending=lines.pop()
        for line in lines:
            if self.discarding:
                self.discarding=False;continue
            if len(line)>65536 or not line.startswith(b'SFM_EVENT '):continue
            try:
                event=json.loads(line[10:])
                stage,message,fraction=event['stage'],event['message'],event.get('progress')
                if not isinstance(stage,str) or not isinstance(message,str) or len(message)>8192:continue
                if fraction is not None and (type(fraction) not in (int,float) or not math.isfinite(fraction) or not 0<=fraction<=1):continue
                self.latest=dict(stage=stage,message=message,fraction=fraction)
            except (ValueError,KeyError,TypeError):continue
        if len(self.pending)>65536:
            self.pending=b'';self.discarding=True
        return self.latest.copy() if self.latest else None
