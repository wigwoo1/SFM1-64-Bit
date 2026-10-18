import contextlib,io,json,sys
import verify_native_convex_brush_ray as corpus
from pe_image import Image
def main():
 image=Image(corpus.ENGINE);corpus.require(corpus.sha(corpus.ENGINE)==corpus.DIGEST,'Engine changed');offset=image.offset(0x10146c30);raw=image.b[offset:offset+0x1a7];header='// Original stationary brush primitive; test only.\nstatic const unsigned char nativeStationaryBrushBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n';hp=corpus.ROOT/'work/native_stationary_brush_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared stationary brush bytes');return
 corpus.require(hp.read_text()==header,'Stale stationary body');corpus.OUT=corpus.ROOT/'outputs/stationary-brush-20261015';corpus.require(raw in (corpus.OUT/'brush_ray_native_x86.exe').read_bytes(),'Original stationary body absent');captured=io.StringIO()
 with contextlib.redirect_stdout(captured):corpus.main()
 summary=json.loads(captured.getvalue());path=corpus.Path(summary['receipt']);receipt=json.loads(path.read_text());receipt['status']='PASS_NATIVE_STATIONARY_BRUSH_PRIMITIVE';receipt['limits']=['Query start is stationary point; end deliberately unused','Native general/optimized stationary primitive, zero extent only','All benchmark brushes tested individually; not surrounding-leaf selection or world fallback','Synthetic surface records; no displacement/editor integration'];path.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=receipt['cases'],receipt=str(path))))
if __name__=='__main__':main()
