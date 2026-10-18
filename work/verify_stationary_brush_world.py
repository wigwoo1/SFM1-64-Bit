import contextlib,io,json,sys
import verify_brush_only_world as corpus
from pe_image import Image
def main():
 image=Image(corpus.ENGINE);corpus.require(corpus.sha(corpus.ENGINE)==corpus.DIGEST,'Engine changed');ranges=[(0x10148030,0x10148156),(0x10147f40,0x1014802a),(0x10146c30,0x10146dd7),(0x10145810,0x101459a4),(0x102df090,0x102df0bb),(0x102de362,0x102de36c)];header='// Original stationary world/leaf/brush/selector and stack/cookie helpers.\n';blobs=[]
 for i,(a,z) in enumerate(ranges):
  raw=image.b[image.offset(a):image.offset(a)+(z-a)];blobs.append(raw);header+='static const unsigned char stationaryWorldPart'+str(i)+'[]={'+','.join('0x%02x'%v for v in raw)+'};\n'
 header+='struct StationaryWorldFragment{unsigned offset;const unsigned char*bytes;unsigned size;};\nstatic const StationaryWorldFragment stationaryWorldFragments[]={'+','.join('{0x%x,stationaryWorldPart%d,sizeof(stationaryWorldPart%d)}'%(a-0x10000000,i,i) for i,(a,z) in enumerate(ranges))+'};\n';hp=corpus.ROOT/'work/native_stationary_world_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared stationary world routines');return
 corpus.require(hp.read_text()==header,'Stale stationary world bytes');corpus.OUTPUT=corpus.ROOT/'outputs/stationary-brush-world-20261015';binary=(corpus.OUTPUT/'world_native_x86.exe').read_bytes();corpus.require(all(blob in binary for blob in blobs),'Original stationary routines absent');captured=io.StringIO()
 with contextlib.redirect_stdout(captured):corpus.main()
 summary=json.loads(captured.getvalue());path=corpus.Path(summary['receipt']);receipt=json.loads(path.read_text());receipt['status']='PASS_NATIVE_STATIONARY_BRUSH_ONLY_WORLD';receipt['limits']=['Query start stationary point; end unused','Original world/leaf/brush/selector routines; runtime records supplied by checked owned loader','Native displacement spans deliberately zero','Synthetic surface metadata; no editor integration'];path.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=receipt['cases'],coverage=receipt['coverage'],receipt=str(path))))
if __name__=='__main__':main()
