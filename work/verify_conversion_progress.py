import json
import tempfile
from pathlib import Path
from conversion_progress_candidate import ConversionProgressCandidate

def event(message='café', fraction=.5):
    return b'SFM_EVENT '+json.dumps(dict(stage='models',message=message,progress=fraction),ensure_ascii=False).encode()+b'\n'

with tempfile.TemporaryDirectory() as directory:
    path=Path(directory)/'log'
    reader=ConversionProgressCandidate(path)
    assert reader.poll() is None
    data=event(); split=data.index('é'.encode())+1
    path.write_bytes(data[:split]); assert reader.poll() is None
    with path.open('ab') as stream: stream.write(data[split:])
    assert reader.poll()['message']=='café'
    with path.open('ab') as stream:
        stream.write(b'SFM_EVENT broken\n'+event('invalid',True)+event('invalid',float('nan')))
    assert reader.poll()['message']=='café'
    with path.open('ab') as stream: stream.write(b'x'*200000+b'\n'+event('recovered'))
    while True:
        offset=reader.offset; result=reader.poll()
        assert reader.offset-offset<=65536 and len(reader.pending)<=65536
        if reader.offset==offset: break
    assert result['message']=='recovered'
    path.write_bytes(event('reset',.25))
    assert reader.poll()['message']=='reset'
    result=reader.poll(); result['message']='mutated'
    assert reader.poll()['message']=='reset'
print('PASS: split UTF-8 events, invalid JSON/fractions, bounded oversized-line recovery, truncation and independent result copies')

reader=ConversionProgressCandidate('work/cj/5aecc5de/controller-worker.log')
while True:
    offset=reader.offset; result=reader.poll()
    if reader.offset==offset: break
print(json.dumps(dict(live_log_bytes=reader.offset,latest_event=result)))
